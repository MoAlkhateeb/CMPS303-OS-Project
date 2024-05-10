#include "pri_queue.h"

PriQueue *createPriQueue() {
    PriQueue *queue = (PriQueue *)malloc(sizeof(PriQueue));
    queue->size = 0;
    queue->head = NULL;
    return queue;
}

void destroyPriQueue(PriQueue **queue) {
    PriQueueNode *head = (*queue)->head;

    while (head) {
        PriQueueNode *next = head->next;
        free(head);
        head = next;
    }

    free(*queue);
}

int isEmptyPriQueue(PriQueue **queue) { return (*queue)->size == 0; }

void insertPriQueue(PriQueue **queue, int priority, pcb *pcb) {
    // creating node
    PriQueueNode *node = (PriQueueNode *)malloc(sizeof(PriQueueNode));
    node->priority = priority;
    node->pcb = pcb;

    // incrementing size
    (*queue)->size++;

    PriQueueNode *current = (*queue)->head;

    // if the queue is empty
    if (!current) {
        node->next = NULL;
        (*queue)->head = node;
        return;
    }

    // traverse to correct position
    PriQueueNode *prev = NULL;
    while (current && current->priority <= priority) {
        prev = current;
        current = current->next;
    }

    // insert the node
    node->next = current;
    if (prev) {
        prev->next = node;
    } else {
        (*queue)->head = node;
    }
}

pcb *popPriQueue(PriQueue **queue) {
    // if the queue is empty return NULL
    if (isEmptyPriQueue(queue)) {
        return NULL;
    }

    // get head node and pcb
    PriQueueNode *node = (*queue)->head;
    pcb *pcb = node->pcb;

    // update head
    (*queue)->head = node->next;

    // free popped node
    free(node);

    // decrement size
    (*queue)->size--;

    return pcb;
}

pcb *peekPriQueue(PriQueue **queue) {
    if (isEmptyPriQueue(queue)) {
        return NULL;
    }

    return (*queue)->head->pcb;
}

void printPriQueue(PriQueue **queue) {
    PriQueueNode *head = (*queue)->head;
    printf("\nQueue Size: %d\n", (*queue)->size);
    while (head) {
        printf("PCB: %d - Priority: %d\n", head->pcb->id, head->priority);
        head = head->next;
    }
    printf("\n");
}
