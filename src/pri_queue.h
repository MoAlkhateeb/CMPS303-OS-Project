#include <stdio.h>
#include <stdlib.h>

typedef struct PCB {
    int pid;
} PCB;

typedef struct PriQueueNode {
    int priority;
    PCB *pcb;
    struct PriQueueNode* next;

} PriQueueNode;

typedef struct PriQueue {
    PriQueueNode *head;
    PriQueueNode *tail;
    int size;
} PriQueue;


PriQueue *create_pri_queue();
void destroy_pri_queue(PriQueue *queue);
void insert_pri_queue(PriQueue *queue, int priority, struct PCB *pcb);
PCB *pop_pri_queue(PriQueue *queue);
int is_empty_pri_queue(PriQueue *queue);
void print_pri_queue(PriQueue* queue);
