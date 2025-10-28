#include "process.h"
#include "scheduler.h"      // Lo necesitamos para add_to_scheduler()
#include "memory_manager.h" // (Tu header) Lo necesitamos para mm_alloc() y mm_free()
#include <lib.h>            // (Tu header) Para memset()
#include <strings.h>        // (Tu header) Para my_strcpy() y strlen()
#include <interrupts.h>
#include "strings.h"
#include "sem.h"

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
extern uint64_t stackInit(uint64_t stack_top, ProcessEntryPoint rip, void (*terminator)(), int argc, char*argv[]);

/**
 * @brief Fuerza una interrupción de timer (int 0x20).
 * (Esta función ya la tienes en 'interrupts.asm' (o similar)).
 */
extern void _force_scheduler_interrupt();

static int pid = 0;

// Contador para el próximo PID a asignar.
static int next_pid = 1;

// Tabla global de procesos - almacena punteros a todos los procesos
static Process* process_table[MAX_PROCESSES] = {NULL};

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
    Process* p = (Process*) mm_alloc(sizeof(Process));
    if (p == NULL) {
        print("PCB_ALLOC_FAIL\n"); // DEBUG
        return NULL;
    }

    p->stackBase = mm_alloc(PROCESS_STACK_SIZE);
    if (p->stackBase == NULL) {
        print("STACK_ALLOC_FAIL\n"); // DEBUG
        mm_free(p);
        return NULL;
    }

    p->pid = pid++;
    if(pid != 1){
        Process *parent = get_current_process();
        if (parent != NULL) {
            p->ppid = parent->pid;
            if (parent->child_count < MAX_CHILDREN) {
                parent->children[parent->child_count++] = p->pid;
            }
        } else {
            p->ppid = 0;
        }
    }else{
        p->ppid = 0; //el primer proceso es su propio padre
    }
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

	if (argc > 0) {
		p->argv = (char **)mm_alloc(sizeof(char *) * p->argc);

		if (p->argv == NULL) {
			mm_free(p);
			return NULL;
		}
	}

	for (int i = 0; i < p->argc; i++) {
		p->argv[i] = (char *)mm_alloc(sizeof(char) * (strlen(argv[i]) + 1));
		if (p->argv[i] == NULL) {
			for (int j = 0; j < i; j++) {
				mm_free(p->argv[j]);
			}
			mm_free(p->argv);
			mm_free(p);
			return NULL;
		}
		my_strcpy(p->argv[i], argv[i]);
	}

	if (p->argc >= 0 && p->argv != NULL) {
		p->argv[0] = p->argv[0];
	} else {
		p->argv[0] = "unnamed_process";
	}

    // --- CAMBIO ---
    // Llamada a stackInit simplificada
    p->rsp = stackInit(stack_top, p->rip, entry_point, p->argc, p->argv);
    
    // Agregar a la tabla de procesos
    if (add_to_process_table(p) != 0) {
        print("Error adding process to process table\n");
        mm_free(p->stackBase);
        mm_free(p);
        return NULL;
    }

    // crear semaforo de wait para este pid: "wait_<pid>"
    if(p->pid != 0){ //no lo quiero hacer para el primer proceso
        char *pid_str = num_to_str((uint64_t)p->pid);
        if (pid_str) {
            int name_len = strlen("wait_") + strlen(pid_str) + 1;
            char *name = mm_alloc(name_len);
            if (name) {
                my_strcpy(name, "wait_");
                catenate(name, pid_str);
                semOpen(name, 0); // crea sem con value 0
                // no cerramos acá; dejar que quien espere cierre
                // (liberaciones menores no cubiertas)
            }
        }
    }
    
    //para asegurar que se cargue
    if(p->pid != 0){ //el proceso idle no lo agrego
        _cli();
        add_to_scheduler(p);
        _sti();
    }
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
    // avisar al padre mediante semPost en "wait_<pid>"
    char *pid_str = num_to_str((uint64_t)pid);
    if (pid_str) {
        int name_len = strlen("wait_") + strlen(pid_str) + 1;
        char *name = mm_alloc(name_len);
        if (name) {
            my_strcpy(name, "wait_");
            catenate(name, pid_str);
            Sem s = semOpen(name, 0);
            if (s) {
                semPost(s);
                semClose(s);
            }
            // mm_free(name) y mm_free(pid_str) omitidos por simplicidad
        }
    }
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

// Nuevas funciones para esperar por hijos
int wait_child(int child_pid) {
    // abre el sem "wait_<child_pid>" y espera
    char *pid_str = num_to_str((uint64_t)child_pid);
    if (!pid_str) return -1;
    int name_len = strlen("wait_") + strlen(pid_str) + 1;
    char *name = mm_alloc(name_len);
    if (!name) return -1;
    my_strcpy(name, "wait_");
    catenate(name, pid_str);

    Sem s = semOpen(name, 0);
    if (!s) return -1;
    int r = semWait(s);
    semClose(s);
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

char ** get_process_data(int process_id){
    if(process_id == NULL){
        return NULL;
    }
    if(process_id < 0 || process_id >= MAX_PROCESSES || process_id > pid){
        return NULL;
    }
    char ** ans = mm_alloc(sizeof(char*) * 7); //7 porque son 6 campos y un null en el final
    char * name = mm_alloc(16); //magic number, pero se tienen que crear demasiados procesos para pasarlo
    char * id = num_to_str((uint64_t)process_table[process_id]->pid);
    my_strcpy(name, "Process ");
    catenate(name, id);
    ans[0] = name;
    ans[1] = id;
    ans[2] = num_to_str((uint64_t)process_table[process_id]->priority);
    ans[3] = num_to_str(process_table[process_id]->rsp);
    ans[4] = num_to_str(process_table[process_id]->rbp);
    ans[5] = num_to_str((uint64_t)process_table[process_id]->ground); //0 si esta en background, 1 si esta en foregorund
    ans[6] = NULL;
    return ans;
}
