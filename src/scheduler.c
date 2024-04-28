#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/wait.h>

#include "headers.h"
#include "pri_queue.h"
#include "process_table.h"

#define outputFileName "scheduler.log"

void addPCBtoPriQueue(PriQueue** priQueue, pcb* pcb,
                      enum schedulingAlgorithm algorithm);
void printOutputStatus(char* filename, pcb* pcb);
pcb* getProcess();

int SchedulerQueueID = -1;

int main(int argc, char* argv[]) {
    enum schedulingAlgorithm algorithm = atoi(argv[1]);
    int quantum = atoi(argv[2]);

    key_t key = ftok("headers.h", 10);
    SchedulerQueueID = msgget(key, 0666);
    if (SchedulerQueueID == -1) {
        perror("Error in getting message queue");
        exit(-1);
    }
    ProcessTable* processTable = createProcessTable();
    PriQueue* readyQueue = createPriQueue();

    initClk();

    int startTime;
    pcb* runningProcess = NULL;

    int prevTime = -1;

    while (true) {
        while (true) {
            pcb* newProcess = getProcess();
            if (!newProcess) break;

            addPCBtoPriQueue(&readyQueue, newProcess, algorithm);
            addPCBFront(&processTable, newProcess);
            printf("[ENTERED]: id: %d\n", newProcess->id);
        }

        // get highest priority process
        pcb* nextProcess = popPriQueue(&readyQueue);

        // update wait time for process next in line
        if (nextProcess) {
            int waitTime =
                getClk() - nextProcess->arrivalTime -
                (nextProcess->burstTime - nextProcess->remainingTime);
            nextProcess->waitTime = (waitTime < 0) ? 0 : waitTime;
        }

        // update remaining time for running process
        if (runningProcess) {
            int remainingTime =
                runningProcess->remainingTime - (getClk() - startTime);
            runningProcess->remainingTime =
                (remainingTime < 0) ? 0 : remainingTime;
        }

        // stop current process if it's not the next process or quantum is over
        if (algorithm != HPF) {
            if ((runningProcess && nextProcess &&
                 runningProcess != nextProcess) ||
                (algorithm == RR && getClk() - startTime >= quantum)) {
                runningProcess->state = STOPPED;
                kill(runningProcess->pid, SIGTSTP);
                printOutputStatus(outputFileName, runningProcess);
            }
        }

        // star the next process
        if (nextProcess && nextProcess->pid == -1) {
            printf("[STARTED]: id: %d\n", nextProcess->id);
            int pid = fork();

            char remainingTime[10];
            sprintf(remainingTime, "%d", nextProcess->remainingTime);

            startTime = getClk();
            if (pid == 0) {
                char* args[] = {"./process.out", remainingTime, NULL};
                execv(args[0], args);
            }

            nextProcess->pid = pid;
            printOutputStatus(outputFileName, nextProcess);

            runningProcess = nextProcess;

        } else if (nextProcess && nextProcess->state == STOPPED) {
            nextProcess->state = RESUMED;
            kill(nextProcess->pid, SIGCONT);
            printOutputStatus(outputFileName, nextProcess);
            startTime = getClk();

            runningProcess = nextProcess;
        }

        if (runningProcess) {
            int Stat = INT_MAX;
            int wpid;

            if (algorithm == HPF)
                wpid = waitpid(runningProcess->pid, &Stat, !WNOHANG);
            else
                wpid = waitpid(runningProcess->pid, &Stat, WNOHANG);

            if (WEXITSTATUS(Stat) == 0) {
                printf("[FINISHED]: id: %d\n", runningProcess->id);
                runningProcess->state = FINISHED;
                runningProcess->remainingTime = 0;
                runningProcess->finishTime = getClk();
                runningProcess->turnaroundTime =
                    runningProcess->finishTime - runningProcess->arrivalTime;
                runningProcess->weightedTurnarounTime =
                    runningProcess->turnaroundTime /
                    (float)runningProcess->burstTime;
                printOutputStatus(outputFileName, runningProcess);
                deletePCB(&processTable, runningProcess->id);
                runningProcess = NULL;

            } else {
                addPCBtoPriQueue(&readyQueue, runningProcess, algorithm);
            }
        }
    }
    destroyClk(true);
    return 0;
}

void addPCBtoPriQueue(PriQueue** priQueue, pcb* pcb,
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

void printOutputStatus(char* filename, pcb* pcb) {
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
            getClk(), pcb->id, state, pcb->arrivalTime, pcb->burstTime,
            pcb->remainingTime, pcb->waitTime);

    if (pcb->state == FINISHED)
        fprintf(file, "TA %d WTA %.2f\n", pcb->turnaroundTime,
                pcb->weightedTurnarounTime);
    else
        fprintf(file, "\n");

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