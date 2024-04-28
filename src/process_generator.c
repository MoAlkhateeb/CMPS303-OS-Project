#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include "headers.h"
#include "process_table.h"

void clearResources();

struct processData {
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
};

int SchedulerQueueID = -1;

int main(int argc, char *argv[]) {
    signal(SIGINT, clearResources);

    // 1. Read the algorithm files.
    FILE *file = fopen("processes.txt", "r");
    if (!file) {
        perror("Error in open file");
        exit(1);
    }

    char line[100];
    int line_count = 0;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] != '#') {
            line_count = line_count + 1;
        }
    }
    fseek(file, 0, SEEK_SET);

    struct processData *p = malloc(sizeof(struct processData) * line_count);
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') {
            continue;
        }
        int id, arrival, runtime, priority;
        sscanf(line, "%d\t%d\t%d\t%d", &id, &arrival, &runtime, &priority);
        p[i].arrivaltime = arrival;
        p[i].id = id;
        p[i].runningtime = runtime;
        p[i].priority = priority;
        i++;
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters,
    // if there are any.
    printf("Choose the scheduling algorithm\n");
    printf(" Highest priority first:              %d\n", HPF);
    printf(" shortest remaining time next:        %d\n", SRTN);
    printf(" Round Robin:                         %d\n", RR);

    int algorithm;
    scanf("%d", &algorithm);

    int quantum;
    if (algorithm == RR) {
        printf("Round robin quantum?\n");
        scanf("%d", &quantum);
    }

    char quantum_str[10], algorithm_str[10];
    sprintf(quantum_str, "%d", quantum);
    sprintf(algorithm_str, "%d", algorithm);

    key_t key = ftok("headers.h", 10);
    SchedulerQueueID = msgget(key, 0666 | IPC_CREAT);
    if (SchedulerQueueID == -1) {
        perror("Error in create");
        exit(-1);
    }

    processMessage message;
    message.type = 1;

    //  3. Initiate and create the scheduler and clock processes.
    int pid = fork();
    if (pid == 0) {
        char *args[] = {"./clk.out", NULL};
        execv(args[0], args);
    }

    pid = fork();
    if (pid == 0) {
        char *args[] = {"./scheduler.out", algorithm_str, quantum_str, NULL};
        execv(args[0], args);
    }

    //  4. Use this function after creating the clock process to initialize
    //  clock
    initClk();

    //  5. Create a data structure for processes and provide it with its
    //  parameters.

    //  6. Send the information to the scheduler at the appropriate time.
    int time;
    for (int j = 0; j < i; j++) {
        if (getClk() < p[j].arrivaltime) {
            sleep(p[j].arrivaltime - getClk());
        }

        pcb process;
        process.pid = -1;
        process.id = p[j].id;
        process.arrivalTime = p[j].arrivaltime;
        process.priority = p[j].priority;
        process.burstTime = p[j].runningtime;
        process.state = STARTED;

        // constantly need to be updated
        process.remainingTime = p[j].runningtime;
        process.waitTime = 0;

        process.finishTime = -1;
        process.turnaroundTime = -1;
        process.weightedTurnarounTime = -1;

        message.process = process;

        int snd = msgsnd(SchedulerQueueID, &message, sizeof(message.process),
                         !IPC_NOWAIT);
        if (snd == -1) {
            perror("Error in send");
            exit(1);
        } else {
            printf("Process %d sent\n", message.process.id);
        }
    }

    //  7. Clear clock resources

    // wait for all children to finish before clearing resources
    wait(NULL);
    clearResources();
}

void clearResources() {
    if (SchedulerQueueID != -1) {
        msgctl(SchedulerQueueID, IPC_RMID, NULL);
    }
    destroyClk(true);
    exit(0);
}
