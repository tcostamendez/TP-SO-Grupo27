#include <lib.h>            
#include <strings.h>       
#include <interrupts.h>

#include "process.h"
#include "scheduler.h"      
#include "memory_manager.h" 
#include "strings.h"
#include "sem.h"
#include "panic.h"

#define LEN_WAIT_SEM strlen("wait_")
#define WAIT_SEM_PREFIX "wait_"

// Tabla global de procesos - almacena punteros a todos los procesos
static Process* process_table[MAX_PROCESSES] = {NULL};

extern uint64_t stack_init(uint64_t stack_top, process_entry_point rip, void (*terminator)(), int argc, char*argv[]);
extern void _force_scheduler_interrupt();
extern int remove_from_scheduler(Process* p);

// Variables globales del scheduler (definidas en scheduler.c)
extern QueueADT ready_queue;
extern QueueADT blocked_queue;
extern Process* running_process;

/**
 * @brief Encuentra y aloca un PID libre.
 * @return PID libre (0 a MAX_PROCESSES-1), o -1 si no hay slots disponibles.
 */
static int allocate_pid(void) {
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

/**
 * @brief Crea el nombre del semáforo de espera para un proceso ("wait_<pid>")
 * @param pid Process ID
 * @return Cadena asignada con el nombre del semáforo, o NULL en caso de error. El llamador debe liberar.
 */
static char* create_wait_semaphore_name(int pid) {
    char *pid_str = num_to_str((uint64_t)pid);
    if (!pid_str) return NULL;
    
    char *name = mm_alloc(LEN_WAIT_SEM + strlen(pid_str) + 1); //wait_ + pid + \0
    if (!name) {
        return NULL;
    }
    
    my_strcpy(name, WAIT_SEM_PREFIX);
    catenate(name, pid_str);
    return name;
}

/**
 * @brief Libera todos los recursos de un proceso de manera segura.
 * Verifica NULL antes de liberar cada recurso.
 * @param p Proceso a liberar
 * @param remove_from_table Si es true, también remueve el proceso de la tabla
 */
static void free_process_resources(Process* p, int remove_from_table) {
    if (p == NULL || p->pid == 0) {
        return;
    }
    
    // Liberar stack
    if (p->stack_base != NULL) {
        mm_free(p->stack_base);
        p->stack_base = NULL;
    }
    
    // Liberar argv
    if (p->argv != NULL) {
        for (int i = 0; i < p->argc; i++) {
            if (p->argv[i] != NULL) {
                mm_free(p->argv[i]);
                p->argv[i] = NULL;
            }
        }
        mm_free(p->argv);
        p->argv = NULL;
    }
    
    // Remover de la tabla si se solicita
    if (remove_from_table) {
        remove_from_process_table(p->pid);
    }
    
    mm_free(p);
}

Process* create_process(int argc, char** argv, process_entry_point entry_point, int priority) {
    // Validate arguments
    if (argc <= 0 || argv == NULL || entry_point == NULL) {
        return NULL;
    }
    
    _cli(); // Proteger asignación de PID
    
    Process* p = (Process*) mm_alloc(sizeof(Process));
    if (p == NULL) {
        _sti();
        return NULL;
    }
    
    // Inicializar todos los punteros a NULL para poder liberar de forma segura 
    memset(p, 0, sizeof(Process));

    p->stack_base = mm_alloc(PROCESS_STACK_SIZE);
    if (p->stack_base == NULL) {
        free_process_resources(p, 0);
        _sti();
        return NULL;
    }

    int new_pid = allocate_pid();
    if (new_pid == -1) {
        free_process_resources(p, 0);
        _sti();
        return NULL;
    }
    p->pid = new_pid;
    print("\n");
    
    _sti(); // Permitir interrupciones de nuevo
    
    // Procesos especiales: idle (PID 0) e init (PID 1) tienen ppid = -1
    if (p->pid == 0 || p->pid == 1) {
        p->ppid = -1;
    } else {
        Process *parent = get_current_process();
        if (parent != NULL) {
            p->ppid = parent->pid;
            if (parent->child_count < MAX_CHILDREN) {
                parent->children[parent->child_count++] = p->pid;
            }
        } else {
            p->ppid = 1; // el init se hace el padre de todos los procesos zombies
        }
    }
    p->state = READY;
    p->rip = entry_point;
    p->ground = BACKGROUND;     // Por defecto en background
    p->rbp = 0;                 // Se actualizará en runtime
    
    // Validar y establecer prioridad
    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
        priority = DEFAULT_PRIORITY;
    }
    p->priority = priority;
    p->quantum_remaining = p->priority + 1;  // Quantum basado en prioridad

    uint64_t stack_top = (uint64_t)p->stack_base + PROCESS_STACK_SIZE;

    p->argc = argc;

	p->argv = (char **)mm_alloc(sizeof(char *) * p->argc);
	if (p->argv == NULL) {
        free_process_resources(p, 0);
		_sti();
		return NULL;
	}
	
	// Inicializar todos los punteros a NULL para poder liberar de forma segura
	memset(p->argv, 0, sizeof(char *) * p->argc);

	for (int i = 0; i < p->argc; i++) {
		p->argv[i] = (char *)mm_alloc(sizeof(char) * (strlen(argv[i]) + 1));
		if (p->argv[i] == NULL) {
			free_process_resources(p, 0);
			_sti();
			return NULL;
		}
		my_strcpy(p->argv[i], argv[i]); 
	}

    p->rsp = stack_init(stack_top, p->rip, process_terminator, p->argc, p->argv);
    
    // Verificar que RSP esté dentro del stack
    uint64_t stack_bottom = (uint64_t)p->stack_base;
    if (p->rsp < stack_bottom || p->rsp >= stack_top) {
        free_process_resources(p, 0); // No está en la tabla todavía
        _sti();
        return NULL;
    }
    
    // Agregar a la tabla de procesos 
    if (add_to_process_table(p) != 0) {
        free_process_resources(p, 0); // No pudo agregarse a la tabla
        _sti();
        return NULL;
    }

    // Create wait semaphore para el proceso ("wait_<pid>")
    // No crear para idle (PID 0)
    if (p->pid != 0) {
        char *name = create_wait_semaphore_name(p->pid);
        if (name) {
            sem_open(name, 0); // Create semaphore with value 0
            mm_free(name);
        }
    }
        
    // Agregar al scheduler solo si NO es idle (PID 0)
    if (p->pid != 0) {
        _cli();
        if (add_to_scheduler(p) != 0) {
            _sti();
            free_process_resources(p, 1); // Ya está en la tabla, hay que removerlo
            return NULL;
        }
        _sti();
    }
    reap_terminated_processes();
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
        return -1; // Process doesn't exist
    }
    
    extern Process* get_running_process();
    Process* running = get_running_process();
    
    // If killing ourselves, use process_terminator
    // (can't free our own stack/memory while using it)
    if (running == p) {
        process_terminator();
        // Should never return
        for(;;) _hlt();
    }
    
    // Killing another process - we can clean up immediately
    char *sem_name = create_wait_semaphore_name(pid);
    
    _cli();
    
    // Mark as terminated and remove from scheduler
    p->state = TERMINATED;
    remove_from_scheduler(p);
    
    _sti();
    
    // Notify parent (outside _cli to avoid deadlock)
    if (sem_name) {
        Sem s = sem_open(sem_name, 0);
        if (s) {
            sem_post(s);
            sem_close(s);
        }
        mm_free(sem_name);
    }
    
    // Free all resources
    free_process_resources(p, 1); // Remover de la tabla también
    
    return 0;
}

void process_terminator(void) {
    Process *cur = get_current_process();
    if (cur == NULL) {
        panic("process_terminator: current process is NULL");
    }

    int pid = cur->pid;

    print("[TERMINATOR] pid=");
    printDec(pid);
    print(" starting termination\n");

    // Preparar el nombre del semáforo antes de deshabilitar interrupciones
    char *sem_name = create_wait_semaphore_name(pid);
    
    print("[TERMINATOR] sem_name created: ");
    print(sem_name ? sem_name : "NULL");
    print("\n");

    _cli();
    
    cur->state = TERMINATED;
    
    queueRemove(ready_queue, &cur);
    queueRemove(blocked_queue, &cur);
    if (running_process == cur) {
        running_process = NULL;
    }
    
    _sti();
    
    // Notificar al padre mediante sem_post (fuera de _cli para evitar deadlocks)
    if (sem_name) {
        print("[TERMINATOR] opening semaphore\n");
        Sem s = sem_open(sem_name, 0);
        if (s) {
            print("[TERMINATOR] doing sem_post\n");
            int result = sem_post(s);
            print("[TERMINATOR] sem_post returned: ");
            printDec(result);
            print("\n");
            sem_close(s);
        } else {
            print("[TERMINATOR] ERROR: sem_open returned NULL\n");
        }
        mm_free(sem_name);
    } else {
        print("[TERMINATOR] ERROR: sem_name is NULL\n");
    }
    print("[TERMINATOR] forcing context switch\n");
    // Forzar cambio de contexto - NUNCA debería retornar
    _force_scheduler_interrupt();

    print("[TERMINATOR] ERROR: returned after interrupt AAAAAAAAAAAAAAAAAAAA\n");
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

/**
 * @brief Waits for a specific child process to terminate
 * @param child_pid PID of child to wait for
 * @return 0 on success, -1 on error
 */
int wait_child(int child_pid) {
    char *name = create_wait_semaphore_name(child_pid);
    if (!name) {
        return -1;
    }
    
    Sem s = sem_open(name, 0);
    mm_free(name); // Free immediately after use
    
    if (!s) {
        return -1;
    }
    
    int r = sem_wait(s);
    sem_close(s);
    
    // Remove this child from parent's children array
    // so wait_all_children() doesn't wait for it again
    Process *cur = get_current_process();
    if (cur) {
        for (int i = 0; i < cur->child_count; ++i) {
            if (cur->children[i] == child_pid) {
                // Shift remaining children left
                for (int j = i; j < cur->child_count - 1; ++j) {
                    cur->children[j] = cur->children[j + 1];
                }
                cur->child_count--;
                break;
            }
        }
    }
    
    return r;
}

int wait_all_children(void) {
    Process *cur = get_current_process();
    if (!cur) return -1;
    for (int i = 0; i < cur->child_count; ++i) {
        int child_pid = cur->children[i];
        if (child_pid <= 0) continue;
        wait_child(child_pid);
    }
    return 0;
}

void reap_terminated_processes(void) {
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        Process* p = get_process(i);
        if (p && p->state == TERMINATED) {
            // Si no es el running_process (o si running_process != p)
            // liberá recursos y borrá de tabla
            if (p != get_running_process()) {
                free_process_resources(p, 1); // Remover de la tabla también
            }
        }
    }
}

