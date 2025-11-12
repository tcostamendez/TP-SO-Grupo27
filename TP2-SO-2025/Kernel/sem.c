#include "sem.h"
#include "interrupts.h"
#include "process.h"
#include "scheduler.h"

extern void semLock(uint8_t *lock);
extern void semUnlock(uint8_t *lock);
extern void _force_scheduler_interrupt();

// Global semaphore registry
static semQueue *global_semaphore_registry = NULL;

// Forward declarations for helper functions
static Sem locate_semaphore_by_name(const char *semaphore_name);
static int is_semaphore_valid(Sem target_semaphore);
static int compare_process_ids(void *pid_a, void *pid_b);
static int compare_semaphore_pointers(void *sem_a_ptr, void *sem_b_ptr);


int initSemQueue(void) {
    if (global_semaphore_registry != NULL) {
        panic("Semaphore queue already initialized");
    }
    
    global_semaphore_registry = mm_alloc(sizeof(semQueue));
    if (global_semaphore_registry == NULL) {
        panic("Failed to allocate memory for semaphore queue");
    }
    
    global_semaphore_registry->lock = 0;
    global_semaphore_registry->sems = createQueue(compare_semaphore_pointers, sizeof(struct sem *));
    
    return (global_semaphore_registry->sems == NULL) ? -1 : 1;
}

void freeSemQueue(void) {
    if (global_semaphore_registry == NULL) {
        return;
    }
    
    semLock(&global_semaphore_registry->lock);
    
    if (global_semaphore_registry->sems != NULL) {
        while (queueSize(global_semaphore_registry->sems) > 0) {
            Sem semaphore_entry = NULL;
            if (dequeue(global_semaphore_registry->sems, &semaphore_entry) != NULL && 
                semaphore_entry != NULL) {
                if (semaphore_entry->blockedProcesses != NULL) {
                    queueFree(semaphore_entry->blockedProcesses);
                }
                mm_free(semaphore_entry);
            }
        }
        queueFree(global_semaphore_registry->sems);
    }
    
    semUnlock(&global_semaphore_registry->lock);
    mm_free(global_semaphore_registry);
    global_semaphore_registry = NULL;
}

Sem semOpen(const char *name, uint16_t value) {
    if (name == NULL || strlen(name) >= MAX_SEM_LENGTH) {
        return NULL;
    }
    
    if (global_semaphore_registry == NULL) {
        return NULL;
    }

    semLock(&global_semaphore_registry->lock);

    // Check if semaphore with this name already exists
    Sem existing_semaphore = NULL;
    if (!queueIsEmpty(global_semaphore_registry->sems)) {
        existing_semaphore = locate_semaphore_by_name(name);
        if (existing_semaphore != NULL) {
            semLock(&existing_semaphore->lock);
            existing_semaphore->users++;
            semUnlock(&existing_semaphore->lock);
            semUnlock(&global_semaphore_registry->lock);
            return existing_semaphore;
        }
    }

    // Create a new semaphore
    Sem new_semaphore = mm_alloc(sizeof(sem));
    if (new_semaphore == NULL) {
        semUnlock(&global_semaphore_registry->lock);
        return NULL;
    }

    my_strcpy(new_semaphore->name, name);
    new_semaphore->value = value;
    new_semaphore->lock = 0;
    new_semaphore->users = 1;
    new_semaphore->blockedProcesses = createQueue(compare_process_ids, sizeof(int));
    
    if (new_semaphore->blockedProcesses == NULL) {
        mm_free(new_semaphore);
        semUnlock(&global_semaphore_registry->lock);
        return NULL;
    }

    enqueue(global_semaphore_registry->sems, &new_semaphore);
    semUnlock(&global_semaphore_registry->lock);

    return new_semaphore;
}

int semClose(Sem target_semaphore) {
    if (global_semaphore_registry == NULL) {
        return -1;
    }
    
    if (!is_semaphore_valid(target_semaphore)) {
        return -1;
    }

    semLock(&global_semaphore_registry->lock);
    semLock(&target_semaphore->lock);

    target_semaphore->users--;

    // If other processes are still using this semaphore, don't remove it
    if (target_semaphore->users != 0) {
        semUnlock(&target_semaphore->lock);
        semUnlock(&global_semaphore_registry->lock);
        return 0;
    }

    // Remove semaphore from registry
    Sem semaphore_to_remove = target_semaphore;
    if (queueRemove(global_semaphore_registry->sems, &semaphore_to_remove) == NULL) {
        target_semaphore->users++;
        semUnlock(&target_semaphore->lock);
        semUnlock(&global_semaphore_registry->lock);
        return -1;
    }

    // Clean up blocked processes queue
    if (target_semaphore->blockedProcesses != NULL) {
        queueFree(target_semaphore->blockedProcesses);
        target_semaphore->blockedProcesses = NULL;
    }

    semUnlock(&target_semaphore->lock);
    mm_free(target_semaphore);
    semUnlock(&global_semaphore_registry->lock);

    return 0;
}


int semPost(Sem target_semaphore) {
    if (global_semaphore_registry == NULL) {
        return -1;
    }
    
    if (!is_semaphore_valid(target_semaphore)) {
        return -1;
    }

    semLock(&target_semaphore->lock);
    
    int result = -1;
    int blocked_count = queueSize(target_semaphore->blockedProcesses);
    
    if (blocked_count > 0) {
        // Unblock a waiting process
        int unblocked_pid;
        if (dequeue(target_semaphore->blockedProcesses, &unblocked_pid) != NULL) {
            Process *process_to_unblock = get_process(unblocked_pid);
            if (process_to_unblock != NULL) {
                unblock_process(process_to_unblock);
            }
            result = 0;
        }
    } else {
        // No blocked processes, increment semaphore value
        target_semaphore->value++;
        result = 0;
    }
    
    semUnlock(&target_semaphore->lock);
    return result;
}

int semWait(Sem target_semaphore) {
    if (global_semaphore_registry == NULL) {
        return -1;
    }
    
    if (!is_semaphore_valid(target_semaphore)) {
        return -1;
    }

    semLock(&target_semaphore->lock);
    
    int operation_result = -1;
    int was_blocked = 0;
    
    if (target_semaphore->value == 0) {
        // Semaphore value is zero, need to block current process
        Process *current_process = get_current_process();
        
        if (current_process == NULL) {
            semUnlock(&target_semaphore->lock);
            return -1;
        }
        
        int current_pid = get_pid(current_process);
        
        if (enqueue(target_semaphore->blockedProcesses, &current_pid) == NULL) {
            semUnlock(&target_semaphore->lock);
            return -1;
        }
        
        semUnlock(&target_semaphore->lock);
        __sync_synchronize();
        
        block_process(current_process);
        was_blocked = 1;
        operation_result = 0;
    } else {
        // Semaphore value > 0, decrement and continue
        target_semaphore->value--;
        operation_result = 0;
        semUnlock(&target_semaphore->lock);
    }
    
    if (was_blocked) {
        _force_scheduler_interrupt();
    }
    
    return operation_result;
}

int semGetValue(Sem target_semaphore) {
    if (global_semaphore_registry == NULL) {
        return -1;
    }
    
    if (!is_semaphore_valid(target_semaphore)) {
        return -1;
    }

    semLock(&target_semaphore->lock);
    int current_value = target_semaphore->value;
    semUnlock(&target_semaphore->lock);
    
    return current_value;
}

int semGetUsersCount(Sem target_semaphore) {
    if (global_semaphore_registry == NULL) {
        return -1;
    }
    
    if (!is_semaphore_valid(target_semaphore)) {
        return -1;
    }

    semLock(&target_semaphore->lock);
    int user_count = target_semaphore->users;
    semUnlock(&target_semaphore->lock);
    
    return user_count;
}

int semGetBlockedProcessesCount(Sem target_semaphore) {
    if (global_semaphore_registry == NULL) {
        return -1;
    }
    
    if (!is_semaphore_valid(target_semaphore)) {
        return -1;
    }

    semLock(&target_semaphore->lock);
    int blocked_count = queueSize(target_semaphore->blockedProcesses);
    semUnlock(&target_semaphore->lock);
    
    return blocked_count;
}

int removeFromSemaphore(Sem target_semaphore, int process_id) {
    if (target_semaphore == NULL) {
        return -1;
    }
    
    semLock(&target_semaphore->lock);
    int removal_result = (queueRemove(target_semaphore->blockedProcesses, &process_id) == NULL) ? -1 : 1;
    semUnlock(&target_semaphore->lock);
    
    return removal_result;
}

/**
 * Locates a semaphore by its name in the global registry.
 * PRECONDITION: The caller must have acquired global_semaphore_registry->lock
 * 
 * @param semaphore_name Name of the semaphore to locate
 * @return Pointer to the semaphore if found, NULL otherwise
 */
static Sem locate_semaphore_by_name(const char *semaphore_name) {
    if (semaphore_name == NULL) {
        return NULL;
    }
    
    int registry_size = queueSize(global_semaphore_registry->sems);
    if (registry_size == 0) {
        return NULL;
    }
    
    sem *semaphore_candidate = NULL;
    int semaphore_found = 0;
    
    queueBeginCyclicIter(global_semaphore_registry->sems);
    for (int index = 0; index < registry_size && !semaphore_found; index++) {
        queueNextCyclicIter(global_semaphore_registry->sems, &semaphore_candidate);
        if (semaphore_candidate != NULL && 
            my_strcmp(semaphore_candidate->name, semaphore_name) == 0) {
            semaphore_found = 1;
        }
    }
    
    return semaphore_found ? semaphore_candidate : NULL;
}

static int is_semaphore_valid(Sem target_semaphore) {
    if (target_semaphore == NULL || global_semaphore_registry == NULL) {
        return 0;
    }
    
    Sem semaphore_to_check = target_semaphore;
    semLock(&global_semaphore_registry->lock);
    int is_valid = queueElementExists(global_semaphore_registry->sems, &semaphore_to_check);
    semUnlock(&global_semaphore_registry->lock);
    
    return is_valid;
}


static int compare_process_ids(void *pid_a, void *pid_b) {
    int process_id_a = *((int *)pid_a);
    int process_id_b = *((int *)pid_b);
    return process_id_a - process_id_b;
}

static int compare_semaphore_pointers(void *sem_a_ptr, void *sem_b_ptr) {
    Sem semaphore_a = *((Sem *)sem_a_ptr);
    Sem semaphore_b = *((Sem *)sem_b_ptr);
    
    if (semaphore_a < semaphore_b) {
        return -1;
    }
    if (semaphore_a > semaphore_b) {
        return 1;
    }
    return 0;
}