#ifndef _SEM_H_
#define _SEM_H_

#include <stdint.h>
#include "queue.h"

#define MAX_SEM_LENGTH 32

typedef struct sem sem;
typedef sem* Sem;

int init_sem_queue(void);
Sem sem_open(const char *name, uint16_t value);
int sem_close(Sem sem);
void free_sem_queue(void);
int sem_post(Sem sem);
int sem_wait(Sem sem);
int sem_get_value(Sem sem);
int sem_get_users_count(Sem sem);
int sem_get_blocked_processes_count(Sem sem);
int remove_from_semaphore(Sem s, int pid);

#endif