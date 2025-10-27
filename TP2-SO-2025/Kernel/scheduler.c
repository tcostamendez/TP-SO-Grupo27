#include "scheduler.h"
#include "process.h" // Necesitamos las definiciones de Process, ProcessState, etc.
#include <stddef.h>  // Para NULL
#include <video.h>
#include "queue.h"
#include "interrupts.h"

QueueADT ready_queue = NULL;
QueueADT blocked_queue = NULL;
Process* running_process = NULL;

static int compareProcesses(void *a, void *b) {
	Process *procA = *(Process **)a;
	Process *procB = *(Process **)b;

	return procA->pid - procB->pid;
}

void idleProcess(){
    while(1){
        _hlt();
    }
}

void init_scheduler() {
    ready_queue = createQueue(compareProcesses, sizeof(Process*));
    Process* idle=create_process(0,NULL, idleProcess); //! AGREGAR PRIORITY
    blocked_queue = createQueue(compareProcesses, sizeof(Process*));
    if(ready_queue == NULL || blocked_queue == NULL){
        print("AYUDAAAAA");
    }   
    running_process = idle;
    idle->state = RUNNING;
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
    if (p == NULL) {
        return;
    }
    // (Asumimos que la función que llama a esta, ej: set_process_state,
    // ya se encarga de la atomicidad (cli/sti))
    p->state=READY;
    ready_queue = enqueue(ready_queue,&p);
}

/**
 * @brief Quita el proceso en ejecución del scheduler y lo bloquea.
 * Mueve el proceso actual a la cola de bloqueados.
 */
void remove_from_scheduler() { //saca al proceso que esta corriendo
    if(running_process == NULL){
        return;
    }
    _cli();
    running_process->state = BLOCKED;
    blocked_queue = enqueue(blocked_queue, &running_process);
    if(blocked_queue == NULL){
        print("BLOCKED NULL");
    }
    running_process = NULL; // FIX: Limpiar el puntero!
    _sti();
}

/**
 * @brief Remueve un proceso específico de todas las colas del scheduler.
 * @param p Proceso a remover.
 */
void remove_process_from_scheduler(Process* p) {
    if (p == NULL) {
        return;
    }
    
    _cli();
    
    // Intentar remover de la cola de listos
    queueRemove(ready_queue, &p);
    
    // Intentar remover de la cola de bloqueados
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
    if (running_process != NULL) {
        running_process->rsp = current_rsp;
        
        // Decrementar el quantum restante
        if (running_process->state == RUNNING) {
            running_process->quantum_remaining--;
            
            // Si aún tiene quantum restante, no cambiar de proceso
            if (running_process->quantum_remaining > 0) {
                return current_rsp; // Continuar con el mismo proceso
            }
            
            // Quantum agotado, resetear y volver a la cola
            running_process->quantum_remaining = running_process->priority + 1;
            running_process->state = READY;
            add_to_scheduler(running_process);
        }
    }
    
    Process* next = NULL;
    if(queueIsEmpty(ready_queue)) {
        //si no hay procesos ready sigo con idle
        if (running_process != NULL && running_process->state == RUNNING) {
            return current_rsp;
        }
        //si ni siquiera hay un proceso corriendo estamos en problemas
        print("CRITICAL: No processes available to run\n");
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
    
    // Si no encontramos ningún proceso válido, volver al idle
    if (next == NULL || next->state == TERMINATED) {
        if (running_process != NULL && running_process->state == RUNNING) {
            return current_rsp;
        }
        print("WARNING: No valid process found\n");
        return current_rsp;
    }
    
    next->state = RUNNING;
    next->quantum_remaining = next->priority + 1; // Resetear quantum al iniciar
    running_process = next;
    return next->rsp;
}

Process* get_running_process() {
    return running_process;
}

int get_ready_process_count() {
    return queueSize(ready_queue);
}

int get_blocked_process_count() {
    return queueSize(blocked_queue);
}

void unblock_process(Process* p) {
    if (p == NULL || p->state != BLOCKED) {
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
    if (p == NULL) {
        return;
    }
    
    // No se puede bloquear un proceso ya bloqueado o terminado
    if (p->state == BLOCKED || p->state == TERMINATED) {
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
