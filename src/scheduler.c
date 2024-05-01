#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/wait.h>

#include "headers.h"
#include "pri_queue.h"
#include "process_table.h"

#define schedulerLogFile "scheduler.log"
#define schedulerPerfFile "scheduler.perf"

void addPCBtoReadyQueue(PriQueue** priQueue, pcb* pcb,
                        enum schedulingAlgorithm algorithm);
void outputPerfFile(char* filename, int totalWaitingTime,
                    int totalTurnaroundTime, int totalWeightedTurnaroundTime,
                    int totalProcesses);
void outputPCBLogEntry(char* filename, pcb* pcb, int time);
pcb* getProcess();

int SchedulerQueueID = -1;

int totalWaitingTime = 0;
int totalTurnaroundTime = 0;
int totalWeightedTurnaroundTime = 0;
int totalProcesses = 0;

int main(int argc, char* argv[]) {
    // get algorithm information from command line arguments
    if (argc < 2) {
        printf("Usage: %s <scheduling algorithm number> [<quantum>]\n",
               argv[0]);
        exit(1);
    }

    enum schedulingAlgorithm algorithm = atoi(argv[1]);

    if (algorithm == RR && argc < 3) {
        printf("Usage: %s <scheduling algorithm number> [<quantum>]\n",
               argv[0]);
        exit(1);
    }

    int quantum = atoi(argv[2]);

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

    // initialize the clock
    initClk();

    // holds a pointer to the current running process
    pcb* runningProcess = NULL;

    // main scheduling loop
    while (true) {
        // add all the new process arrived at the current clk time
        while (true) {
            pcb* newProcess = getProcess();
            if (!newProcess) break;

            addPCBtoReadyQueue(&readyQueue, newProcess, algorithm);
            addPCBFront(&processTable, newProcess);
            totalProcesses++;
            printf("[ENTERED]: id: %d\n", newProcess->id);
        }

        // check if the running process has finished
        if (runningProcess) {
            int status;

            // if non-premptive wait for the process to finish
            waitpid(runningProcess->pid, &status,
                    (algorithm == HPF) ? !WNOHANG : WNOHANG);

            // if the process has finished successfully update the pcb and log
            if (WIFEXITED(status)) {
                printf("[FINISHED]: id: %d\n", runningProcess->id);
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

                totalWaitingTime += runningProcess->waitTime;
                totalTurnaroundTime += runningProcess->turnaroundTime;
                totalWeightedTurnaroundTime +=
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

        // the next process that should run
        pcb* nextProcess = popPriQueue(&readyQueue);

        // calculate the wait time for the process that will run next
        if (nextProcess) {
            int waitTime =
                getClk() - nextProcess->arrivalTime -
                (nextProcess->burstTime - nextProcess->remainingTime);
            nextProcess->waitTime = (waitTime < 0) ? 0 : waitTime;
        }

        // stop current process if it's not the next process or quantum is over
        if ((algorithm != HPF) && (runningProcess && nextProcess &&
                                   runningProcess != nextProcess) ||
            (algorithm == RR &&
             getClk() - runningProcess->resumedTime >= quantum)) {
            int remainingTime = runningProcess->remainingTime -
                                (getClk() - runningProcess->startTime);
            runningProcess->remainingTime =
                (remainingTime < 0) ? 0 : remainingTime;
            runningProcess->state = STOPPED;
            runningProcess->stoppedTime = getClk();
            kill(runningProcess->pid, SIGTSTP);
            outputPCBLogEntry(schedulerLogFile, runningProcess, getClk());
        }

        if (runningProcess) continue;

        // start the next process
        if (nextProcess && nextProcess->pid == -1) {
            printf("[STARTED]: id: %d\n", nextProcess->id);
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
    }
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

void outputPerfFile(char* filename, int totalWaitingTime,
                    int totalTurnaroundTime, int totalWeightedTurnaroundTime,
                    int totalProcesses) {
    FILE* file = fopen(filename, "w");

    fprintf(file, "Avg WTA. = %.2f\n",
            totalWeightedTurnaroundTime / (float)totalProcesses);
    fprintf(file, "Avg Waiting = %.2f\n",
            totalWaitingTime / (float)totalProcesses);
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