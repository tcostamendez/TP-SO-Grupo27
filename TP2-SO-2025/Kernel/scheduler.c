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
    Process* idle=create_process("Idle", idleProcess);
    blocked_queue = createQueue(compareProcesses, sizeof(Process*));
    if(ready_queue == NULL || blocked_queue == NULL){
        print("AYUDAAAAA");
    }   
    running_process=idle;
    idle->state=RUNNING;
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
 * @brief Quita un proceso del scheduler.
 * En nuestro diseño, esto es un no-op.
 * El proceso que llama a esta función (ej: block_process() o
 * process_terminator()) se encargará de cambiar el estado del
 * proceso a BLOCKED o TERMINATED.
 * El scheduler simplemente lo "saltará" en la próxima iteración
 * porque no está en estado READY.
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
    _sti();
}

/**
 * @brief El corazón del scheduler.
 * Es llamado ÚNICAMENTE por el handler de interrupción (ASM).
 * (Las interrupciones ya están deshabilitadas en este punto).
 */
uint64_t schedule(uint64_t current_rsp) {
    // 1. Guardar el RSP del proceso que acaba de ser interrumpido.
    if (running_process != NULL) {
        running_process->rsp = current_rsp;
        // Si estaba RUNNING, lo ponemos en READY para que pueda
        // volver a ser elegido en la siguiente vuelta.
        if (running_process->state == RUNNING) {
            running_process->state=READY;
            add_to_scheduler(running_process);            
        }
    }
    Process* next=NULL;
    if(queueIsEmpty(ready_queue)){
        //si no hay procesos ready sigo con idle
        if (running_process != NULL && running_process->state == RUNNING) {
            return current_rsp;
        }
        //si ni siquiera hay un proceso corriendo estamos en problemas
        print("CRITICAL: No processes available to run\n");
        return current_rsp;
    }
    // Obtener siguiente proceso
    if (dequeue(ready_queue, &next) == NULL) {
        print("ERROR: Failed to dequeue next process\n");
        return current_rsp;
    }
    next->state=RUNNING;
    running_process=next;
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