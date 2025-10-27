#include "scheduler.h"
#include "process.h" // Necesitamos las definiciones de Process, ProcessState, etc.
#include <stddef.h>  // Para NULL
#include <video.h>
#include "queue.h"
#include "interrupts.h"

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
    blocked_queue = createQueue(compareProcesses, sizeof(Process*));
    if(ready_queue == NULL || blocked_queue == NULL){
        panic("Failed to create scheduler queues");
    } 
    
    char* idleArgs[] = {"idle"};
    idle_proc = create_process(1, idleArgs, idleProcess, MIN_PRIORITY);
    if (idle_proc == NULL) {
        panic("Idle create failed\n");
    }
      
    running_process = idle_proc;
    idle_proc->state = RUNNING;
    print("init scheduler");
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
    // (Asumimos que la función que llama a esta, ej: set_process_state,
    // ya se encarga de la atomicidad (cli/sti))
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
    
    // Intentar remover de la cola de listos o bloqueados
    queueRemove(ready_queue, &p);
    queueRemove(blocked_queue, &p);
    
    // Si es el proceso en ejecución, limpiarlo
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

    // 1. Guardar el RSP del proceso que acaba de ser interrumpido.
    if (running_process) {
        running_process->rsp = current_rsp;
        
        // Decrementar el quantum restante
        if (running_process != idle_proc && running_process->state == RUNNING) {
            running_process->quantum_remaining--;
            
            // Si aún tiene quantum restante, no cambiar de proceso
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
        // Mientras haya procesos, busca el primero que este ready
        while (!queueIsEmpty(ready_queue)) {
            if (dequeue(ready_queue, &next) == NULL || next->state == TERMINATED) {
                next = NULL;
                break;
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
        return idle_proc->rsp;
    } 
    
    next->state = RUNNING;
    next->quantum_remaining = next->priority + 1;
    running_process = next;  
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
        // Remover de la cola de bloqueados
    if (queueRemove(blocked_queue, &p) != NULL) {
        // Añadir a la cola de listos
        add_to_scheduler(p);
    }
    _sti();
}

void block_process(Process* p) {
    if (p == NULL || p == idle_proc || p->state == BLOCKED || p->state == TERMINATED) {
        return;
    }
    
    _cli();
    
    // Cambiar estado a BLOCKED
    p->state = BLOCKED;
    
    // Si está en la cola de listos, removerlo
    queueRemove(ready_queue, &p);
    
    // Si es el proceso en ejecución, limpiarlo
    if (running_process == p) {
        running_process = NULL;
    }
    
    // Agregarlo a la cola de bloqueados
    blocked_queue = enqueue(blocked_queue, &p);
    
    _sti();
}

int get_running_pid(){
    if(running_process != NULL){
        return running_process->pid;
    }
    return -1;
}
