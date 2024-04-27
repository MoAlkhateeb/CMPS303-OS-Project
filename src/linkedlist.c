#include "linkedlist.h"

node *createList() {
    node *dummyNode;
    dummyNode = malloc(sizeof(node));
    dummyNode->pcb = NULL;

    return dummyNode;
}

void addFront(node **head, pcb process) {
    if (*head == NULL) {
        fprintf(stderr, "Error: Invalid pointer\n");
        exit(EXIT_FAILURE);
    }

    pcb *newPCB = malloc(sizeof(pcb));
    *newPCB = process;

    if ((*head)->pcb == NULL) {
        (*head)->pcb = newPCB;
        return;
    }

    node *newNode = initNode(newPCB);
    newNode->next = (*head);
    (*head) = newNode;
}

node *initNode(pcb *i) {
    node *newNode;
    newNode = malloc(sizeof(node));
    newNode->pcb = i;
    newNode->next = NULL;

    return newNode;
}

pcb *getFront(node *head) {
    if (head == NULL || head->next == NULL) {
        fprintf(stderr, "Error: List is empty\n");
        exit(EXIT_FAILURE);
    }
    return head->next->pcb;
}

void deleteNode(node *head, int id) {
    if (head == NULL) {
        fprintf(stderr, "Error: List is empty\n");
        exit(EXIT_FAILURE);
    }
    node *current = head->next;
    node *prev = head;
    if (current->pcb->id == id && current->next == NULL) {
        free(current->pcb);
        free(current);
        head->next = NULL;  // Update head to point to NULL
        return;
    }

    while (current != NULL) {
        if (current->pcb->id == id) {
            prev->next = current->next;
            free(current->pcb);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
    fprintf(stderr, "Error: Node with id %d not found\n", id);
}

void printList(node *head) {
    node *N;
    N = head;
    while (N != NULL) {
        printf("%d ", N->pcb->id);
        N = N->next;
    }
}

void destroy(node **headptr) {
    if (*headptr == NULL) {
        return;  // No need to destroy an already destroyed list
    }
    node *current = *headptr;
    node *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *headptr = NULL;  // Set head pointer to NULL after destruction
}