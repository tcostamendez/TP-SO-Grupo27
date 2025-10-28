#include "process.h"
#include "scheduler.h"      // Lo necesitamos para add_to_scheduler()
#include "memory_manager.h" // (Tu header) Lo necesitamos para mm_alloc() y mm_free()
#include <lib.h>            // (Tu header) Para memset()
#include <strings.h>        // (Tu header) Para my_strcpy() y strlen()
#include <interrupts.h>
#include "strings.h"

/**
 * @brief Prepara un stack falso para un nuevo proceso.
 * (Esta función la crearemos en 'libasm.asm' más adelante).
 * @param stack_top Puntero al tope del stack (ej: stackBase + STACK_SIZE)
 * @param rip       Puntero a la función a ejecutar (el entry point)
 * @param argc      Argument count
 * @param argv      Argument vector
 * @param terminator Puntero a la función (wrapper) que debe ejecutarse
 * cuando 'rip' retorne.
 * @return El nuevo valor de RSP para este contexto.
 */
extern uint64_t stackInit(uint64_t stack_top, ProcessEntryPoint rip, void (*terminator)(), int argc, char*argv[]);

/**
 * @brief Fuerza una interrupción de timer (int 0x20).
 * (Esta función ya la tienes en 'interrupts.asm' (o similar)).
 */
extern void _force_scheduler_interrupt();

// Tabla global de procesos - almacena punteros a todos los procesos
static Process* process_table[MAX_PROCESSES] = {NULL};

/**
 * @brief Encuentra y aloca un PID libre.
 * @return PID libre (0 a MAX_PROCESSES-1), o -1 si no hay slots disponibles.
 */
static int allocate_pid(void) {
    if (process_table == NULL) {
        return -1;
    }
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] == NULL) {
            return i;
        }
    }
    return -1; // No hay PIDs disponibles
}

/**
 * @brief Agrega un proceso a la tabla global de procesos.
 * @param p Puntero al proceso.
 * @return 0 en éxito, -1 si la tabla está llena.
 */
static int add_to_process_table(Process* p) {
    if (p == NULL || p->pid < 0 || p->pid >= MAX_PROCESSES) {
        return -1;
    }
    process_table[p->pid] = p;
    return 0;
}

/**
 * @brief Remueve un proceso de la tabla global.
 * @param pid PID del proceso a remover.
 */
static void remove_from_process_table(int pid) {
    if (pid >= 0 && pid < MAX_PROCESSES) {
        process_table[pid] = NULL;
    }
}

Process* create_process(int argc, char** argv, ProcessEntryPoint entry_point, int priority) {
    if (argc <= 0 || (argc > 0 && argv == NULL) || entry_point == NULL) {
        return NULL; 
    }
    
    // Allocate a free PID first
    int new_pid = allocate_pid();
    if (new_pid == -1) {
        return NULL;
    }

    Process* p = (Process*) mm_alloc(sizeof(Process));
    if (p == NULL) {
        return NULL;
    }

    p->stackBase = mm_alloc(PROCESS_STACK_SIZE);
    if (p->stackBase == NULL) {
        mm_free(p);
        return NULL;
    }

    p->pid = new_pid;
    p->ppid = 0;
    p->state = READY;
    p->rip = entry_point;
    p->ground = BACKGROUND;  // Por defecto en background
    p->rbp = 0;              // Se actualizará en runtime
    
    // Validar y establecer prioridad
    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
        priority = DEFAULT_PRIORITY;
    }
    p->priority = priority;
    p->quantum_remaining = p->priority + 1;  // Quantum basado en prioridad

    uint64_t stack_top = (uint64_t)p->stackBase + PROCESS_STACK_SIZE;

    p->argc = argc;

	p->argv = (char **)mm_alloc(sizeof(char *) * p->argc);
	if (p->argv == NULL) {
		mm_free(p->stackBase);
		mm_free(p);
		return NULL;
	}

	for (int i = 0; i < p->argc; i++) {
		p->argv[i] = (char *)mm_alloc(sizeof(char) * (strlen(argv[i]) + 1));
		if (p->argv[i] == NULL) {
			for (int j = 0; j < i; j++) {
				mm_free(p->argv[j]);
			}
			mm_free(p->argv);
			mm_free(p->stackBase);
			mm_free(p);
			return NULL;
		}
		my_strcpy(p->argv[i], argv[i]); 
	}

    // Llamada a stackInit simplificada
    p->rsp = stackInit(stack_top, p->rip, entry_point, p->argc, p->argv);
    
    // Agregar a la tabla de procesos
    if (add_to_process_table(p) != 0) {
        print("Error adding process to process table\n");
        mm_free(p->stackBase);
        for (int i = 0; i < p->argc; i++) {
            mm_free(p->argv[i]); 
        }
        mm_free(p->argv);
        mm_free(p);
        return NULL;
    }
    
    // Para asegurar que se cargue
    _cli();
    add_to_scheduler(p);
    _sti();
    return p;
}

int get_pid(Process* p) {
    return p->pid;
}

int get_parent_pid(Process * p){
    return p->ppid;
}

void yield_cpu() {
    _force_scheduler_interrupt();
}

Process* get_process(int pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) {
        return NULL;
    }
    return process_table[pid];
}

int set_priority(int pid, int new_priority) {
    if (new_priority < MIN_PRIORITY || new_priority > MAX_PRIORITY) {
        return -1; // Prioridad inválida
    }
    
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1; // Proceso no encontrado
    }
    
    _cli();
    p->priority = new_priority;
    p->quantum_remaining = new_priority + 1; // Resetear quantum
    _sti();
    
    return 0;
}

int get_priority(int pid) {
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1;
    }
    return p->priority;
}

Process* get_current_process() {
    // Esta función debería llamar al scheduler para obtener el proceso running
    extern Process* get_running_process();
    return get_running_process();
}

int get_process_count() {
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] != NULL) {
            count++;
        }
    }
    return count;
}

void foreach_process(void (*callback)(Process* p, void* arg), void* arg) {
    if (callback == NULL) {
        return;
    }
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] != NULL) {
            callback(process_table[i], arg);
        }
    }
}

int kill_process(int pid) {
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1; // Proceso no existe
    }
    
    _cli();
    
    // Marcar como terminado
    p->state = TERMINATED;
    
    // Remover del scheduler (de todas las colas)
    extern void remove_process_from_scheduler(Process* p);
    remove_process_from_scheduler(p);
    
    // Liberar recursos
    if (p->stackBase != NULL) {
        mm_free(p->stackBase);
        p->stackBase = NULL;
    }
    
    // Remover de la tabla de procesos
    remove_from_process_table(pid);
    
    // Liberar el PCB
    mm_free(p);
    
    _sti();
    
    return 0;
}

int set_ground(int pid, int ground) {
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1;
    }
    
    _cli();
    p->ground = ground;
    _sti();
    
    return 0;
}

int get_ground(int pid) {
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1;
    }
    return p->ground;
}

