#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "queue.h"
#include "array.h"

// Para evitar dependencias circulares
typedef struct Process Process;

// --- Aging Configuration ---
#define AGING_THRESHOLD 50         // Ticks esperando antes de boost de prioridad
#define AGING_BOOST 1               // Cuántos niveles de prioridad se boostan (máximo 3)

// External declarations - defined in scheduler.c
extern ArrayADT process_priority_table;    // Array of 4 priority queues (priority 0-3)
extern Process* running_process;
extern Process* idle_proc;
extern Process* shell_proc;

/**
 * @brief Inicializa el scheduler.
 * (Para Round Robin, esto inicializa la lista/array de procesos).
 */
void init_scheduler();

/**
 * @brief Añade un proceso a la cola de 'READY' del scheduler.
 * @param p Puntero al PCB del proceso.
 */
void add_to_scheduler(Process *p);

/**
 * @brief Remueve un proceso específico del scheduler (de cualquier priority queue).
 * @param p Puntero al PCB del proceso a remover.
 */
void remove_process_from_scheduler(Process* p);

/**
 * @brief El corazón del scheduler.
 * Esta función es llamada en C desde el handler de ASM (irq00Handler).
 * 1. Obtiene el proceso actual (el que fue interrumpido).
 * 2. Si el proceso está 'RUNNING', guarda su 'current_rsp' en process->rsp
 * y lo pone en 'READY'.
 * 3. Elige el siguiente proceso 'READY' de la cola (Round Robin).
 * 4. Marca el nuevo proceso como 'RUNNING'.
 * 5. Retorna el 'process->rsp' del nuevo proceso.
 * * @param current_rsp El RSP del proceso que acaba de ser interrumpido 
 * (pasado desde el handler ASM).
 * @return El RSP del proceso que debe ejecutarse ahora.
 */
uint64_t schedule(uint64_t current_rsp);

/**
 * @brief Obtiene el proceso actualmente en ejecución.
 * @return Puntero al proceso RUNNING.
 */
Process* get_running_process();

/**
 * @brief Obtiene el proceso idle.
 * @return Puntero al proceso idle.
 */
Process* get_idle_process();

/**
 * @brief Obtiene el proceso shell.
 * @return Puntero al proceso shell, o NULL si no existe.
 */
Process* get_shell_process();

/**
 * @brief Desbloquea un proceso y lo mueve a la cola de listos.
 * Cambia su estado a READY y lo agrega nuevamente a su priority queue.
 * @param p Puntero al proceso a desbloquear.
 */
void unblock_process(Process* p);

/**
 * @brief Bloquea un proceso cambiando su estado a BLOCKED.
 * Lo remueve de su priority queue y lo deja solo en la PCB table.
 * Al desbloquear, debe ser agregado nuevamente a su priority queue.
 * @param p Puntero al proceso a bloquear.
 */
void block_process(Process* p);

int get_running_pid();

#endif // SCHEDULER_H