#include "sem.h"
#include "interrupts.h"
#include "panic.h"
#include "process.h"
#include "scheduler.h"
#include "strings.h"
#include "queue.h"
#include "memory_manager.h"
#include "sem.h"

typedef struct sem {
    char name[MAX_SEM_LENGTH];
    uint16_t value;
    uint8_t lock;
    uint16_t users;
    QueueADT blockedProcesses; 
} sem;

typedef struct semQueue {
    uint8_t lock;
    QueueADT sems;
} semQueue;

static semQueue *sQueue = NULL;

extern void semLock(uint8_t *lock);
extern void semUnlock(uint8_t *lock);

static int cmpPid(void *pid1, void *pid2) { return *((int *)pid1) - *((int *)pid2); }
int cmpSem(void *psem1, void *psem2) { return *((Sem *)psem1) - *((Sem *)psem2); }

static sem * findSemByName(const char *name) {
    if (!sQueue || queueIsEmpty(sQueue->sems)) return NULL;
    sem *current = NULL;
    int size = queueSize(sQueue->sems);
    queueBeginCyclicIter(sQueue->sems);
    for (int i = 0; i < size; i++) {
        queueNextCyclicIter(sQueue->sems, &current);
        if (current && my_strcmp(current->name, name) == 0) return current;
    }
    return NULL;
}

static int validSem(Sem semToValid) {
    if (semToValid == NULL || sQueue == NULL) return 0;
    Sem aux = semToValid;
    semLock(&sQueue->lock);
    int exists = queueElementExists(sQueue->sems, &aux);
    semUnlock(&sQueue->lock);
    return exists;
}

int initSemQueue(void) {
    if (sQueue != NULL) {
        panic("Semaphore queue already initialized");
    }
    sQueue = mm_alloc(sizeof(semQueue));
    if (sQueue == NULL) {
        panic("Failed to allocate memory for semaphore queue");
        return -1;
    }
    sQueue->lock = 0;
    sQueue->sems = createQueue(cmpSem, sizeof(struct sem *));
    return sQueue->sems == NULL ? -1 : 1;
}

Sem semOpen(const char *name, uint16_t value) {
    if (name == NULL || strlen(name) >= MAX_SEM_LENGTH) return NULL;
    semLock(&sQueue->lock);

    sem *found = findSemByName(name);
    if (found) {
        semLock(&found->lock);
        found->users++;
        semUnlock(&found->lock);
        semUnlock(&sQueue->lock);
        return found;
    }

    sem *newSem = mm_alloc(sizeof(sem));
    if (newSem == NULL) {
        semUnlock(&sQueue->lock);
        return NULL;
    }

    my_strcpy(newSem->name, name);
    newSem->value = value;
    newSem->lock = 0;
    newSem->users = 1;
    newSem->blockedProcesses = createQueue(cmpPid, sizeof(int));
    if (newSem->blockedProcesses == NULL) {
        mm_free(newSem);
        semUnlock(&sQueue->lock);
        return NULL;
    }

    enqueue(sQueue->sems, &newSem);
    semUnlock(&sQueue->lock);
    return newSem;
}

int semClose(Sem semToClose) {
    if (!validSem(semToClose)) return -1;
    semLock(&semToClose->lock);
    semToClose->users--;
    sem *aux = semToClose;
    semLock(&sQueue->lock);
    if (semToClose->users != 0) {
        semUnlock(&semToClose->lock);
        semUnlock(&sQueue->lock);
        return 0;
    } else {
        if (queueRemove(sQueue->sems, &aux) == NULL) {
            semUnlock(&sQueue->lock);
            semUnlock(&semToClose->lock);
            return -1;
        }
        queueFree(semToClose->blockedProcesses);
        mm_free(semToClose);
        semUnlock(&sQueue->lock);
        return 0;
    }
}

void freeSemQueue(void) {
    if (sQueue == NULL) return;
    if (sQueue->sems) queueFree(sQueue->sems);
    mm_free(sQueue);
    sQueue = NULL;
}

int semPost(Sem semToPost) {
    if (!validSem(semToPost)) return -1;
    
    print("[sem] semPost on '");
    print(semToPost->name);
    print("'\n");
    
    semLock(&semToPost->lock);
    if (queueSize(semToPost->blockedProcesses) != 0) {
        int pid;
        if (dequeue(semToPost->blockedProcesses, &pid) != NULL) {
            print("[sem] waking up pid=");
            printDec(pid);
            print("\n");
            Process *p = get_process(pid);
            if (p != NULL) {
                unblock_process(p);
            } else {
                print("[sem] WARNING: process not found!\n");
            }
        }
    } else {
        semToPost->value++;
    }
    semUnlock(&semToPost->lock);
    return 0;
}

int semWait(Sem semToWait) {
    if (!validSem(semToWait)) return -1;
    
    print("[sem] semWait on '");
    print(semToWait->name);
    print("'\n");
    
    int blocked = 0;
    semLock(&semToWait->lock);
    if (semToWait->value == 0) {
        Process *cur = get_current_process();
        if (!cur) {
            semUnlock(&semToWait->lock);
            return -1;
        }
        int pid = cur->pid;
        print("[sem] blocking pid=");
        printDec(pid);
        print("\n");
        
        if (enqueue(semToWait->blockedProcesses, &pid) == NULL) {
            semUnlock(&semToWait->lock);
            return -1;
        }
        block_process(cur);
        blocked = 1;
    } else {
        semToWait->value--;
    }
    semUnlock(&semToWait->lock);
    if (blocked) {
        print("[sem] forcing scheduler interrupt\n");
        _force_scheduler_interrupt();
    }
    return 0;
}

int semGetValue(Sem semToGet) {
    if (!validSem(semToGet)) return -1;
    int val;
    semLock(&semToGet->lock);
    val = semToGet->value;
    semUnlock(&semToGet->lock);
    return val;
}

int semGetUsersCount(Sem semToGet) {
    if (!validSem(semToGet)) return -1;
    int u;
    semLock(&semToGet->lock);
    u = semToGet->users;
    semUnlock(&semToGet->lock);
    return u;
}

int semGetBlockedProcessesCount(Sem semToGet) {
    if (!validSem(semToGet)) return -1;
    int s;
    semLock(&semToGet->lock);
    s = queueSize(semToGet->blockedProcesses);
    semUnlock(&semToGet->lock);
    return s;
}

int removeFromSemaphore(Sem s, int pid) {
    if (s == NULL) return -1;
    return queueRemove(s->blockedProcesses, &pid) == NULL ? -1 : 1;
}