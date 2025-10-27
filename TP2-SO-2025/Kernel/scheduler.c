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

static int compareProcesses(void *a, void *b) {
	Process *procA = *(Process **)a;
	Process *procB = *(Process **)b;

	return procA->pid - procB->pid;
}

static void panic(const char *msg) {
    print(msg);
    for (;;) { _hlt(); }
}

static Process *last_switched = NULL;
static inline void trace_switch(Process *next) {
    if (next != last_switched) {
        if (next == idle_proc) {
            print("[sched] -> idle\n");
        } else if (last_switched == idle_proc) {
            print("[sched] idle -> running\n");
        }
        last_switched = next;
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
    // 1. Guardar el RSP del proceso que acaba de ser interrumpido.
    if (running_process) {
        running_process->rsp = current_rsp;
        
        // Decrementar el quantum restante
        if (running_process->state == RUNNING) {
            running_process->quantum_remaining--;
            
            // Si aún tiene quantum restante y no es el idle, no cambiar de proceso
            // Si es el idle, siempre verificar si hay procesos listos
            if (running_process->quantum_remaining > 0 && running_process != idle_proc) {
                return current_rsp; // Continuar con el mismo proceso
            }
            
            // Si es el idle y hay procesos listos, o quantum agotado
            if (running_process == idle_proc && !queueIsEmpty(ready_queue)) {
                // Hay procesos listos, el idle debe ceder la CPU
                running_process->state = READY; // El idle vuelve a estar listo
                // No agregamos idle a la cola, solo cambiamos su estado
            } else if (running_process != idle_proc) {
                // Quantum agotado para proceso normal, resetear y volver a la cola
                running_process->quantum_remaining = running_process->priority + 1;
                running_process->state = READY;
                add_to_scheduler(running_process);
            }
        }
    }
    
    Process* next = NULL;
    if (queueIsEmpty(ready_queue)) {
        // No hay procesos ready, usar el proceso idle
        if (idle_proc != NULL) {
            idle_proc->state = RUNNING;
            idle_proc->quantum_remaining = idle_proc->priority + 1;
            running_process = idle_proc;
            return idle_proc->rsp;
        }
        // si ni siquiera hay un proceso idle estamos en problemas
        print("CRITICAL: No idle process available\n");
        return current_rsp;
    }
    
    // Obtener siguiente proceso (skip TERMINATED processes)
    while (!queueIsEmpty(ready_queue)) {
        if (dequeue(ready_queue, &next) == NULL) {
            break; // Error dequeuing
        }
        
        // Si el proceso está listo, usarlo
        if (next->state != TERMINATED) {
            break; // Found a valid process!
        }
        
        // Si está terminado, continuar buscando
        next = NULL;
    }
    
    // Si no encontramos ningún proceso válido, usar el idle
    if (next == NULL || next->state == TERMINATED) {
        if (idle_proc != NULL) {
            idle_proc->state = RUNNING;
            idle_proc->quantum_remaining = idle_proc->priority + 1;
            running_process = idle_proc;
            return idle_proc->rsp;
        }
        print("WARNING: No valid process found and no idle process\n");
        return current_rsp;
    }
    
    next->state = RUNNING;
    next->quantum_remaining = next->priority + 1; // Resetear quantum al iniciar
    running_process = next;
    trace_switch(next);
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
