#include "buddy.h"

#include <math.h>

mem_block *createBlock(int size, int start) {
    mem_block *block = (mem_block *)malloc(sizeof(mem_block));
    block->size = size;
    block->process = NULL;
    block->isFree = true;
    block->start = start;
    block->end = start + size;
    block->left = NULL;
    block->right = NULL;
    return block;
}

buddy *createBuddy(int size) {
    buddy *b = (buddy *)malloc(sizeof(buddy));
    b->size = size;
    b->root = createBlock(size, 0);
    return b;
}

void printBuddy(mem_block *block, int depth, const char *prefix) {
    if (block == NULL) return;

    printf("%*s%s%d [%d][%d-%d]\n", depth * 4, "", prefix, block->size,
           block->isFree, block->start, block->end);

    if (block->left != NULL || block->right != NULL) {
        printBuddy(block->right, depth + 1, "└──R:");
        printBuddy(block->left, depth + 1, "└──L:");
    }
}

int getClosestPowerOfTwo(int size) { return (int)pow(2, ceil(log2(size))); }

bool divideBlock(mem_block *block) {
    if (block->size == 1 || block->left != NULL || block->right != NULL) {
        return false;
    }
    block->left = createBlock(block->size / 2, block->start);
    block->right = createBlock(block->size / 2, block->start + block->size / 2);
    return true;
}

bool insertBuddy(buddy *b, pcb *process) {
    int closest_size = getClosestPowerOfTwo(process->memsize);
    mem_block *closest = findBlockBySize(b->root, closest_size);
    if (closest) {
        closest->process = process;
        closest->isFree = false;
        return true;
    }
    return insertProcess(b->root, process, closest_size);
}

bool insertProcess(mem_block *block, pcb *process, int closest_size) {
    if (!block) return false;
    if (block->size < process->memsize) return false;
    if (!block->isFree) return false;

    if (!block->left && !block->right && block->size == closest_size) {
        block->process = process;
        block->isFree = false;
        return true;
    }

    if (!block->left && !block->right && block->size > process->memsize) {
        if (!divideBlock(block)) return false;
        return insertProcess(block->left, process, closest_size);
    } else {
        if (insertProcess(block->left, process, closest_size)) return true;
        return insertProcess(block->right, process, closest_size);
    }
    return false;
}

mem_block *findBlockBySize(mem_block *block, int size) {
    if (!block) return NULL;

    if (!block->left && !block->right) {
        if (block->size == size && block->isFree) {
            return block;
        }
    }

    mem_block *leftFind = findBlockBySize(block->left, size);
    if (leftFind) return leftFind;

    return findBlockBySize(block->right, size);
}

mem_block *findBlockByProcess(mem_block *block, pcb *process) {
    if (!block) return NULL;

    if (block->process == process) {
        return block;
    }

    mem_block *leftFind = findBlockByProcess(block->left, process);
    if (leftFind) return leftFind;

    return findBlockByProcess(block->right, process);
}

void combineMemoryBlocks(mem_block *block) {
    // post-order traversal to combine memory blocks
    if (!block) return;

    combineMemoryBlocks(block->left);
    combineMemoryBlocks(block->right);

    if (!block->left && !block->right) return;

    if (block->left->isFree && block->right->isFree) {
        if (block->left->left || block->right->left) return;
        free(block->left);
        free(block->right);
        block->left = NULL;
        block->right = NULL;
    }
}

bool removeProcess(buddy *b, pcb *process) {
    mem_block *block = findBlockByProcess(b->root, process);
    if (!block) return false;

    block->process = NULL;
    block->isFree = true;

    combineMemoryBlocks(b->root);

    return true;
}