#include "sem.h"
#include "alloc.h"
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
    QueueADT blocked_processes; // queue of int (pid)
} sem;

typedef struct sem_queue {
    uint8_t lock;
    QueueADT sems;
} sem_queue;

static sem_queue *s_queue = NULL;

extern void sem_lock(uint8_t *lock);
extern void sem_unlock(uint8_t *lock);

extern void _force_scheduler_interrupt();

static int cmp_pid(void *pid1, void *pid2) { return *((int *)pid1) - *((int *)pid2); }
int cmp_sem(void *psem1, void *psem2) { return *((Sem *)psem1) - *((Sem *)psem2); }

static sem * find_sem_by_name(const char *name) {
    if (!s_queue || queueIsEmpty(s_queue->sems)) return NULL;
    sem *current = NULL;
    int size = queueSize(s_queue->sems);
    queueBeginCyclicIter(s_queue->sems);
    for (int i = 0; i < size; i++) {
        queueNextCyclicIter(s_queue->sems, &current);
        if (current && my_strcmp(current->name, name) == 0) return current;
    }
    return NULL;
}

static int valid_sem(Sem sem_to_valid) {
    if (sem_to_valid == NULL || s_queue == NULL) {
        return 0;
    }

    Sem aux = sem_to_valid;
    
    _cli();  // Deshabilitar interrupciones para evitar deadlock en spinlock
    sem_lock(&s_queue->lock);
    int exists = queueElementExists(s_queue->sems, &aux);
    sem_unlock(&s_queue->lock);
    _sti();  // Rehabilitar interrupciones

    return exists;
}

int init_sem_queue(void) {
    if (s_queue != NULL) {
        panic("Semaphore queue already initialized");
        return -1;
    }
    s_queue = mm_alloc(sizeof(sem_queue));
    if (s_queue == NULL) {
        panic("Failed to allocate memory for semaphore queue");
        return -1;
    }
    s_queue->lock = 0;
    s_queue->sems = createQueue(cmp_sem, sizeof(struct sem *));
    return s_queue->sems == NULL ? -1 : 0;
}

Sem sem_open(const char *name, uint16_t value) {
    if (name == NULL || strlen(name) >= MAX_SEM_LENGTH) {
        return NULL;
    }

    _cli();  // Deshabilitar interrupciones para evitar deadlock en spinlock
    sem_lock(&s_queue->lock);

    sem *found = find_sem_by_name(name);
    if (found) {
        sem_lock(&found->lock);
        found->users++;
        sem_unlock(&found->lock);
        sem_unlock(&s_queue->lock);
        _sti();  // Rehabilitar interrupciones
        return found;
    }

    sem *new_sem = mm_alloc(sizeof(sem));
    if (new_sem == NULL) {
        sem_unlock(&s_queue->lock);
        _sti();  // Rehabilitar interrupciones
        return NULL;
    }

    // copiar nombre
    my_strcpy(new_sem->name, name);
    new_sem->value = value;
    new_sem->lock = 0;
    new_sem->users = 1;
    new_sem->blocked_processes = createQueue(cmp_pid, sizeof(int));
    if (new_sem->blocked_processes == NULL) {
        mm_free(new_sem);
        sem_unlock(&s_queue->lock);
        _sti();  // Rehabilitar interrupciones
        return NULL;
    }

    enqueue(s_queue->sems, &new_sem);
    sem_unlock(&s_queue->lock);
    _sti();  // Rehabilitar interrupciones
    return new_sem;
}

int sem_close(Sem sem_to_close) {
    if (!valid_sem(sem_to_close)) {
        return -1;
    }

    sem_lock(&sem_to_close->lock);
    sem_to_close->users--;

    sem *aux = sem_to_close;
    
    _cli();  // Deshabilitar interrupciones para evitar deadlock en spinlock
    sem_lock(&s_queue->lock);

    if (sem_to_close->users != 0) {
        sem_unlock(&sem_to_close->lock);
        sem_unlock(&s_queue->lock);
        _sti();  // Rehabilitar interrupciones
        return 0;
    } else {
        // Remover de la cola global (ya no es visible para sem_open)
        if (queueRemove(s_queue->sems, &aux) == NULL) {
            sem_unlock(&sem_to_close->lock);
            sem_unlock(&s_queue->lock);
            _sti();  // Rehabilitar interrupciones
            return -1;
        }
        
        // Libero lock del semáforo antes de destruirlo
        sem_unlock(&sem_to_close->lock);
        queueFree(sem_to_close->blocked_processes);
        mm_free(sem_to_close);
        sem_unlock(&s_queue->lock);
        _sti();  // Rehabilitar interrupciones
        return 0;
    }
}

void free_sem_queue(void) {
    if (s_queue == NULL) {
        return;
    }

    if (s_queue->sems) {
        queueFree(s_queue->sems);
    }

    mm_free(s_queue);
    s_queue = NULL;
}

int sem_post(Sem sem_to_post) {
    
    if (!valid_sem(sem_to_post)) {
        print("[sem_post] ERROR: invalid semaphore\n");
        return -1;
    }
    
    sem_lock(&sem_to_post->lock);

    print("[sem_post] name=");
    print(sem_to_post->name);
    print(" blocked_queue_size=");
    printDec(queueSize(sem_to_post->blocked_processes));
    print("\n");

    if (queueSize(sem_to_post->blocked_processes) != 0) {
        int pid;
        if (dequeue(sem_to_post->blocked_processes, &pid) != NULL) {
            Process *p = get_process(pid);
            if (p != NULL) {
                unblock_process(p);
            } else {
                print("[sem] WARNING: process not found!\n");
            }
        }
    } else {
        sem_to_post->value++;
    }
    sem_unlock(&sem_to_post->lock);
    return 0;
}

int sem_wait(Sem sem_to_wait) {
    if (!valid_sem(sem_to_wait)) {
        return -1;
    }

    int blocked = 0;

    sem_lock(&sem_to_wait->lock);
    if (sem_to_wait->value == 0) {
        Process *cur = get_current_process();
        if (!cur) {
            sem_unlock(&sem_to_wait->lock);
            return -1;
        }
        int pid = cur->pid;
        print("[sem] blocking pid=");
        printDec(pid);
        print("\n");
        
        if (enqueue(sem_to_wait->blocked_processes, &pid) == NULL) {
            sem_unlock(&sem_to_wait->lock);
            return -1;
        }
        // block current process
        block_process(cur);
        blocked = 1;
    } else {
        sem_to_wait->value--;
    }
    sem_unlock(&sem_to_wait->lock);

    if (blocked) {
        print("[sem] forcing scheduler interrupt\n");
        _force_scheduler_interrupt();
    }

    return 0;
}

int sem_get_value(Sem sem_to_get) {
    if (!valid_sem(sem_to_get)) {
        return -1;
    }

    int val;

    sem_lock(&sem_to_get->lock);
    val = sem_to_get->value;
    sem_unlock(&sem_to_get->lock);

    return val;
}

int sem_get_users_count(Sem sem_to_get) {
    if (!valid_sem(sem_to_get)) {
        return -1;
    }

    int u;

    sem_lock(&sem_to_get->lock);
    u = sem_to_get->users;
    sem_unlock(&sem_to_get->lock);

    return u;
}

int sem_get_blocked_processes_count(Sem sem_to_get) {
    if (!valid_sem(sem_to_get)) {
        return -1;
    }

    int s;

    sem_lock(&sem_to_get->lock);
    s = queueSize(sem_to_get->blocked_processes);
    sem_unlock(&sem_to_get->lock);

    return s;
}

int remove_from_semaphore(Sem s, int pid) {
    if (s == NULL) {
        return -1;
    }
    sem_lock(&s->lock);
    int result = queueRemove(s->blocked_processes, &pid) == NULL ? -1 : 0;
    sem_unlock(&s->lock);

    return result;
}