#include "scheduler.h"
#include <stddef.h>  // Para NULL
#include <video.h>

#include "queue.h"
#include "interrupts.h"
#include "panic.h"
#include "process.h"

// Variables globales del scheduler
QueueADT ready_queue = NULL;
QueueADT blocked_queue = NULL;
Process* running_process = NULL;
Process* idle_proc = NULL;

static volatile int scheduler_online = 0;   

static int compare_processes(void *a, void *b) {
	Process *proc_a = *(Process **)a;
	Process *proc_b = *(Process **)b;

	return proc_a->pid - proc_b->pid;
}

void idle_process(){
    while(1){
        _sti();
        _hlt();
    }
}

int init_scheduler() {
    ready_queue = createQueue(compare_processes, sizeof(Process*));
    blocked_queue = createQueue(compare_processes, sizeof(Process*));
    if(ready_queue == NULL || blocked_queue == NULL){
        return -1;
    } 
    
    char* idle_args[] = {"idle"};
    idle_proc = create_process(1, idle_args, idle_process, MIN_PRIORITY);
    if (idle_proc == NULL) {
        return -1;
    }
      
    running_process = idle_proc;
    idle_proc->state = RUNNING;

    print("init scheduler");
    scheduler_online = 1;
    return 0;
}

/**
 * @brief Añade un proceso al scheduler.
 * En nuestro diseño Round Robin, no necesitamos una cola separada.
 * La "cola" es el process_table global.
 * Simplemente nos aseguramos de que su estado sea READY.
 * Esta función es llamada por create_process() y unblock_process().
 */
void add_to_scheduler(Process *p) {
    if (p == NULL || p == idle_proc || ready_queue == NULL) {
        return;
    }
    
    p->state=READY;
    ready_queue = enqueue(ready_queue,&p);
}

/**
 * @brief Remueve un proceso específico de todas las colas del scheduler.
 * @param p Proceso a remover.
 */
void remove_from_scheduler(Process* p) {
    if (p == NULL || p == idle_proc || ready_queue == NULL || blocked_queue == NULL) {
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
        
        // Solo debug para transiciones importantes
        /*
        print("[sched] saved RSP for pid=");
        printDec(running_process->pid);
        print(" state=");
        printDec(running_process->state);
        print(" rsp=");
        printHex(current_rsp);
        print("\n");
        */
        
        // Manejar procesos TERMINATED
        if (running_process->state == TERMINATED) {
            print("[sched] current process TERMINATED, pid=");
            printDec(running_process->pid);
            print("\n");
            // No agregar a ready_queue, solo buscar el siguiente
            running_process = NULL;
        }
        // Manejar procesos BLOCKED
        else if (running_process->state == BLOCKED) {
            print("[sched] current process BLOCKED, pid=");
            printDec(running_process->pid);
            print(" - RSP saved=");
            printHex(current_rsp);
            print("\n");
            // El proceso ya está en blocked_queue (block_process lo puso ahí)
            // Solo limpiamos running_process
            running_process = NULL;
        }
        // Decrementar el quantum restante para procesos RUNNING
        else if (running_process != idle_proc && running_process->state == RUNNING) {
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

    // Solo debug ocasional
    /*
    print("[sched] looking for next process, ready_queue size=");
    printDec(queueSize(ready_queue));
    print(", blocked_queue size=");
    printDec(queueSize(blocked_queue));
    print("\n");
    */

    if (!queueIsEmpty(ready_queue)) {
        // Mientras haya procesos, busca el primero que este ready
        while (!queueIsEmpty(ready_queue)) {
            if (dequeue(ready_queue, &next) == NULL) {
                print("[sched] dequeue returned NULL\n");
                next = NULL;
                break;
            }
            
            /*
            print("[sched] dequeued pid=");
            printDec(next->pid);
            print(" state=");
            printDec(next->state);
            print("\n");
            */
            
            // Saltar procesos TERMINATED
            if (next->state == TERMINATED) {
                print("[sched] skipping TERMINATED process, pid=");
                printDec(next->pid);
                print("\n");
                next = NULL;
                continue; // Importante: continue, no break
            }
            
            // Saltar procesos BLOCKED (no deberían estar aquí, pero por si acaso)
            if (next->state == BLOCKED) {
                print("[sched] WARNING: BLOCKED process in ready_queue, pid=");
                printDec(next->pid);
                print(" - moving to blocked_queue\n");
                blocked_queue = enqueue(blocked_queue, &next);
                next = NULL;
                continue;
            }
            
            // Proceso válido encontrado
            if (next != NULL && next->state == READY && next != idle_proc) { 
                break;
            }
            next = NULL;
        }
    }

    if (next == NULL) {
        // Comentado para evitar spam cuando solo está idle ejecutando
        // print("[sched] no ready process, switching to idle\n");
        next = idle_proc;
        idle_proc->state = RUNNING;
        running_process = idle_proc;
        return idle_proc->rsp;
    } 
    
    // Verificación final: asegurar que el proceso esté READY
    if (next->state != READY) {
        print("[sched] ERROR: selected process pid=");
        printDec(next->pid);
        print(" is not READY! state=");
        printDec(next->state);
        print("\n");
        // Volver a idle como fallback
        next = idle_proc;
        idle_proc->state = RUNNING;
        running_process = idle_proc;
        return idle_proc->rsp;
    }
    
    // Solo imprimir si NO es el proceso idle (para evitar spam)
    // if (next != idle_proc) {
    //     print("[sched] switching to pid=");
    //     printDec(next->pid);
    //     print(" (state=");
    //     printDec(next->state);
    //     print(", rsp=");
    //     printHex(next->rsp);
    //     print(")\n");
    // }
    
    next->state = RUNNING;
    next->quantum_remaining = next->priority + 1;
    running_process = next;
    
    // VALIDACIÓN: verificar que el RSP sea válido
    if (next != idle_proc) {
        uint64_t stack_bottom = (uint64_t)next->stack_base;
        uint64_t stack_top = stack_bottom + PROCESS_STACK_SIZE;
        
        if (next->rsp < stack_bottom || next->rsp >= stack_top) {
            print("[sched] CRITICAL ERROR: Invalid RSP for pid=");
            printDec(next->pid);
            print(" rsp=");
            printHex(next->rsp);
            print(" stack_bottom=");
            printHex(stack_bottom);
            print(" stack_top=");
            printHex(stack_top);
            print("\n");
            
            // Fallback a idle
            next = idle_proc;
            next->state = RUNNING;
            running_process = idle_proc;
            return idle_proc->rsp;
        }
    }
    
    // Solo imprimir si NO es el proceso idle
    // if (next != idle_proc) {
    //     print("[sched] returning rsp=");
    //     printHex(next->rsp);
    //     print("\n");
    // }
    
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
    if (p == NULL || p == idle_proc || p->state != BLOCKED || blocked_queue == NULL) {
        return;
    }
    _cli();
    
    print("[sched] unblocking process pid=");
    printDec(p->pid);
    print("\n");
    
    // Remover de la cola de bloqueados
    if (queueRemove(blocked_queue, &p) != NULL) {
        print("[sched] removed from blocked_queue\n");
        // Añadir a la cola de listos
        add_to_scheduler(p);
        print("[sched] added to ready_queue\n");
    } else {
        print("[sched] WARNING: process not in blocked_queue!\n");
    }
    _sti();
}

void block_process(Process* p) {
    if (p == NULL || p == idle_proc || p->state == BLOCKED || 
        p->state == TERMINATED || ready_queue == NULL || blocked_queue == NULL) {
        return;
    }
    
    _cli();
    
    print("[sched] blocking process pid=");
    printDec(p->pid);
    print("\n");
    
    // Cambiar estado a BLOCKED PRIMERO
    p->state = BLOCKED;
    
    // Si está en la cola de listos, removerlo
    if (queueRemove(ready_queue, &p) != NULL) {
        print("[sched] removed from ready_queue\n");
    }
    
    // NO limpiar running_process aquí - dejar que el scheduler lo maneje
    // cuando se llame a schedule() y vea que el estado es BLOCKED
    
    // Agregarlo a la cola de bloqueados
    blocked_queue = enqueue(blocked_queue, &p);
    
    _sti();
}

int get_running_pid() {
    if (running_process != NULL) {
        return running_process->pid;
    }
    return -1;
}
