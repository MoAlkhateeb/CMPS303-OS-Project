#pragma once
#include "headers.h"

typedef struct node {
    pcb* pcb;
    struct node* next;
} node;

typedef struct ProcessTable {
    node* head;
} ProcessTable;

node* createNode(pcb* data);
ProcessTable* createProcessTable();
void addPCBFront(ProcessTable** list, pcb* pcb);
void deletePCB(ProcessTable** list, int id);
void printProcessTable(ProcessTable* list);
void destroyProcessTable(ProcessTable* list);
