#ifndef _SEM_H_
#define _SEM_H_

#include <stdint.h>
#include "queue.h"

#define MAX_SEM_LENGTH 32

typedef struct sem sem;
typedef sem* Sem;

int initSemQueue(void);
Sem semOpen(const char *name, uint16_t value);
int semClose(Sem sem);
void freeSemQueue(void);
int semPost(Sem sem);
int semWait(Sem sem);
int semGetValue(Sem sem);
int semGetUsersCount(Sem sem);
int semGetBlockedProcessesCount(Sem sem);
int removeFromSemaphore(Sem s, int pid);

#endif