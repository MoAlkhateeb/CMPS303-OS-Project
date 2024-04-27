#include <errno.h>
#include <stdio.h>
#include <sys/_types/_key_t.h>
#include <sys/_types/_ssize_t.h>
#include <sys/msg.h>
#include <sys/wait.h>

#include "headers.h"
#include "linkedlist.h"
#include "pri_queue.h"

#define outputFileName "scheduler.log"

void addPCBtoPriQueue(PriQueue* priQueue, pcb* pcb,
                      enum schedulingAlgorithm algorithm);
void printOutputStatus(char* filename, pcb* pcb);

int main(int argc, char* argv[]) {
    // argv[1] -> Scheduling Type
    // argv[2] -> ?Quantum

    enum schedulingAlgorithm algorithm = atoi(argv[1]);
    int quantum = atoi(argv[2]);

    initClk();
    key_t key = ftok("headers.h", 10);
    int msgq_id = msgget(key, 0666);
    if (msgq_id == -1) {
        perror("Error in getting message queue");
        exit(-1);
    }
    printf("SCHEDULER: Created message queue\n");

    node* processTable = createList();
    PriQueue* readyQueue = createPriQueue();

    processMessage message;

    pcb* runningProcess = NULL;
    int startTime, endTime;
    while (true) {
        int rcv =
            msgrcv(msgq_id, &message, sizeof(message.process), 0, IPC_NOWAIT);

        if (rcv != -1) {
            printf("SCHEDULER: Received process %d, %d\n", message.process.id,
                   message.process.arrivalTime);

            addFront(&processTable, message.process);
            addPCBtoPriQueue(readyQueue, processTable->pcb, algorithm);
        } else if (errno == ENOMSG) {
        } else {
            fprintf(stderr, "Error in receive message\n");
            exit(-1);
        }

        pcb* nextProcess = popPriQueue(readyQueue);
        if (nextProcess) printf("Next Process: %d\n", nextProcess->id);

        // TODO: What if next process is null but there is a running
        // processes
        if (nextProcess) {
            // printf("Updating wait time for process %d\n", nextProcess->id);
            nextProcess->waitTime =
                getClk() - nextProcess->arrivalTime -
                (nextProcess->burstTime - nextProcess->remainingTime);
        }
        if (runningProcess) {
            // printf("Updating remaining time for process %d\n",
            // runningProcess->id);
            runningProcess->remainingTime =
                runningProcess->remainingTime - (getClk() - startTime);
        }

        // stop previous process
        // Stop during round robbin or if the current process is not the next
        // process
        if ((runningProcess && nextProcess && runningProcess != nextProcess) ||
            (algorithm == RR && getClk() - startTime >= quantum)) {
            printf("Stopping process %d\n", runningProcess->id);
            runningProcess->state = STOPPED;
            kill(runningProcess->pid, SIGTSTP);
            printOutputStatus(outputFileName, runningProcess);
        }

        // start current process
        if (nextProcess && nextProcess->pid == -1) {
            printOutputStatus(outputFileName, nextProcess);

            int pid = fork();
            nextProcess->pid = pid;

            char remainingTime[10];
            sprintf(remainingTime, "%d", nextProcess->remainingTime);

            if (pid == 0) {
                char* args[] = {"./process.out", remainingTime, NULL};
                execv(args[0], args);
            }
            startTime = getClk();

        } else if (nextProcess) {
            nextProcess->state = RESUMED;
            printOutputStatus(outputFileName, nextProcess);
            kill(nextProcess->pid, SIGCONT);
            startTime = getClk();
        }

        runningProcess = nextProcess;

        if (runningProcess) {
            int Stat;
            int wpid = waitpid(runningProcess->pid, &Stat, WNOHANG);
            if (WEXITSTATUS(Stat) == 1) {
                printf("Process %d finished\n", runningProcess->id);
                runningProcess->state = FINISHED;
                runningProcess->finishTime = getClk();
                runningProcess->turnaroundTime =
                    runningProcess->finishTime - runningProcess->arrivalTime;
                runningProcess->weightedTurnarounTime =
                    runningProcess->turnaroundTime /
                    (float)runningProcess->burstTime;
                // deleteNode(processTable, runningProcess->id);
                printOutputStatus(outputFileName, runningProcess);

            } else {
                addPCBtoPriQueue(readyQueue, runningProcess, algorithm);
            }
        }
    }

    destroyClk(true);
}

void addPCBtoPriQueue(PriQueue* priQueue, pcb* pcb,
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
}
