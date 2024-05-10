#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/wait.h>

#include "buddy.h"
#include "headers.h"
#include "pri_queue.h"
#include "process_table.h"

// Function prototypes
pcb* getProcess();
void outputPerfFile(char* filename);
void outputPCBLogEntry(char* filename, pcb* pcb, int time);
void addPCBtoReadyQueue(PriQueue** priQueue, pcb* pcb,
                        enum schedulingAlgorithm algorithm);
bool finishProcess(pcb* runningProcess, enum schedulingAlgorithm algorithm);
void updateStoppedTime(pcb* process, pcb* runningProcess, int currentTime);
void updateRemainingTime(pcb* process, int currentTime);

// Holds the Message Queue ID between the scheduler and the process
// generator
int SchedulerQueueID = -1;

// Aggregated times for performance metrics
int totalWaitingTime = 0;
int totalTurnaroundTime = 0;
float totalWeightedTurnaroundTime = 0;
float totalWeightedTurnaroundTimeSquared = 0;
int totalBurstTime = 0;
int countProcesses = 0;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf(
            "Usage: %s <num processes> <scheduling algorithm number>"
            "[<quantum>]\n",
            argv[0]);
        exit(1);
    }

    // number of process that will enter the system
    int totalProcesses = atoi(argv[1]);

    // scheduling algorithm that will be used
    enum schedulingAlgorithm algorithm = atoi(argv[2]);

    // If the quantum is not specified for RR algorithm exit
    if (algorithm == RR && argc < 4) {
        printf(
            "Usage: %s <num processes> <scheduling algorithm number> "
            "[<quantum>]\n",
            argv[0]);
        exit(1);
    }

    int quantum = atoi(argv[3]);

    // establish connection with the message queue  in order to receive
    key_t key = ftok(MSGQUEUENAME, MSGQUEUEKEY);
    SchedulerQueueID = msgget(key, 0666);
    if (SchedulerQueueID == -1) {
        perror("Error in getting message queue");
        exit(1);
    }

    // create structures for the scheduler
    buddy* buddy = createBuddy(1024);
    ProcessTable* processTable = createProcessTable();
    PriQueue* readyQueue = createPriQueue();

    // output headers for the log file
    FILE* file = fopen(schedulerLogFile, "w");
    fprintf(file, "#At time x process y state arr w total z remain y wait k\n");
    fclose(file);

    // output headers for the memory file
    FILE* memoryFile = fopen(memoryLogFile, "w");
    fprintf(memoryFile,
            "#At time x allocated y bytes for process z from i to j\n");
    fclose(memoryFile);

    // initialize the clock
    initClk();

    // holds a pointer to the current running process
    pcb* runningProcess = NULL;

    // main scheduling loop
    while (true) {
        bool quantumOver = false;

        // wait for either the quantum to finish or the process to finish
        while (runningProcess && algorithm == RR && !quantumOver) {
            int waitTime = quantum;

            if (runningProcess->remainingTime < quantum)
                waitTime = runningProcess->remainingTime;

            quantumOver = (getClk() - runningProcess->resumedTime) >= waitTime;
        }

        // check if the running process has finished
        if (runningProcess) {
            // if the process has finished delete it and output the log entry
            if (finishProcess(runningProcess, algorithm)) {
                removeProcess(buddy, runningProcess);
                outputPCBLogEntry(schedulerLogFile, runningProcess,
                                  runningProcess->finishTime);
                deletePCB(&processTable, runningProcess->id);
                runningProcess = NULL;

            }
            // add the process back to the ready queue if it's not finished
            else {
                addPCBtoReadyQueue(&readyQueue, runningProcess, algorithm);
            }
        }

        // Wait for processes to arrive
        while (true) {
            pcb* newProcess = getProcess();
            if (!newProcess) break;
            printf("[ENTERED]: %d with memsize %d\n", newProcess->id,
                   newProcess->memsize);
            addPCBtoReadyQueue(&readyQueue, newProcess, algorithm);
            addPCBFront(&processTable, newProcess);
            countProcesses++;
        }

        // the next process that should run
        pcb* nextProcess = popPriQueue(&readyQueue);

        int currentTime = getClk();
        updateRemainingTime(runningProcess, currentTime);
        updateStoppedTime(nextProcess, runningProcess, currentTime);

        // if the remainingTime is 0 wait for waitpid to return
        while (runningProcess && runningProcess->remainingTime == 0) {
            if (finishProcess(runningProcess, algorithm)) {
                removeProcess(buddy, runningProcess);
                outputPCBLogEntry(schedulerLogFile, runningProcess,
                                  runningProcess->finishTime);
                deletePCB(&processTable, runningProcess->id);
                runningProcess = NULL;
                break;
            }
        }

        bool currentNotSameAsNext =
            runningProcess && nextProcess && runningProcess != nextProcess;

        if (algorithm == SRTN && currentNotSameAsNext &&
            runningProcess->remainingTime <= nextProcess->remainingTime) {
            addPCBtoReadyQueue(&readyQueue, nextProcess, algorithm);
            continue;
        }

        bool SRTNShouldStop =
            runningProcess && nextProcess &&
            runningProcess->remainingTime > nextProcess->remainingTime;

        // If preemptive and current is different from next
        // stop the current Process (if RR check if quantum is over)
        if (((algorithm == SRTN && SRTNShouldStop) ||
             (algorithm == RR && quantumOver)) &&
            currentNotSameAsNext) {
            runningProcess->state = STOPPED;
            runningProcess->stoppedTime = getClk();
            runningProcess->remainingTimeAfterStop =
                runningProcess->remainingTime;
            kill(runningProcess->pid, SIGSTOP);
            outputPCBLogEntry(schedulerLogFile, runningProcess, getClk());
            runningProcess = NULL;
        }

        // ensures a single CPU architecture
        // no process can start if there is a running process
        if (runningProcess) continue;

        // start the next process
        if (nextProcess && nextProcess->pid == -1) {
            // printf("Starting process %d %d\n", nextProcess->id,
            //    nextProcess->memsize);
            if (!insertBuddy(buddy, nextProcess, getClk())) {
                // printf("Skipped process %d\n", nextProcess->id);
                addPCBtoReadyQueue(&readyQueue, nextProcess, algorithm);
                continue;
            }

            int pid = fork();

            nextProcess->startTime = getClk();
            nextProcess->resumedTime = nextProcess->startTime;
            nextProcess->state = STARTED;

            char remainingTime[10];
            sprintf(remainingTime, "%d", nextProcess->remainingTime);

            if (pid == 0) {
                char* args[] = {"./process.out", remainingTime, NULL};
                execv(args[0], args);
            }

            nextProcess->pid = pid;
            outputPCBLogEntry(schedulerLogFile, nextProcess,
                              nextProcess->startTime);

            runningProcess = nextProcess;

        } else if (nextProcess && nextProcess->state == STOPPED) {
            nextProcess->resumedTime = getClk();
            nextProcess->state = RESUMED;
            kill(nextProcess->pid, SIGCONT);
            outputPCBLogEntry(schedulerLogFile, nextProcess,
                              nextProcess->resumedTime);

            runningProcess = nextProcess;
        }

        if (!runningProcess && !nextProcess &&
            countProcesses == totalProcesses && readyQueue->size == 0)
            break;
    }
    outputPerfFile(schedulerPerfFile);
    destroyPriQueue(&readyQueue);
    destroyProcessTable(processTable);
    destroyClk(true);
    return 0;
}

void addPCBtoReadyQueue(PriQueue** priQueue, pcb* pcb,
                        enum schedulingAlgorithm algorithm) {
    switch (algorithm) {
        case RR:
            insertPriQueue(priQueue, 10, pcb);
            break;
        case HPF:
            insertPriQueue(priQueue, pcb->priority, pcb);
            break;
        case SRTN:
            insertPriQueue(priQueue, pcb->remainingTime, pcb);
            break;
    }
}

void outputPCBLogEntry(char* filename, pcb* pcb, int time) {
    FILE* file = fopen(filename, "a");

    char state[10];
    switch (pcb->state) {
        case STARTED:
            sprintf(state, "started");
            break;
        case STOPPED:
            sprintf(state, "stopped");
            break;
        case RESUMED:
            sprintf(state, "resumed");
            break;
        case FINISHED:
            sprintf(state, "finished");
            break;
    }

    fprintf(file, "At time %d process %d %s arr %d total %d remain %d wait %d ",
            time, pcb->id, state, pcb->arrivalTime, pcb->burstTime,
            pcb->remainingTime, pcb->waitTime);

    if (pcb->state == FINISHED)
        fprintf(file, "TA %d WTA %.2f\n", pcb->turnaroundTime,
                pcb->weightedTurnaroundTime);
    else
        fprintf(file, "\n");

    fclose(file);
}

void outputPerfFile(char* filename) {
    FILE* file = fopen(filename, "w");

    float avgWeightedTurnaroundTime =
        totalWeightedTurnaroundTime / (float)countProcesses;
    fprintf(file, "CPU utilization = %.2f\n",
            (float)totalBurstTime / (getClk() - 1) * 100);
    fprintf(file, "Avg WTA. = %.2f\n", avgWeightedTurnaroundTime);
    fprintf(file, "Avg Waiting = %.2f\n",
            totalWaitingTime / (float)countProcesses);

    // STD DEV
    // WTA1^2 - 2WTA1*avgWTA + avgWTA^2
    // WTA^2/totalProcesses - 2WTA*avgTA/totalProcesses + avgWTA^2
    // WTA^2/totalProcesses - avgWTA^2

    fprintf(file, "Std WTA = %.2f\n",
            sqrt((totalWeightedTurnaroundTimeSquared / (float)countProcesses) -
                 (avgWeightedTurnaroundTime * avgWeightedTurnaroundTime)));

    fclose(file);
}

pcb* getProcess() {
    processMessage message;
    int rcv =
        msgrcv(SchedulerQueueID, &message, sizeof(message), 0, IPC_NOWAIT);

    if (rcv == -1) return NULL;

    pcb* newProcess = malloc(sizeof(pcb));
    *newProcess = message.process;

    return newProcess;
}

bool finishProcess(pcb* runningProcess, enum schedulingAlgorithm algorithm) {
    if (!runningProcess) return false;

    int status, pid;

    // if non-premptive wait for the process to finish
    if (algorithm == HPF)
        pid = waitpid(runningProcess->pid, &status, !WNOHANG);
    else
        pid = waitpid(runningProcess->pid, &status, WNOHANG);

    if (pid > 0 && WIFEXITED(status)) {
        runningProcess->state = FINISHED;
        runningProcess->remainingTime = 0;
        runningProcess->finishTime = runningProcess->waitTime +
                                     runningProcess->burstTime +
                                     runningProcess->arrivalTime;
        runningProcess->turnaroundTime =
            runningProcess->finishTime - runningProcess->arrivalTime;
        runningProcess->weightedTurnaroundTime =
            runningProcess->turnaroundTime / (float)runningProcess->burstTime;
        totalBurstTime += runningProcess->burstTime;
        totalWaitingTime += runningProcess->waitTime;
        totalTurnaroundTime += runningProcess->turnaroundTime;
        totalWeightedTurnaroundTime += runningProcess->weightedTurnaroundTime;
        totalWeightedTurnaroundTimeSquared +=
            runningProcess->weightedTurnaroundTime *
            runningProcess->weightedTurnaroundTime;

        return true;
    }

    return false;
}

void updateRemainingTime(pcb* process, int currentTime) {
    if (process && process->stoppedTime <= process->resumedTime) {
        int remainingTime = process->remainingTimeAfterStop -
                            (currentTime - process->resumedTime);

        if (remainingTime < 0) {
            process->remainingTime = 0;
        } else {
            process->remainingTime = remainingTime;
        }
    }
}

void updateStoppedTime(pcb* process, pcb* runningProcess, int currentTime) {
    // update the wait time for the process that will run next
    if (process && process != runningProcess) {
        int waitTime = currentTime - process->arrivalTime -
                       (process->burstTime - process->remainingTime);

        if (waitTime < 0) {
            process->waitTime = 0;
        } else {
            process->waitTime = waitTime;
        }
    }
}