#include "headers.h"
#include "linkedlist.h"

enum state{
    STARTED,
    RESUMED,
    STOPPED,
    FINISHED
};

struct pcb{
    int id;
    pid_t pid;
    enum state State;
    int arrivalTime;
    int waitTime;
    int finishTime;
    int remainingTime;
    int burstTime;
    int priority;
    int turnaroundTime;
    float weightedTurnarounTime;
};

void addPCB(node * head,int id, int arrivalTime, int priority, int runtime){

}