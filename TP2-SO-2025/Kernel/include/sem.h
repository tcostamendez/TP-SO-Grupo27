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

/**
 * @brief Initialize the global semaphore registry.
 * @return 0 on success, -1 on error.
 */
int initSemQueue(void);

/**
 * @brief Free all resources held by the semaphore registry.
 */
void freeSemQueue(void);

/**
 * @brief Open or create a named semaphore.
 * @param name Null-terminated semaphore name.
 * @param value Initial value (if created).
 * @return Handle to the semaphore, or NULL on error.
 */
Sem semOpen(const char *name, uint16_t value);

/**
 * @brief Close a semaphore handle.
 * @param sem Semaphore handle.
 * @return 0 on success, -1 on error.
 */
int semClose(Sem sem);

/**
 * @brief Post (increment) a semaphore.
 * @param sem Semaphore handle.
 * @return 0 on success, -1 on error.
 */
int semPost(Sem sem);

/**
 * @brief Wait (decrement) a semaphore, blocking if necessary.
 * @param sem Semaphore handle.
 * @return 0 on success, -1 on error.
 */
int semWait(Sem sem);

/**
 * @brief Get the current value of a semaphore.
 * @param sem Semaphore handle.
 * @return Current value, or -1 on error.
 */
int semGetValue(Sem sem);

/**
 * @brief Remove a process from a semaphore's blocked queue.
 * @param s Semaphore handle.
 * @param pid Process id to remove.
 * @return 0 on success, -1 on error.
 */
int removeFromSemaphore(Sem s, int pid);

/**
 * @brief Get the number of processes blocked on a semaphore.
 * @param sem Semaphore handle.
 * @return Count of blocked processes.
 */
int semGetBlockedProcessesCount(Sem sem);

/**
 * @brief Get the number of users holding/opening a semaphore.
 * @param sem Semaphore handle.
 * @return Count of users.
 */
int semGetUsersCount(Sem sem);

#endif