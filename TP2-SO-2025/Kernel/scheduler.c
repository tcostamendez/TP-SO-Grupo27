#include "scheduler.h"
#include <stddef.h>  
#include <video.h>
#include "pipe.h"
#include "interrupts.h"
#include "process.h"

// Global variable definitions
QueueADT ready_queue = NULL;
QueueADT blocked_queue = NULL;
Process* running_process = NULL;
Process* idle_proc = NULL;

static volatile int scheduler_online = 0;   

static int compareProcesses(void *a, void *b) {
	Process *procA = *(Process **)a;
	Process *procB = *(Process **)b;

	return procA->pid - procB->pid;
}

static void panic(const char *msg) {
    _cli();
    print("=== KERNEL PANIC ===\n");
    print(msg);
    for (;;) { 
        _hlt(); 
    }
}

void idleProcess(){
    while(1){
        _sti();
        _hlt();
    }
}

void init_scheduler() {
    ready_queue = createQueue(compareProcesses, sizeof(Process*));
    if(ready_queue == NULL){
        panic("Failed to create ready queue");
    }
    
    blocked_queue = createQueue(compareProcesses, sizeof(Process*));
    if(blocked_queue == NULL){
        panic("Failed to create blocked queue");
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
 * @brief Añade un proceso al scheduler.
 * En nuestro diseño Round Robin, no necesitamos una cola separada.
 * La "cola" es el process_table global.
 * Simplemente nos aseguramos de que su estado sea READY.
 * Esta función es llamada por create_process() y unblock_process().
 */
void add_to_scheduler(Process *p) {
    if (p == NULL || p == idle_proc) {
        return;
    }
    p->state=READY;
    ready_queue = enqueue(ready_queue,&p);
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
    
    queueRemove(ready_queue, &p);
    queueRemove(blocked_queue, &p);
    
    if (running_process == p) {
        running_process = NULL;
    }
    
    _sti();
}

/**
 * @brief El corazón del scheduler.
 * Es llamado ÚNICAMENTE por el handler de interrupción (ASM).
 * (Las interrupciones ya están deshabilitadas en este punto).
 * 
 * Implementa Round Robin con prioridades:
 * - Los procesos con mayor prioridad obtienen más tiempo de CPU (quantum más largo)
 * - Quantum = priority + 1 ticks
 * 
 * Chequeamos si hay procesos TERMINATED y los saltamos, puede haber alguno por race conditions
 */
uint64_t schedule(uint64_t current_rsp) {
    if (!scheduler_online) {
        return current_rsp;
    }

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
            
            running_process->quantum_remaining = running_process->priority + 1;
            running_process->state = READY;
            add_to_scheduler(running_process);
        }
    }
    
    Process* next = NULL;

    if (!queueIsEmpty(ready_queue)) {
        while (!queueIsEmpty(ready_queue)) {
            if (dequeue(ready_queue, &next) == NULL) {
                next = NULL;
                break;
            }
            
            if (next->state == TERMINATED) {;
                next = NULL;
                continue; 
            }
            
            if (next->state == BLOCKED) {
                blocked_queue = enqueue(blocked_queue, &next);
                next = NULL;
                continue;
            }
            
            if (next != NULL && next->state == READY && next != idle_proc) { 
                break;
            }
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
    next->quantum_remaining = next->priority + 1;
    running_process = next;
    
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

int get_ready_process_count() {
    return queueSize(ready_queue);
}

int get_blocked_process_count() {
    return queueSize(blocked_queue);
}

void unblock_process(Process* p) {
    if (p == NULL || p == idle_proc || p->state != BLOCKED) {
        return;
    }
    _cli();

    if (queueRemove(blocked_queue, &p) != NULL) {
        add_to_scheduler(p);
    } 

    _sti();
}

void block_process(Process* p) {
    if (p == NULL || p == idle_proc || p->state == BLOCKED || p->state == TERMINATED) {
        return;
    }
    
    _cli();
    
    p->state = BLOCKED;
    
    queueRemove(ready_queue, &p);
    blocked_queue = enqueue(blocked_queue, &p);

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
