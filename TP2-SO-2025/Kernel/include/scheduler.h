#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "process.h"

/**
 * @brief Inicializa el scheduler.
 */
void initializeScheduler();

/**
 * @brief Añade un proceso a la lista de procesos listos para ser ejecutados.
 * @param pid El PID del proceso a añadir.
 */
void addToScheduler(int pid);

/**
 * @brief El corazón del scheduler. Decide qué proceso se ejecuta a continuación.
 * Esta función es llamada por la interrupción del timer.
 * @param currentRsp El puntero de pila del proceso que se estaba ejecutando.
 * @return El puntero de pila del SIGUIENTE proceso que debe ejecutarse.
 */
uint8_t* schedule(uint8_t* currentRsp);

#endif // SCHEDULER_H