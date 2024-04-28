#include "process_table.h"

#include <stdio.h>
#include <stdlib.h>

node *createNode(pcb *data) {
    node *newnode = malloc(sizeof(node));
    if (!newnode) {
        return NULL;
    }
    newnode->pcb = data;
    newnode->next = NULL;
    return newnode;
}

ProcessTable *createProcessTable() {
    ProcessTable *list = malloc(sizeof(ProcessTable));
    if (!list) {
        return NULL;
    }
    list->head = NULL;
    return list;
}

void printProcessTable(ProcessTable *list) {
    node *current = list->head;
    if (list->head == NULL) return;

    for (; current != NULL; current = current->next) {
        printf("%d ", current->pcb->id);
    }
    printf("\n");
}

void addPCBFront(ProcessTable **list, pcb *pcb) {
    node *current = NULL;
    if ((*list)->head == NULL) {
        (*list)->head = createNode(pcb);
    } else {
        node *newnode = createNode(pcb);
        newnode->next = (*list)->head;
        (*list)->head = newnode;
    }
}

void deletePCB(ProcessTable **list, int id) {
    node *current = (*list)->head;
    node *previous = current;
    while (current != NULL) {
        if (current->pcb->id == id) {
            previous->next = current->next;
            if (current == (*list)->head) (*list)->head = current->next;
            free(current->pcb);
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
    free(*list);
}

void destroyProcessTable(ProcessTable *list) {
    node *current = list->head;
    node *next = current;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
}