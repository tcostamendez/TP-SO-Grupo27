#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h> // Para size_t

/**
 * PROCESOS ESPECIALES:
 * - PID 0 (IDLE): ppid = -1, NUNCA se agrega al scheduler, siempre está running cuando no hay otros procesos
 * - PID 1 (INIT): ppid = -1, se agrega al scheduler pero debe bloquearse inmediatamente (TODO: implementar)
 */

#define PROCESS_STACK_SIZE (1024 * 8) 

// Límite de procesos que podemos tener.
#define MAX_PROCESSES 64
#define MAX_CHILDREN 32

#define MAX_PROCESS_NAME 32

// Prioridades del proceso
#define MIN_PRIORITY 0
#define MAX_PRIORITY 3
#define DEFAULT_PRIORITY 0

#define FOREGROUND 1
#define BACKGROUND 0

// El entry point de un proceso.
typedef void (*process_entry_point)(int argc, char**argv);

// Enumeración de los estados de un proceso
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED // Usaremos este estado para 'limpiar' procesos
} ProcessState;

/**
 * @brief Process Control Block (PCB)
 * Esta estructura contiene toda la información de un proceso.
 */
typedef struct Process {
    int pid;                // Process ID
    int ppid;               // Parent Process ID (-1 para procesos especiales: idle e init)
    ProcessState state;     // Estado actual 

    int argc;
    char ** argv;

    uint64_t rsp;

    void *stack_base;        // Puntero al inicio del stack (para mm_free())

    process_entry_point rip;    // Puntero a la función a ejecutar

    int priority;           // Prioridad del proceso (0-3, mayor = más prioridad)
    int quantum_remaining;  // Ticks restantes en el quantum actual

    int ground;      // 1 si está en foreground, 0 si en background
    uint64_t rbp;           // Base pointer (para debugging/listing)

    int children[MAX_CHILDREN]; // IDs de los procesos hijos
    int child_count;            // Número de hijos actuales
} Process;

/**
 * @brief Inicializa el Process Control Block (PCB) global.
 * Debe llamarse una sola vez al inicio del kernel.
 */
void init_pcb();

/**
 * @brief Crea un nuevo proceso.
 * 1. Pide memoria para el stack (usando tu mm_alloc).
 * 2. Pide memoria para la estructura Process.
 * 3. Prepara el "stack falso" inicial (llamando a stack_init en ASM).
 * 4. Lo añade al scheduler.
 *
 * @param name Nombre del proceso.
 * @param entry_point Puntero a la función que debe ejecutar.
 * @param priority Prioridad del proceso (0-3).
 * @return El PID del nuevo proceso, o -1 si hay error.
 */
Process* create_process(int argc, char ** argv, process_entry_point entry_point, int priority);

/**
 * @brief Obtiene el PID del proceso que se está ejecutando.
 * @return PID del proceso 'RUNNING'.
 */
int get_pid();

/**
 * @brief Cede voluntariamente la CPU al scheduler.
 * Llama a la interrupción del timer por software.
 */
void yield_cpu();


/**
 * @brief Obtiene la estructura de un proceso por su PID.
 * @param pid PID a buscar.
 * @return Puntero a la struct Process, o NULL si no existe.
 */
Process* get_process(int pid);

/**
 * @brief Cambia la prioridad de un proceso.
 * @param pid PID del proceso.
 * @param new_priority Nueva prioridad (0-3).
 * @return 0 en éxito, -1 en error.
 */
int set_priority(int pid, int new_priority);

/**
 * @brief Obtiene la prioridad de un proceso.
 * @param pid PID del proceso.
 * @return Prioridad del proceso, o -1 si no existe.
 */
int get_priority(int pid);

/**
 * @brief Obtiene el proceso actualmente en ejecución.
 * @return Puntero al proceso RUNNING (nunca NULL, siempre hay un proceso corriendo).
 */
Process* get_current_process();

/**
 * @brief Cuenta cuántos procesos existen en total.
 * @return Cantidad de procesos activos (READY, RUNNING, BLOCKED).
 */
int get_process_count();

/**
 * @brief Itera sobre todos los procesos en la tabla.
 * @param callback Función que se llama para cada proceso no NULL.
 * @param arg Argumento adicional para pasar al callback.
 */
void foreach_process(void (*callback)(Process* p, void* arg), void* arg);

/**
 * @brief Termina un proceso dado su PID.
 * Libera sus recursos y lo marca como TERMINATED.
 * @param pid PID del proceso a terminar.
 * @return 0 en éxito, -1 si el proceso no existe.
 */
int kill_process(int pid);

/**
 * @brief Función terminadora llamada cuando un proceso retorna desde su punto de entrada.
 * Esta función se ejecuta en el contexto del proceso que está terminando, por lo que NO puede liberar su propia memoria.
 * La limpieza de memoria ocurre más tarde via reap_terminated_processes().
 */
void process_terminator(void);

/**
 * @brief Setea el ground de un proceso.
 * @param pid PID del proceso.
 * @param ground FOREGROUND o BACKGROUND.
 * @return 0 en éxito, -1 si el proceso no existe.
 */
int set_ground(int pid, int ground);

/**
 * @brief Obtiene si un proceso está en foreground o background.
 * @param pid PID del proceso.
 * @return FOREGROUND o BACKGROUND, -1 si no existe.
 */
int get_ground(int pid);

int wait_child(int child_pid);

int wait_all_children(void);

void reap_terminated_processes(void);

#endif // PROCESS_H