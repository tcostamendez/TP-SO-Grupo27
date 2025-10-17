#include "scheduler.h"
#include "process.h" // Necesitamos las definiciones de Process, ProcessState, etc.
#include <stddef.h>  // Para NULL
#include <video.h>

// --- Variables Globales del Scheduler ---

/**
 * @brief El índice del *último* proceso que se ejecutó.
 * El PID será (current_pid_index + 1).
 * Lo usamos para saber desde dónde empezar a buscar el próximo proceso.
 */
static int current_pid_index = -1;

// --- Implementación de la Interfaz (scheduler.h) ---

void init_scheduler() {
    // Empezamos en -1 para que la primera búsqueda comience en el índice 0
    // (PID 1).
    current_pid_index = -1;
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
    p->state = READY;
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
void remove_from_scheduler(Process *p) {
    (void)p; // No-op
}

/**
 * @brief El corazón del scheduler.
 * Es llamado ÚNICAMENTE por el handler de interrupción (ASM).
 * (Las interrupciones ya están deshabilitadas en este punto).
 */
uint64_t schedule(uint64_t current_rsp) {
    //print("S");
    // 1. Guardar el RSP del proceso que acaba de ser interrumpido.
    Process* old_process = get_current_process();
    if (old_process != NULL && old_process->state != TERMINATED) {
        
        old_process->rsp = current_rsp;

        // Si estaba RUNNING, lo ponemos en READY para que pueda
        // volver a ser elegido en la siguiente vuelta.
        if (old_process->state == RUNNING) {
            old_process->state = READY;
        }
    }

    //print("["); //debug

    // 2. Buscar el siguiente proceso para ejecutar (Round Robin).
    // Damos MAX_PROCESSES vueltas para asegurarnos de encontrar uno.
    for (int i = 0; i < MAX_PROCESSES; i++) {
        
        // Avanzamos al siguiente índice en la tabla
        current_pid_index = (current_pid_index + 1) % MAX_PROCESSES;
        
        Process* p = get_process(current_pid_index + 1); // PID = index + 1
        
        if (p != NULL && p->state == READY) {
            // ¡Encontramos un proceso listo para ejecutar!
            //print("F]"); // "F" de "Found"
            // Lo marcamos como el 'proceso actual' (en process.c)
            // Esta función también lo pone en estado RUNNING.
            set_current_process(p);
            
            // Retornamos el RSP del *nuevo* proceso al handler ASM.
            return p->rsp;
        }
    }
    
    //print("N]"); // "N" de "Not Found"

    // 3. ¿Qué pasa si no se encontró nada?
    // Esto NUNCA debería pasar si tenemos un "Proceso Idle"
    // (un proceso que nunca se bloquea).
    // Pero si llegara a pasar (ej: todos los procesos están BLOCKED),
    // no podemos crashear. Simplemente devolvemos el RSP del
    // proceso que ya estaba corriendo. El kernel se "congelará"
    // en ese proceso hasta que una interrupción (ej. teclado)
    // desbloquee a otro.
    
    if (old_process != NULL) {
        set_current_process(old_process);
        return old_process->rsp;
    }

    // Pánico total: No había proceso anterior y no hay procesos nuevos.
    // Devolvemos el RSP que nos dieron (probablemente el RSP del kernel).
    return current_rsp;
}