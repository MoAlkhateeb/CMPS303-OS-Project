#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "headers.h"

typedef struct listNode {
    pcb* pcb;
    struct listNode* next;
} node;

node* createList();
void addFront(node** head, pcb process);
pcb* getFront(node* head);
void deleteNode(node* head, int id);
void printList(node* head);
node* initNode(pcb* i);
void destroy(node** headptr);
