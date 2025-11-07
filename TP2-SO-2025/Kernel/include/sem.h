#ifndef _SEM_H_
#define _SEM_H_

#include <stdint.h>
#include "queue.h"
#include "memory_manager.h"
#include "strings.h"
#include "panic.h"

#define MAX_SEM_LENGTH 32
typedef struct sem {
    char name[MAX_SEM_LENGTH];
    uint16_t value;
    uint8_t lock;
    uint16_t users;
    QueueADT blockedProcesses; 
} sem;

typedef struct sem* Sem;

typedef struct semQueue {
    uint8_t lock;
    QueueADT sems;
} semQueue;


int initSemQueue(void);

void freeSemQueue(void);

Sem semOpen(const char *name, uint16_t value);

int semClose(Sem sem);

int semPost(Sem sem);

int semWait(Sem sem);

int semGetValue(Sem sem);

int removeFromSemaphore(Sem s, int pid);

int semGetBlockedProcessesCount(Sem sem);

int semGetUsersCount(Sem sem);

#endif