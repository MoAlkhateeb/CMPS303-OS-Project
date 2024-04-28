#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "headers.h"

typedef struct PriQueueNode {
    int priority;
    pcb *pcb;
    struct PriQueueNode *next;

} PriQueueNode;

typedef struct PriQueue {
    PriQueueNode *head;
    PriQueueNode *tail;
    int size;
} PriQueue;

PriQueue *createPriQueue();
void destroyPriQueue(PriQueue **queue);
void insertPriQueue(PriQueue **queue, int priority, pcb *pcb);
pcb *popPriQueue(PriQueue **queue);
int isEmptyPriQueue(PriQueue **queue);
void printPriQueue(PriQueue **queue);
