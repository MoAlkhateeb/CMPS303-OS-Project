#pragma once

#include <signal.h>
#include <stdio.h> //if you don't use scanf/printf change this include
#include <stdlib.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

#define SHKEY 300

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

static int getClk() { return *shmaddr; }

/*
 * All process call this function at the beginning to establish communication
 * between them and the clock module. Again, remember that the clock is only
 * emulation!
 */
static void initClk() {
  int shmid = shmget(SHKEY, 4, 0444);
  while ((int)shmid == -1) {
    // Make sure that the clock exists
    printf("Wait! The clock not initialized yet!\n");
    sleep(1);
    shmid = shmget(SHKEY, 4, 0444);
  }
  shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of
 * simulation. It terminates the whole system and releases resources.
 */

static void destroyClk(bool terminateAll) {
  shmdt(shmaddr);
  if (terminateAll) {
    killpg(getpgrp(), SIGINT);
  }
}

enum state{
    STARTED,
    RESUMED,
    STOPPED,
    FINISHED
};

typedef struct pcb{
    int id;
    int priority;
    int burstTime;
    int arrivalTime;
    enum state state;

    int pid;
    int remainingTime;

    int waitTime;
    int finishTime;
    int turnaroundTime;
    float weightedTurnarounTime;
} pcb;

typedef struct processMessage {
    long type;
    long request_id;
    pcb process;
} processMessage;


enum schedulingAlgorithm
{
    HPF = 1,
    SRTN,
    RR
};
