#include "pri_queue.h"

PriQueue *createPriQueue() {
    PriQueue *queue = (PriQueue *)malloc(sizeof(PriQueue));
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
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

    PriQueueNode *head = (*queue)->head;

    // if the queue is empty
    if (!head) {
        node->next = head;
        (*queue)->head = node;
        (*queue)->tail = node;
        return;
    }

    // if there is only one element in the queue
    if (head == (*queue)->tail && head->priority > priority) {
        (*queue)->head = node;
        node->next = head;
        (*queue)->tail = head;
        return;
    }

    // traverse to correct position
    while (head && head->priority <= priority) {
        head = head->next;
    }

    // insert the node
    node->next = head;
    (*queue)->head = node;

    // if node is at the end of the queue update tail
    if (!node->next) {
        (*queue)->tail = node;
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

void printPriQueue(PriQueue **queue) {
    PriQueueNode *head = (*queue)->head;
    printf("\nQueue Size: %d\n", (*queue)->size);
    while (head) {
        printf("PCB: %d - Priority: %d\n", head->pcb->id, head->priority);
        head = head->next;
    }
    printf("\n");
}