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
bool insertProcess(mem_block *block, pcb *process, int closest_size);
bool removeProcess(buddy *b, pcb *process);

mem_block *findBlockBySize(mem_block *block, int size);
mem_block *findBlockByProcess(mem_block *block, pcb *process);
void combineMemoryBlocks(mem_block *block);

void printBuddy(mem_block *block, int depth, const char *prefix);
void freeBuddy(buddy *b);
bool divideBlock(mem_block *block);
mem_block *createBlock(int size, int start);