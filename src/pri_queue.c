#include "pri_queue.h"

PriQueue *create_pri_queue() {
    PriQueue *queue = (PriQueue *)malloc(sizeof(PriQueue));
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void destroy_pri_queue(PriQueue *queue) {
    PriQueueNode *head = queue->head;

    while (head) {
        PriQueueNode *next = head->next;
        free(head);
        head = next;
    }

    free(queue);
}

int is_empty_pri_queue(PriQueue *queue) {
    return queue->size == 0;
}

void insert_pri_queue(PriQueue *queue, int priority, PCB *pcb) {

    // creating node
    PriQueueNode *node = (PriQueueNode *)malloc(sizeof(PriQueueNode));
    node->priority = priority;
    node->pcb = pcb;

    // incrementing size
    queue->size++;

    PriQueueNode* head = queue->head;

    // if the queue is empty
    if (!head) {
        node->next = head;
        queue->head = node;
        queue->tail = node;
        return;
    }

    // if there is only one element in the queue
    if (head == queue->tail && head->priority > priority) {
        queue->head = node;
        node->next = head;
        queue->tail = head;
        return;
    }

    // traverse to correct position
    while (head->next && head->next->priority <= priority) {
        head = head->next;
    }

    // inser the node
    node->next = head->next;
    head->next = node;

    // if node is at the end of the queue update tail
    if (!node->next) {
        queue->tail = node;
    }

}

PCB *pop_pri_queue(PriQueue *queue) {

    // if the queue is empty return NULL
    if (is_empty_pri_queue(queue)) {
        return NULL;
    }

    // get head node and pcb
    PriQueueNode *node = queue->head;
    PCB *pcb = node->pcb;

    // update head
    queue->head = node->next;

    // free popped node
    free(node);

    // decrement size
    queue->size--;

    return pcb;
}


void print_pri_queue(PriQueue* queue) {
    PriQueueNode* head = queue->head;
    while (head) {
        printf("PCB: %d - Priority: %d\n", head->pcb->pid, head->priority);
        head = head->next;
    }
}
