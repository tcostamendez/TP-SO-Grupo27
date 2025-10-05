#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

#include "list.h"

#define PROCESS_STACK_SIZE 4096// no se de donde sale esto, capaz estaria bueo preguntarlo

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} PROC_STATE;




typedef struct Process{
    int pid;
    PROC_STATE state;
    int ppid;
    char * name;
    uint64_t pc;
    uint64_t rsp;
    uint8_t * stack;
    int priority;
} Process;

void scheduler_init();

int queue_proc(Process * parentProcess);

uint64_t schedule(uint64_t current_rsp);



#endif