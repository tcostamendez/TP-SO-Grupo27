// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "process.h"
#include "scheduler.h"      
#include "memory_manager.h" 
#include <lib.h>           
#include <strings.h>        
#include <interrupts.h>
#include "strings.h"
#include "sem.h"
#include "fd.h" 
#include "pipe.h"


extern uint64_t stackInit(uint64_t stack_top, ProcessEntryPoint rip, void (*terminator)(), int argc, char*argv[]);
extern void _force_scheduler_interrupt();
extern void remove_process_from_scheduler(Process* p);
extern Process* get_running_process();

extern ArrayADT process_priority_table;
extern Process* running_process;
extern Process* shell_proc;

// Tabla global de procesos - almacena punteros a todos los procesos
static Process* process_table[MAX_PROCESSES] = {NULL};

/**
 * @brief Libera toda la memoria asociada a un proceso.
 * Centraliza toda la lógica de limpieza de procesos.
 * @param p Puntero al proceso a limpiar.
 * @param remove_from_scheduler  Booleano para indicar si el proceso debe ser removido del scheduler.
 */
static void free_process_resources(Process* p, int remove_from_scheduler) {
    if (p == NULL) {
        return;
    }
    
    // Liberar argv y sus elementos
    if (p->argv != NULL) {
        for (int i = 0; i < p->argc; i++) {
            if (p->argv[i] != NULL) {
                mm_free(p->argv[i]);
            }
        }
        mm_free(p->argv);
    }
    
    // Liberar stack
    if (p->stackBase != NULL) {
        mm_free(p->stackBase);
    }
    
    // Remover del scheduler si es necesario
    if (remove_from_scheduler) {
        remove_process_from_scheduler(p);
    }
    
    // Liberar la estructura del proceso
    mm_free(p);
}

/**
 * @brief Asigna un PID iterando a través de la tabla de procesos.
 * Busca un slot que esté desocupado (NULL) o que contenga un proceso terminado.
 * @return PID asignado (0-MAX_PROCESSES-1), o -1 si la tabla está llena.
 */
static int assign_pid() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] == NULL || process_table[i]->state == TERMINATED) {
            return i;
        }
    }
    return -1; // Tabla llena
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

Process* create_process(int argc, char** argv, ProcessEntryPoint entry_point, int priority, int targets[], int hasForeground) {
    _cli();
    
    Process* p = (Process*) mm_alloc(sizeof(Process));
    if (p == NULL) {
        _sti();
        return NULL;
    }

    p->stackBase = mm_alloc(PROCESS_STACK_SIZE);
    if (p->stackBase == NULL) {
        mm_free(p);
        _sti();
        return NULL;
    }

    p->pid = assign_pid();
    if (p->pid == -1) {
        free_process_resources(p, 0);
        _sti();
        return NULL;
    }
    
    _sti();
    
    if(p->pid != 0){
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
        p->ppid = 0;
    }
    p->state = READY;
    p->rip = entry_point;
    p->ground = hasForeground? FOREGROUND: BACKGROUND; 
    p->rbp = 0;              
    
    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
        priority = DEFAULT_PRIORITY;
    }
    p->priority = priority;
    p->original_priority = priority;
    p->quantum_remaining = DEFAULT_QUANTUM;
    p->wait_ticks = 0;                  // Inicializar contador de aging

    uint64_t stack_top = (uint64_t)p->stackBase + PROCESS_STACK_SIZE;

    p->argc = argc;

	if (argc > 0 && argv != NULL) {
		p->argv = (char **)mm_alloc(sizeof(char *) * p->argc);

		if (p->argv == NULL) {
			free_process_resources(p, 0);
			return NULL;
		}
	} else {
        free_process_resources(p, 0);
        return NULL;
    }

	for (int i = 0; i < p->argc; i++) {
		p->argv[i] = (char *)mm_alloc(sizeof(char) * (strlen(argv[i]) + 1));
		if (p->argv[i] == NULL) {
			free_process_resources(p, 0);
			return NULL;
		}
		my_strcpy(p->argv[i], argv[i]);
	}

	if (p->argc < 0 || p->argv == NULL) {
		p->argv[0] = "unnamed_process";
	} 
    
    p->rsp = stackInit(stack_top, p->rip, process_terminator, p->argc, p->argv);
    
    uint64_t stack_bottom = (uint64_t)p->stackBase;
    if (p->rsp < stack_bottom || p->rsp >= stack_top) {
        free_process_resources(p, 0);
        return NULL;
    }
    
    if (add_to_process_table(p) != 0) {
        free_process_resources(p, 0);
        return NULL;
    }

    if(p->pid != 0){
        char *pid_str = num_to_str((uint64_t)p->pid);
        if (pid_str) {
            int name_len = strlen("wait_") + strlen(pid_str) + 1;
            char *name = mm_alloc(name_len);
            if (name) {
                my_strcpy(name, "wait_");
                catenate(name, pid_str);
                semOpen(name, 0);
            }
        }
    }
    
    p->targetByFd[READ_FD] = targets[0];
    p->targetByFd[WRITE_FD] = targets[1];
    p->targetByFd[ERR_FD] = targets[2];

    if(p->pid != 0){
        _cli();
        add_to_scheduler(p);
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
    Process* current = get_current_process();
    if (current != NULL && current->state == RUNNING) {
        _cli();
        // Forzar el cambio de contexto poniendo quantum a 0
        current->quantum_remaining = 0;
        _sti();
    }
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
        return -1;
    }
    
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1;
    }
    
    _cli();
    p->priority = new_priority;
    p->original_priority = new_priority;
    p->quantum_remaining = DEFAULT_QUANTUM;
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
    // No se puede matar al proceso idle (PID 0) ni al init (PID 1)
    if (pid == 0 || pid == 1) {
        return -1;
    }
    
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1;
    }
    
    Process* running = get_running_process();

    _cli();

    p->state = TERMINATED;
    remove_process_from_scheduler(p);
    
    // Close file descriptors
    uint8_t r = p->targetByFd[READ_FD];
    uint8_t w = p->targetByFd[WRITE_FD];
    if (r != STDIN && r != STDOUT) {
        removeAttached(r, pid);
        closePipe(r);
    }
    if (w != STDIN && w != STDOUT && w != r) {
        removeAttached(w, pid);
        closePipe(w);
    }
    
    // Signal any waiting parents via semaphore
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
            mm_free(name);
        }
    }
    
    remove_from_process_table(pid);
    
    // Si se está eliminando la shell, limpiar el puntero global
    if (shell_proc == p) {
        shell_proc = NULL;
    }

    // Centralized cleanup: removes from scheduler and frees all memory
    free_process_resources(p, 1);

    _sti();
    
    // If killing the running process, force a context switch
    if (running == p) {
        yield_cpu();
        for(;;) _hlt();
    }
    
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

int wait_child(int child_pid) {
    /*
    char *pid_str = num_to_str((uint64_t)child_pid);
    if (!pid_str) return -1;
    int name_len = strlen("wait_") + strlen(pid_str) + 1;
    char *name = mm_alloc(name_len);
    if (!name) {
        return -1;
    }
    my_strcpy(name, "wait_");
    catenate(name, pid_str);

    Sem s = semOpen(name, 0);
    if (!s) {
        mm_free(name);
        return -1;
    }
    */
    // Poll until the process is terminated (non-blocking approach)
    // This avoids deadlock with spinlocks in a preemptive system
    
    while (1) {
        Process *child = get_process(child_pid);
        
        // Process is done
        if (child == NULL || child->state == TERMINATED) {
            print("[DEBUG] wait_child: Process ");
            printDec(child_pid);
            print(" has terminated\n");
            break;
        }
        
        // Process still running, yield CPU and try again
        print("[DEBUG] wait_child: Process ");
        printDec(child_pid);
        print(" still running, yielding...\n");
        yield_cpu();
    }
    /*
    int r = semWait(s);
    semClose(s);
    */
    Process *cur = get_current_process();
    if (cur) {
        for (int i = 0; i < cur->child_count; ++i) {
            if (cur->children[i] == child_pid) {
                for (int j = i; j < cur->child_count - 1; ++j) {
                    cur->children[j] = cur->children[j + 1];
                }
                cur->child_count--;
                break;
            }
        }
    }
    
    return 0;
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
            if (p != get_running_process()) {
                // Centralized cleanup: removes from scheduler and frees all memory
                free_process_resources(p, 0);
            }
        }
    }
}

int ps(ProcessInfo* process_info) {
    if (process_info == NULL) {
        return -1;
    }

    // Lleno el array con la información de los procesos
    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process* p = process_table[i];
        if (p != NULL && p->state != TERMINATED) {
            process_info[i].pid = p->pid;
            process_info[i].ppid = p->ppid;
            process_info[i].state = p->state;
            process_info[i].rsp = p->rsp;
            process_info[i].stackBase = p->stackBase;
            process_info[i].priority = p->priority;
            process_info[i].ground = p->ground;
            
            if (p->argc > 0 && p->argv != NULL && p->argv[0] != NULL) {
                my_strcpy(process_info[i].name, p->argv[0]);
            } else {
                my_strcpy(process_info[i].name, "unknown");
            }
        }
    }
    return 0;
}

int get_process_info(ProcessInfo * info, int pid){
    if(info == NULL || pid >= MAX_PROCESSES || process_table[pid] != NULL){
         return -1;
    }
    info->pid = process_table[pid]->pid;
    info->ppid = process_table[pid]->ppid;
    info->state = process_table[pid]->state;
    info->rsp = process_table[pid]->rsp;
    info->stackBase = process_table[pid]->stackBase;
    info->priority = process_table[pid]->priority;
    info->ground = process_table[pid]->ground;

    return 1;
}


void process_terminator(void) {
    Process *cur = get_current_process();
    if (cur == NULL) {
        for(;;) _hlt();
    }

    int pid = cur->pid;
    // ! ACA ESTA EL ERROR, SE METE EN UN DEADLOCK
    /* Semaphore approach
    char *pid_str = num_to_str((uint64_t)pid);
    char *sem_name = NULL;
    if (pid_str) {
        int name_len = strlen("wait_") + strlen(pid_str) + 1;
        sem_name = mm_alloc(name_len);
        if (sem_name) {
            my_strcpy(sem_name, "wait_");
            catenate(sem_name, pid_str);
        }
    }*/ 

    _cli();
    
    // Mark process as TERMINATED so waiting parents can detect it
    cur->state = TERMINATED;
    
    remove_process_from_scheduler(cur);
    if (running_process == cur) {
        running_process = NULL;
    }
    
    // Si se está terminando la shell, limpiar el puntero global
    if (shell_proc == cur) {
        shell_proc = NULL;
    }
    
    _sti();
    
    if (cur) {
        uint8_t r = cur->targetByFd[READ_FD];
        uint8_t w = cur->targetByFd[WRITE_FD];
        if (r != STDIN && r != STDOUT) {
            removeAttached(r, pid);
            closePipe(r);
        }
        if (w != STDIN && w != STDOUT && w != r) {
            removeAttached(w, pid);
            closePipe(w);
        }
    }
    // ! ESTO ES CONTINUACION DEL ERROR
    /*
    if (sem_name) {
        Sem s = semOpen(sem_name, 0);
        if (s) {
            semPost(s);
            semClose(s);
        }
        mm_free(sem_name);
    }
    */
    yield_cpu();
    
    for(;;) _hlt();
}
