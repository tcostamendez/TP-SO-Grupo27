#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

#include "list.h"

#define PROCESS_STACK_SIZE 4096

#define PROCESS_STATE_READY 0
#define PROCESS_STATE_RUNNING 1
#define PROCESS_STATE_BLOCKED 2

typedef struct Process {
    int pid;
    int priority;
    int state;
    uint8_t * stack;
} Process;

#endif