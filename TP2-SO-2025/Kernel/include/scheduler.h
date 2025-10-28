#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "process.h" // Depende de la definición de 'Process'

/**
 * @brief Inicializa el scheduler.
 * (Para Round Robin, esto inicializa la lista/array de procesos).
 */
int init_scheduler();

/**
 * @brief Añade un proceso a la cola de 'READY' del scheduler.
 * @param p Puntero al PCB del proceso.
 * @return 0 en éxito, -1 en error.
 */
int add_to_scheduler(Process *p);

/**
 * @brief Remueve un proceso específico del scheduler (de cualquier cola).
 * Busca el proceso en ready_queue y blocked_queue y lo elimina.
 * @param p Puntero al PCB del proceso a remover.
 * @return 0 en éxito, -1 en error.
 */
int remove_from_scheduler(Process* p);

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
 * @brief Cuenta cuántos procesos hay en la cola de listos.
 * @return Cantidad de procesos en ready_queue.
 */
int get_ready_process_count();

/**
 * @brief Cuenta cuántos procesos hay en la cola de bloqueados.
 * @return Cantidad de procesos en blocked_queue.
 */
int get_blocked_process_count();

/**
 * @brief Desbloquea un proceso y lo mueve a la cola de listos.
 * @param p Puntero al proceso a desbloquear.
 */
void unblock_process(Process* p);

/**
 * @brief Bloquea un proceso y lo mueve a la cola de bloqueados.
 * @param p Puntero al proceso a bloquear.
 */
void block_process(Process* p);

int get_running_pid();


#endif // SCHEDULER_H