#pragma once

#include "headers.h"

typedef struct mem_block {
    int size;
    pcb *process;
    bool isFree;
    int start;
    int end;
    struct mem_block *left;
    struct mem_block *right;
} mem_block;

typedef struct buddy {
    int size;
    mem_block *root;
} buddy;

int getClosestPowerOfTwo(int size);

buddy *createBuddy(int size);

bool insertBuddy(buddy *b, pcb *process, int time);
bool insertProcess(mem_block *block, pcb *process, int closest_size, int time);

bool removeProcess(buddy *b, pcb *process);
void combineMemoryBlocks(mem_block *block);

mem_block *findBlockBySize(mem_block *block, int size);
mem_block *findBlockByProcess(mem_block *block, pcb *process);

void printBuddy(mem_block *block, int depth, const char *prefix);
bool divideBlock(mem_block *block);

mem_block *createBlock(int size, int start);
void freeBuddy(buddy *b);