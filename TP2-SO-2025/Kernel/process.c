#include "process.h"
#include "scheduler.h"      // Lo necesitamos para add_to_scheduler()
#include "memory_manager.h" // (Tu header) Lo necesitamos para mm_alloc() y mm_free()
#include <lib.h>            // (Tu header) Para memset()
#include <strings.h>        // (Tu header) Para my_strcpy() y strlen()

// --- Declaraciones de Funciones Externas (ASM) ---

/**
 * @brief Prepara un stack falso para un nuevo proceso.
 * (Esta función la crearemos en 'libasm.asm' más adelante).
 * * @param stack_top Puntero al tope del stack (ej: stackBase + STACK_SIZE)
 * @param rip       Puntero a la función a ejecutar (el entry point)
 * @param argc      Argument count
 * @param argv      Argument vector
 * @param terminator Puntero a la función (wrapper) que debe ejecutarse
 * cuando 'rip' retorne.
 * @return El nuevo valor de RSP para este contexto.
 */
extern uint64_t stackInit(uint64_t stack_top, ProcessEntryPoint rip, void (*terminator)());

/**
 * @brief Fuerza una interrupción de timer (int 0x20).
 * (Esta función ya la tienes en 'interrupts.asm' (o similar)).
 */
extern void _force_scheduler_interrupt();


// --- Variables Globales del PCB (estáticas) ---

// "La base de datos" de todos los procesos.
static Process* process_table[MAX_PROCESSES];

// El PID del proceso que está actualmente en la CPU.
static Process* running_process = NULL;

// Contador para el próximo PID a asignar.
static int next_pid = 1;


// --- Funciones de Wrapper y Helpers ---

/**
 * @brief Busca el próximo PID disponible.
 * (Implementación simple con wrap-around).
 */
static int get_new_pid() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        int pid = ((next_pid - 1 + i) % MAX_PROCESSES) + 1;
        if (process_table[pid - 1] == NULL) {
            next_pid = pid + 1;
            if (next_pid > MAX_PROCESSES) {
                next_pid = 1;
            }
            return pid;
        }
    }
    return -1; // No hay PIDs libres
}

/**
 * @brief Wrapper de terminación.
 * Esta es la función que se ejecuta cuando el entry_point de un
 * proceso (ej: la shell) retorna. Su trabajo es limpiar la memoria
 * y forzar al scheduler a ejecutar otra cosa, porque este proceso
 * ha muerto.
 */
static void process_terminator() {
    Process* p = get_current_process();
    p->state = TERMINATED;
    
    remove_from_scheduler(p);

    // --- CAMBIO ---
    // Ya no hay que liberar argv
    mm_free(p->stackBase);
    process_table[p->pid - 1] = NULL;
    mm_free(p);
    
    _force_scheduler_interrupt();
    while(1);
}


// --- Implementación de la Interfaz (process.h) ---

void init_pcb() {
    // Limpiamos la tabla de procesos y el proceso 'running'
    memset(process_table, 0, sizeof(Process*) * MAX_PROCESSES);
    running_process = NULL;
    next_pid = 1;
}

int create_process(char *name, ProcessEntryPoint entry_point) {
    
    int new_pid = get_new_pid();
    if (new_pid == -1) {
        print("PID_FAIL\n"); // DEBUG
        return -1;
    }

    Process* p = (Process*) mm_alloc(sizeof(Process));
    if (p == NULL) {
        print("PCB_ALLOC_FAIL\n"); // DEBUG
        return -1;
    }

    p->stackBase = mm_alloc(PROCESS_STACK_SIZE);
    if (p->stackBase == NULL) {
        print("STACK_ALLOC_FAIL\n"); // DEBUG
        mm_free(p);
        return -1;
    }

    // --- DEBUG ---
    print("New Proc: "); print(name);
    print(" Stack Base: "); printHex((uint64_t)p->stackBase);
    print("\n");
    // --- FIN DEBUG ---
    
    // (Toda la lógica de copiar argv se ha eliminado)

    p->pid = new_pid;
    p->ppid = get_pid();
    p->state = READY;
    p->rip = entry_point;
    my_strcpy(p->name, name);
    
    uint64_t stack_top = (uint64_t)p->stackBase + PROCESS_STACK_SIZE;

    // --- CAMBIO ---
    // Llamada a stackInit simplificada
    p->rsp = stackInit(stack_top, p->rip, process_terminator);

    process_table[p->pid - 1] = p;
    add_to_scheduler(p);

    return new_pid;
}

int get_pid() {
    if (running_process == NULL) {
        return 0; // PID 0 = Kernel (antes de que inicie el scheduler)
    }
    return running_process->pid;
}

void yield_cpu() {
    _force_scheduler_interrupt();
}

Process* get_process(int pid) {
    if (pid < 1 || pid > MAX_PROCESSES) {
        return NULL;
    }
    return process_table[pid - 1];
}

int set_process_state(int pid, ProcessState newState) {
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1;
    }
    p->state = newState;
    return 0;
}

// --- Funciones para uso *interno* del Scheduler ---

Process* get_current_process() {
    return running_process;
}

void set_current_process(Process* p) {
    running_process = p;
    if (p != NULL) {
        p->state = RUNNING;
    }
}