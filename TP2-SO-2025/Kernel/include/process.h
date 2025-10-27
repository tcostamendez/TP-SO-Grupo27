#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h> // Para size_t

// Definimos un tamaño de stack por defecto para cada proceso (ej: 8KB)
// Lo tomamos de tu memory_manager.h (FIRST_FIT_MM_H)
#define PROCESS_STACK_SIZE (1024 * 8) 

// Límite de procesos que podemos tener.
#define MAX_PROCESSES 64

// Nombre máximo para un proceso (debugging)
#define MAX_PROCESS_NAME 32

// Prioridades del proceso
#define MIN_PRIORITY 0
#define MAX_PRIORITY 3
#define DEFAULT_PRIORITY 0

#define FOREGROUND 1
#define BACKGROUND 0

// El entry point de un proceso.
typedef void (*ProcessEntryPoint)(int argc, char**argv);

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
    int ppid;               // Parent Process ID
    ProcessState state;     // Estado actual (READY, RUNNING, etc.)
    int argc;
    char ** argv;
    // --- Contexto de la CPU ---
    // El RSP es lo único que necesitamos guardar para el context switch.
    // El 'schedule' en C recibirá el RSP del proceso saliente
    // y lo guardará aquí.
    uint64_t rsp;
    // --- Gestión de Memoria ---
    void *stackBase;        // Puntero al inicio del stack (para mm_free())
    // --- Info de Ejecución ---
    ProcessEntryPoint rip;    // Puntero a la función a ejecutar
    // --- Scheduling ---
    int priority;           // Prioridad del proceso (0-3, mayor = más prioridad)
    int quantum_remaining;  // Ticks restantes en el quantum actual
    // --- Estado de Ejecución ---
    int ground;      // 1 si está en foreground, 0 si en background
    uint64_t rbp;           // Base pointer (para debugging/listing)
    // (Más adelante podemos añadir FDs, semáforos, etc.)

} Process;


// --- Interfaz Pública de Gestión de Procesos ---

/**
 * @brief Inicializa el Process Control Block (PCB) global.
 * Debe llamarse una sola vez al inicio del kernel.
 */
void init_pcb();

/**
 * @brief Crea un nuevo proceso.
 * * 1. Pide memoria para el stack (usando tu mm_alloc).
 * 2. Pide memoria para la estructura Process.
 * 3. Prepara el "stack falso" inicial (llamando a stackInit en ASM).
 * 4. Lo añade al scheduler.
 *
 * @param name Nombre del proceso.
 * @param entry_point Puntero a la función que debe ejecutar.
 * @param priority Prioridad del proceso (0-3).
 * @return El PID del nuevo proceso, o -1 si hay error.
 */
Process* create_process(int argc, char ** argv, ProcessEntryPoint entry_point, int priority);

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
 * @brief Establece si un proceso está en foreground o background.
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

#endif // PROCESS_H