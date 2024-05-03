#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/wait.h>

#include "headers.h"
#include "pri_queue.h"
#include "process_table.h"

#define schedulerLogFile "scheduler.log"
#define schedulerPerfFile "scheduler.perf"

pcb* getProcess();
void outputPerfFile(char* filename);
void outputPCBLogEntry(char* filename, pcb* pcb, int time);
void addPCBtoReadyQueue(PriQueue** priQueue, pcb* pcb,
                        enum schedulingAlgorithm algorithm);

int SchedulerQueueID = -1;

int totalWaitingTime = 0;
int totalTurnaroundTime = 0;
float totalWeightedTurnaroundTime = 0;
float totalWeightedTurnaroundTimeSquared = 0;
int totalBurstTime = 0;
int countProcesses = 0;

int main(int argc, char* argv[]) {
    // get algorithm information from command line arguments
    if (argc < 3) {
        printf(
            "Usage: %s <num processes> <scheduling algorithm number>"
            "[<quantum>]\n",
            argv[0]);
        exit(1);
    }

    int totalProcesses = atoi(argv[1]);
    enum schedulingAlgorithm algorithm = atoi(argv[2]);

    if (algorithm == RR && argc < 4) {
        printf(
            "Usage: %s <num processes> <scheduling algorithm number> "
            "[<quantum>]\n",
            argv[0]);
        exit(1);
    }

    int quantum = atoi(argv[3]);

    printf("SCHEDULER: Quantum: %d\n", quantum);

    // establish connection with the message queue
    key_t key = ftok(MSGQUEUENAME, MSGQUEUEKEY);
    SchedulerQueueID = msgget(key, 0666);
    if (SchedulerQueueID == -1) {
        perror("Error in getting message queue");
        exit(1);
    }

    // create structures for the scheduler
    ProcessTable* processTable = createProcessTable();
    PriQueue* readyQueue = createPriQueue();

    // output headers for the log file
    FILE* file = fopen(schedulerLogFile, "w");
    fprintf(file, "#At time x process y state arr w total z remain y wait k\n");
    fclose(file);

    // initialize the clock
    initClk();

    // holds a pointer to the current running process
    pcb* runningProcess = NULL;

    // main scheduling loop
    while (true) {
        bool quantumOver;

        do {
            quantumOver = runningProcess &&
                          (getClk() - runningProcess->resumedTime) >= quantum;
        } while (runningProcess && !quantumOver && algorithm == RR);

        // check if the running process has finished
        if (runningProcess) {
            int status, pid;

            // if non-premptive wait for the process to finish
            if (algorithm == HPF)
                pid = waitpid(runningProcess->pid, &status, !WNOHANG);
            else
                pid = waitpid(runningProcess->pid, &status, WNOHANG);

            // status is only set if pid > 0.
            // if the process has finished successfully update the pcb and log
            if (pid > 0 && WIFEXITED(status)) {
                printf("The pid is %d. The exit code is %d. IFEXITED IS %d\n",
                       pid, WEXITSTATUS(status), WIFEXITED(status));
                printf("[FINISHED]: id: %d %d %d\n", runningProcess->id,
                       getClk(), runningProcess->remainingTime);
                runningProcess->state = FINISHED;
                runningProcess->remainingTime = 0;
                runningProcess->finishTime = runningProcess->waitTime +
                                             runningProcess->burstTime +
                                             runningProcess->arrivalTime;
                runningProcess->turnaroundTime =
                    runningProcess->finishTime - runningProcess->arrivalTime;
                runningProcess->weightedTurnaroundTime =
                    runningProcess->turnaroundTime /
                    (float)runningProcess->burstTime;
                totalBurstTime += runningProcess->burstTime;
                totalWaitingTime += runningProcess->waitTime;
                totalTurnaroundTime += runningProcess->turnaroundTime;
                totalWeightedTurnaroundTime +=
                    runningProcess->weightedTurnaroundTime;
                totalWeightedTurnaroundTimeSquared +=
                    runningProcess->weightedTurnaroundTime *
                    runningProcess->weightedTurnaroundTime;

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

        // add all the new processes arrived at the current clk time
        while (true) {
            pcb* newProcess = getProcess();
            if (!newProcess) break;

            addPCBtoReadyQueue(&readyQueue, newProcess, algorithm);
            addPCBFront(&processTable, newProcess);
            countProcesses++;
            printf("[ENTERED]: id: %d %d\n", newProcess->id,
                   newProcess->remainingTime);
        }

        // the next process that should run
        pcb* nextProcess = popPriQueue(&readyQueue);

        int time = getClk();

        // update the remaining time for the current process
        if (runningProcess &&
            runningProcess->stoppedTime <= runningProcess->resumedTime) {
            int remainingTime = runningProcess->remainingTimeAfterStop -
                                (time - runningProcess->resumedTime);
            runningProcess->remainingTime =
                (remainingTime < 0) ? 0 : remainingTime;
        }

        // calculate the wait time for the process that will run next
        if (nextProcess && nextProcess != runningProcess) {
            int waitTime =
                time - nextProcess->arrivalTime -
                (nextProcess->burstTime - nextProcess->remainingTime);
            nextProcess->waitTime = (waitTime < 0) ? 0 : waitTime;
        }

        bool currentNotSameAsNext =
            runningProcess && nextProcess && runningProcess != nextProcess;

        if (algorithm == SRTN && currentNotSameAsNext &&
            runningProcess->remainingTime > nextProcess->remainingTime) {
            addPCBtoReadyQueue(&readyQueue, nextProcess, algorithm);
            continue;
        }

        // stop current process if it's not the next process or quantum
        // is over
        if ((algorithm == SRTN && currentNotSameAsNext) ||
            (algorithm == RR && quantumOver && currentNotSameAsNext)) {
            printf("[STOPPED]: id: %d %d %d\n", runningProcess->id, getClk(),
                   runningProcess->remainingTime);
            runningProcess->state = STOPPED;
            runningProcess->stoppedTime = getClk();
            runningProcess->remainingTimeAfterStop =
                runningProcess->remainingTime;
            kill(runningProcess->pid, SIGSTOP);
            outputPCBLogEntry(schedulerLogFile, runningProcess, getClk());
            runningProcess = NULL;
        }

        // ensure no processes are started when there is a running processes
        if (runningProcess) continue;

        // start the next process
        if (nextProcess && nextProcess->pid == -1) {
            printf("[STARTED]: id: %d %d %d\n", nextProcess->id, getClk(),
                   nextProcess->remainingTime);
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
            printf("[RESUMED]: id: %d %d %d\n", nextProcess->id, getClk(),
                   nextProcess->remainingTime);
            nextProcess->resumedTime = getClk();
            nextProcess->state = RESUMED;
            kill(nextProcess->pid, SIGCONT);
            outputPCBLogEntry(schedulerLogFile, nextProcess,
                              nextProcess->resumedTime);

            runningProcess = nextProcess;
        }

        if (!runningProcess && !nextProcess &&
            countProcesses == totalProcesses) {
            break;
        }
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
            (float)totalBurstTime / getClk() * 100);
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
    int rcv = msgrcv(SchedulerQueueID, &message, sizeof(message.process), 0,
                     IPC_NOWAIT);

    if (rcv == -1) return NULL;

    pcb* newProcess = malloc(sizeof(pcb));
    *newProcess = message.process;

    return newProcess;
}
