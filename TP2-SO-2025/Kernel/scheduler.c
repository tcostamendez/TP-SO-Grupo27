// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "scheduler.h"
#include <stddef.h>  
#include <video.h>
#include "pipe.h"
#include "interrupts.h"
#include "process.h"

// Global variable definitions
ArrayADT process_priority_table;
Process* running_process = NULL;
Process* idle_proc = NULL;
Process* shell_proc = NULL;

static volatile int scheduler_online = 0;   

static int compareProcesses(void *a, void *b) {
	Process *procA = *(Process **)a;
	Process *procB = *(Process **)b;

	return procA->pid - procB->pid;
}

/**
 * @brief Gets a queue from the process priority table by priority level.
 * @param priority Priority level (0-3).
 * @return Pointer to the queue for that priority, or NULL if invalid.
 */
static QueueADT get_priority_queue(int priority) {
    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY || process_priority_table == NULL) {
        return NULL;
    }
    
    QueueADT queue = NULL;
    if (getElemByIndex(process_priority_table, priority, &queue) == NULL) {
        return NULL;
    }
    return queue;
}

/**
 * @brief Updates a queue in the process priority table.
 * @param priority Priority level (0-3).
 * @param queue The queue to store.
 * @return 0 on success, -1 on error.
 */
static int set_priority_queue(int priority, QueueADT queue) {
    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY || process_priority_table == NULL) {
        return -1;
    }
    
    if (setElemByIndex(process_priority_table, priority, &queue) == NULL) {
        return -1;
    }
    return 0;
}

/**
 * @brief Removes a process from all priority queues.
 * @param p Process to remove.
 */
static void remove_from_all_priority_queues(Process* p) {
    if (p == NULL) {
        return;
    }
    
    // Try to remove from the process's current priority queue
    QueueADT priority_queue = get_priority_queue(p->priority);
    if (priority_queue != NULL) {
        queueRemove(priority_queue, &p);
    }
}

/**
 * @brief Aplicar aging a procesos que han esperado demasiado tiempo.
 * Incrementa el wait_ticks de todos los procesos en READY.
 * Si un proceso alcanza AGING_THRESHOLD, se boosteaa su prioridad.
 */
static void apply_aging(void) {
    for (int priority = MIN_PRIORITY; priority <= MAX_PRIORITY - AGING_BOOST; priority++) {
        QueueADT priority_queue = get_priority_queue(priority);
        if (priority_queue == NULL || queueIsEmpty(priority_queue)) {
            continue;
        }
        
        // Iterar sobre los procesos en esta cola de prioridad
        queueBeginCyclicIter(priority_queue);
        Process* p = NULL;
        while (queueNextCyclicIter(priority_queue, &p) != NULL && p != NULL) {
            if (p->state != READY || p == idle_proc) {
                continue;
            }
            
            // Incrementar contador de espera
            p->wait_ticks++;
            
            if (p->wait_ticks >= AGING_THRESHOLD) {
                p->priority = MAX_PRIORITY;

                queueRemove(priority_queue, &p);
                QueueADT new_priority_queue = get_priority_queue(MAX_PRIORITY);
                if (new_priority_queue == NULL) {
                    panic("Failed to create new priority queue");
                }
                new_priority_queue = enqueue(new_priority_queue, &p);
                set_priority_queue(MAX_PRIORITY, new_priority_queue);
                p->wait_ticks = 0;
            }
            
            if (queueCyclicIterLooped(priority_queue)) {
                break;
            }
        }
    }
}

/**
 * @brief Resetea el contador de aging cuando un proceso se ejecuta.
 * También restaura la prioridad original si fue boosteada.
 */
static void reset_aging(Process* p) {
    if (p == NULL || p == idle_proc) {
        return;
    }
    
    p->wait_ticks = 0;
}

void idleProcess(){
    while(1){
        _sti();
        _hlt();
    }
}

void init_scheduler() {
    process_priority_table = createArray(sizeof(QueueADT));
    if(process_priority_table == NULL){
        panic("Failed to create process priority table");
    }

    // Initialize 4 priority queues (one for each priority level 0-3)
    for (int priority = MIN_PRIORITY; priority <= MAX_PRIORITY; priority++) {
        QueueADT priority_queue = createQueue(compareProcesses, sizeof(Process*));
        if (priority_queue == NULL) {
            panic("Failed to create priority queue");
        }
        process_priority_table = arrayAdd(process_priority_table, &priority_queue);
        if (process_priority_table == NULL) {
            panic("Failed to add queue to priority table");
        }
    }

    char* idleArgs[] = {"idle"};
    int targets[] = {STDIN, STDOUT, STDOUT};
    idle_proc = create_process(1, idleArgs, idleProcess, MIN_PRIORITY, targets, 0);
    if (idle_proc == NULL) {
        panic("Idle create failed\n");
    }
      
    running_process = idle_proc;
    idle_proc->state = RUNNING;
    scheduler_online = 1;
}

/**
 * @brief Añade un proceso al scheduler (priority queue).
 * Agrega el proceso a la cola de prioridad correspondiente.
 * Esta función es llamada por create_process() y unblock_process().
 */
void add_to_scheduler(Process *p) {
    if (p == NULL || p == idle_proc) {
        return;
    }
    p->state = READY;
    
    QueueADT priority_queue = get_priority_queue(p->priority);
    if (priority_queue == NULL) {
        panic("Priority queue not found");
    }
    priority_queue = enqueue(priority_queue, &p);
    set_priority_queue(p->priority, priority_queue);
}

/**
 * @brief Remueve un proceso específico de todas las colas del scheduler.
 * @param p Proceso a remover.
 */
void remove_process_from_scheduler(Process* p) {
    if (p == NULL || p == idle_proc) {
        return;
    }
    
    _cli();
    
    remove_from_all_priority_queues(p);
    
    if (running_process == p) {
        running_process = NULL;
    }
    
    _sti();
}

/**
 * @brief El corazón del scheduler con soporte a prioridades.
 * Es llamado ÚNICAMENTE por el handler de interrupción (ASM).
 * (Las interrupciones ya están deshabilitadas en este punto).
 * 
 * Implementa Round Robin con prioridades:
 * - Busca procesos en orden de prioridad (3 → 2 → 1 → 0)

 * 
 * Chequeamos si hay procesos TERMINATED y los saltamos, puede haber alguno por race conditions
 */
uint64_t schedule(uint64_t current_rsp) {
    if (!scheduler_online) {
        return current_rsp;
    }

    // Aplicar aging a procesos que han esperado demasiado
    apply_aging();

    if (running_process) {
        running_process->rsp = current_rsp;
        
        if (running_process->state == TERMINATED) {
            running_process = NULL;
        }
        else if (running_process->state == BLOCKED) {
            running_process = NULL;
        }
        else if (running_process->state == RUNNING) {
            running_process->quantum_remaining--;
            if (running_process->quantum_remaining > 0) {
                return current_rsp; // Continuar con el mismo proceso
            }
            
            running_process->quantum_remaining = DEFAULT_QUANTUM;
            running_process->state = READY;
            add_to_scheduler(running_process);
        }
    }
    
    Process* next = NULL;

    // Search for next ready process in priority order (highest to lowest: 3, 2, 1, 0)
    for (int priority = MAX_PRIORITY; priority >= MIN_PRIORITY && next == NULL; priority--) {
        QueueADT priority_queue = get_priority_queue(priority);
        if (priority_queue == NULL || queueIsEmpty(priority_queue)) {
            continue;
        }
        
        // Try to find a valid READY process in this priority queue
        while (!queueIsEmpty(priority_queue)) {
            if (dequeue(priority_queue, &next) == NULL) {
                next = NULL;
                break;
            }
            
            // Skip TERMINATED processes
            if (next->state == TERMINATED) {
                next = NULL;
                continue; 
            }
            
            // Found a valid READY process
            if (next != NULL && next->state == READY && next != idle_proc) { 
                break;
            }
            
            // If not READY, skip it (should not happen since BLOCKED are removed from queue)
            next = NULL;
        }
    }

    if (next == NULL) {
        next = idle_proc;
        idle_proc->state = RUNNING;
        running_process = idle_proc;
        return idle_proc->rsp;
    } 
    
    if (next->state != READY) {
        next = idle_proc;
        idle_proc->state = RUNNING;
        running_process = idle_proc;
        return idle_proc->rsp;
    }
    
    
    next->state = RUNNING;
    next->quantum_remaining = DEFAULT_QUANTUM;
    running_process = next;
    
    // Resetear aging cuando el proceso se ejecuta
    reset_aging(next);
    
    if (next != idle_proc) {
        uint64_t stack_bottom = (uint64_t)next->stackBase;
        uint64_t stack_top = stack_bottom + PROCESS_STACK_SIZE;
        
        if (next->rsp < stack_bottom || next->rsp >= stack_top) {
            next = idle_proc;
            next->state = RUNNING;
            running_process = idle_proc;
            return idle_proc->rsp;
        }
    }
    
    return next->rsp;
}

Process* get_running_process() {
    return running_process;
}

Process* get_idle_process() {
    return idle_proc;
}

Process* get_shell_process() {
    return shell_proc;
}


void unblock_process(Process* p) {
    if (p == NULL || p == idle_proc || p->pid == 1 || p->state != BLOCKED) {
        return;
    }
    _cli();

    // Cambiar estado y volver a agregar a su priority queue
    p->state = READY;
    add_to_scheduler(p);

    _sti();
}

void block_process(Process* p) {
    if (p == NULL || p == idle_proc || p->pid == 1 || p->state == BLOCKED || p->state == TERMINATED) {
        return;
    }
    
    _cli();
    
    // Remover de priority queue
    remove_from_all_priority_queues(p);
    
    p->state = BLOCKED;
    
    // Resetear aging cuando se bloquea
    reset_aging(p);

    _sti();

    if (running_process == p) {
        running_process = NULL;
        extern void _force_scheduler_interrupt();
        _force_scheduler_interrupt();
    }
}

int get_running_pid() {
    if (running_process != NULL) {
        return running_process->pid;
    }
    return -1;
}
