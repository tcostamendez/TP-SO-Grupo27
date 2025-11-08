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
 * @brief Construye el nombre del semáforo para sincronización de wait.
 * @param pid PID del proceso.
 * @return Nombre del semáforo asignado dinámicamente, o NULL si falla.
 *         El caller debe liberar la memoria con mm_free().
 */
static char* build_wait_semaphore_name(int pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) {
        return NULL;
    }
    
    char *pid_str = num_to_str((uint64_t)pid);
    if (!pid_str) {
        return NULL;
    }
    
    int name_len = strlen("wait_") + strlen(pid_str) + 1;
    char *name = mm_alloc(name_len);
    if (name) {
        my_strcpy(name, "wait_");
        catenate(name, pid_str);
    }
    
    return name;
}

/**
 * @brief Re-parent all children of a dying process to the init process (PID 1).
 * This prevents orphaned processes from having invalid ppid references.
 * @param dying_pid PID of the process that is terminating.
 */
static void reparent_children(int dying_pid) {
    Process* dying = get_process(dying_pid);
    if (dying == NULL) {
        return;
    }
    
    Process* init = get_process(INIT_PID);
    if (init == NULL) {
        return; // No init process to reparent to
    }
    
    // Iterate through the dying process's children array
    for (int i = 0; i < dying->child_count; i++) {
        int child_pid = dying->children[i];
        Process* child = get_process(child_pid);
        
        if (child != NULL && child->state != TERMINATED) {
            // Update the child's ppid to point to init
            child->ppid = 1;
            
            // Add to init's children list if there's space
            if (init->child_count < MAX_CHILDREN) {
                init->children[init->child_count++] = child_pid;
            }
        }
    }
    
    // Clear the dying process's children array
    dying->child_count = 0;
}

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

    // Crear semáforo para sincronización con wait_child (solo para procesos no-idle)
    if(p->pid != 0){
        char *sem_name = build_wait_semaphore_name(p->pid);
        if (sem_name) {
            // Crear semáforo con valor 0 (bloqueado hasta que el proceso termine)
            semOpen(sem_name, 0);
            mm_free(sem_name);
        }
    }
    
    p->targetByFd[READ_FD] = targets[0];
    p->targetByFd[WRITE_FD] = targets[1];
    p->targetByFd[ERR_FD] = targets[2];

    // Adjuntar roles a pipes no estándar para mantener contadores consistentes
    if (p->targetByFd[READ_FD] != STDIN && p->targetByFd[READ_FD] != STDOUT) {
        attachReader(p->targetByFd[READ_FD]);
    }
    if (p->targetByFd[WRITE_FD] != STDIN && p->targetByFd[WRITE_FD] != STDOUT) {
        attachWriter(p->targetByFd[WRITE_FD]);
    }

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
    
    // PASO 1: Cerrar file descriptors ANTES de locks críticos
    uint8_t r = p->targetByFd[READ_FD];
    uint8_t w = p->targetByFd[WRITE_FD];
    if (r != STDIN && r != STDOUT) {
        detachReader(r, pid);
        closePipe(r);
    }
    if (w != STDIN && w != STDOUT) {
        detachWriter(w, pid);
        if (w != r) {
            closePipe(w);
        }
    }
    
    // PASO 2: Reparentar los hijos del proceso que va a morir
    reparent_children(pid);
    
    // PASO 3: Señalizar a los procesos padres que esperan (sin locks)
    char *sem_name = build_wait_semaphore_name(pid);
    if (sem_name) {
        Sem s = semOpen(sem_name, 0);
        if (s) {
            semPost(s);  // Despertar al padre que está esperando
            semClose(s);
        }
        mm_free(sem_name);
    }
    
    // PASO 4: Ahora sí, sección crítica con locks
    _cli();
    
    p->state = TERMINATED;
    
    remove_from_process_table(pid);
    
    // Si se está eliminando la shell, limpiar el puntero global
    if (shell_proc == p) {
        shell_proc = NULL;
    }

    // Centralized cleanup: removes from scheduler and frees all memory
    free_process_resources(p, 1);

    _sti();
    
    // PASO 5: Si matamos el proceso actual, forzar context switch
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

int kill_foreground_processes() {
    // Obtener el proceso actualmente en ejecución
    Process* current = get_current_process();
    
    if (current == NULL) {
        return -1; // No hay proceso corriendo
    }
    
    
    // Verificar si el proceso actual está en foreground y no es idle o init
    if (current->ground == FOREGROUND && current->pid != 0 && current->pid != 1) {
        // Matar el proceso actual
        return kill_process(current->pid);
    }
    
    return 0; // No se mató ningún proceso (no estaba en foreground)
}

int wait_child(int child_pid) {
    // Verificar que el proceso hijo exista
    Process *child = get_process(child_pid);
    if (child == NULL) {
        return -1; // El hijo ya terminó o no existe
    }
    
    // Construir el nombre del semáforo para este proceso hijo
    char *sem_name = build_wait_semaphore_name(child_pid);
    if (!sem_name) {
        return -1;
    }
    
    // Abrir el semáforo (ya fue creado en create_process)
    Sem s = semOpen(sem_name, 0);
    if (s == NULL) {
        mm_free(sem_name);
        return -1;
    }
    
    // Esperar a que el hijo señalice su terminación
    // semWait bloqueará hasta que el hijo haga semPost
    int result = semWait(s);
    
    // Cerrar el semáforo
    semClose(s);
    mm_free(sem_name);
    
    if (result != 0) {
        return -1;
    }
    
    // Remover el hijo de la lista de children del padre
    Process *cur = get_current_process();
    if (cur) {
        for (int i = 0; i < cur->child_count; ++i) {
            if (cur->children[i] == child_pid) {
                // Shift remaining children
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
            remove_from_process_table(p->pid);
            free_process_resources(p, 0);
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
        panic("process_terminator: Current process is NULL");
    }

    int pid = cur->pid;
    
    // PASO 1: Cerrar los file descriptors PRIMERO (sin locks críticos)
    uint8_t r = cur->targetByFd[READ_FD];
    uint8_t w = cur->targetByFd[WRITE_FD];
    if (r != STDIN && r != STDOUT) {
        detachReader(r, pid);
        closePipe(r);
    }
    if (w != STDIN && w != STDOUT) {
        detachWriter(w, pid);
        if (w != r) {
            closePipe(w);
        }
    }
    
    // PASO 2: Reparentar los hijos del proceso que va a morir
    reparent_children(pid);
    
    // PASO 3: Señalizar la terminación al proceso padre VÍA SEMÁFORO
    // Esto debe hacerse ANTES de adquirir locks críticos para evitar deadlock
    char *sem_name = build_wait_semaphore_name(pid);
    if (sem_name) {
        // Abrir el semáforo y hacer post para despertar al padre
        Sem s = semOpen(sem_name, 0);
        if (s) {
            semPost(s);
            semClose(s);
        }
        mm_free(sem_name);
    }
    
    // PASO 4: Ahora sí, adquirir locks y marcar como TERMINATED
    _cli();
    cur->state = TERMINATED;
    remove_process_from_scheduler(cur);
    if (running_process == cur) {
        running_process = NULL;
    }
    if (shell_proc == cur) {
        shell_proc = NULL;
    }
    _sti();
    
    // PASO 5: Yield y halt - el proceso ya no debería ejecutarse más
    yield_cpu();
    
    for(;;) _hlt();
}
