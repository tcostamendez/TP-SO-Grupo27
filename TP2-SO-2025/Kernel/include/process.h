#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 10 // Un número pequeño para empezar
#define PROCESS_STACK_SIZE 4096 // 4KB de stack para cada proceso

// Estados súper simples: o está listo para ejecutarse, o ya se está ejecutando.
typedef enum {
    READY,
    RUNNING
} ProcessState;

// El Bloque de Control de Proceso (PCB) más simple posible.
// Solo contiene lo esencial para el context switch.
typedef struct Process {
    int pid;                // Identificador del proceso.
    ProcessState state;     // Estado actual (READY o RUNNING).
    uint8_t *rsp;           // Puntero de la pila (¡la clave del context switch!).
    uint8_t *stackBase;     // Guardamos la base para poder liberarla después.
} Process;

/**
 * @brief Inicializa el sistema de procesos.
 */
void initializeProcesses();

/**
 * @brief Crea un nuevo proceso.
 * @param entryPoint Puntero a la función que el proceso ejecutará.
 * @return El PID del nuevo proceso, o -1 si hay un error.
 */
int createProcess(void *entryPoint);

#endif // PROCESS_H