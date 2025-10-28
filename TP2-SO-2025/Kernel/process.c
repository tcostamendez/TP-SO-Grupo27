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
 * @param stack_top Puntero al tope del stack (ej: stackBase + STACK_SIZE)
 * @param rip       Puntero a la función a ejecutar (el entry point)
 * @param argc      Argument count
 * @param argv      Argument vector
 * @param terminator Puntero a la función (wrapper) que debe ejecutarse
 * cuando 'rip' retorne.
 * @return El nuevo valor de RSP para este contexto.
 */
extern uint64_t stackInit(uint64_t stack_top, ProcessEntryPoint rip, void (*terminator)(), int argc, char*argv[]);

void reap_terminated_processes(void);
/**
 * @brief Fuerza una interrupción de timer (int 0x20).
 * (Esta función ya la tienes en 'interrupts.asm' (o similar)).
 */
extern void _force_scheduler_interrupt();

static int pid = 0;

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

void process_terminator(void) {
    // Comentado para reducir spam
    // print("[process_terminator] entry\n");
    Process *cur = get_current_process();
    if (cur == NULL) {
        // Shouldn't happen, pero en caso de problemas: halt CPU
        for(;;) _hlt();
    }

    int pid = cur->pid;
    print("[process_terminator] pid="); printDec(pid); print("\n");

    // Preparar nombre del semáforo ANTES de _cli
    char *pid_str = num_to_str((uint64_t)pid);
    char *sem_name = NULL;
    if (pid_str) {
        int name_len = strlen("wait_") + strlen(pid_str) + 1;
        sem_name = mm_alloc(name_len);
        if (sem_name) {
            my_strcpy(sem_name, "wait_");
            catenate(sem_name, pid_str);
        }
    }

    _cli();
    
    // Marcar como TERMINATED
    cur->state = TERMINATED;
    // print("[process_terminator] marked TERMINATED\n");
    
    // CRÍTICO: Remover de todas las colas del scheduler
    extern QueueADT ready_queue;
    extern QueueADT blocked_queue;
    extern Process* running_process;
    
    queueRemove(ready_queue, &cur);
    queueRemove(blocked_queue, &cur);
    if (running_process == cur) {
        running_process = NULL;
    }
    // print("[process_terminator] removed from queues\n");
    
    _sti();
    
    // Notificar al padre mediante semPost (FUERA de _cli)
    if (sem_name) {
        Sem s = semOpen(sem_name, 0);
        if (s) {
            // print("[process_terminator] posting to semaphore\n");
            semPost(s);
            semClose(s);
        }
        mm_free(sem_name);
    }
    
    // print("[process_terminator] antes _force_scheduler_interrupt()\n");
    // Forzar cambio de contexto - NUNCA debería retornar
    _force_scheduler_interrupt();
    
    // Si por alguna razón llegamos aquí, loop infinito
    print("[process_terminator] ERROR: returned from interrupt (should never happen)\n");
    for(;;) _hlt();
}

Process* create_process(int argc, char** argv, ProcessEntryPoint entry_point, int priority) {
    _cli(); // Proteger asignación de PID
    
    Process* p = (Process*) mm_alloc(sizeof(Process));
    if (p == NULL) {
        return NULL;
    }

    p->stackBase = mm_alloc(PROCESS_STACK_SIZE);
    if (p->stackBase == NULL) {
        mm_free(p);
        _sti();
        return NULL;
    }

    p->pid = pid++;
    print("[create_process] assigned PID=");
    printDec(p->pid);
    print("\n");
    
    _sti(); // Permitir interrupciones de nuevo
    
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
    /*
    print("[create_process] calling stackInit, stack_top=");
    printHex(stack_top);
    print(", entry=");
    printHex((uint64_t)p->rip);
    print("\n");
    */
    
    p->rsp = stackInit(stack_top, p->rip, process_terminator, p->argc, p->argv);
    
    /*
    print("[create_process] stackInit returned rsp=");
    printHex(p->rsp);
    print("\n");
    */
    
    // VALIDACIÓN: verificar que RSP esté dentro del stack
    uint64_t stack_bottom = (uint64_t)p->stackBase;
    if (p->rsp < stack_bottom || p->rsp >= stack_top) {
        print("[create_process] ERROR: RSP fuera de rango! stack_bottom=");
        printHex(stack_bottom);
        print(", rsp=");
        printHex(p->rsp);
        print(", stack_top=");
        printHex(stack_top);
        print("\n");
        mm_free(p->stackBase);
        mm_free(p);
        return NULL;
    }
    
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
    print("[kill] entry pid="); printDec(pid); print("\n");
    Process* p = get_process(pid);
    if (p == NULL) {
        print("[kill] pid not found\n");
        return -1; // Proceso no existe
    }
    
    extern Process* get_running_process();
    Process* running = get_running_process();

    _cli();
    print("[kill] after cli, state="); printDec(p->state); print("\n");
    
    // Si el proceso a matar es el que se está ejecutando AHORA:
    // Solo marcar TERMINATED y forzar cambio de contexto
    if (running == p) {
        print("[kill] killing self - marking TERMINATED\n");
        p->state = TERMINATED;
        
        // CRÍTICO: Remover de scheduler antes de notificar
        extern QueueADT ready_queue;
        extern QueueADT blocked_queue;
        extern Process* running_process;
        
        queueRemove(ready_queue, &p);
        queueRemove(blocked_queue, &p);
        if (running_process == p) {
            running_process = NULL;
        }
        print("[kill] removed from scheduler\n");
        
        // Notificar al padre mediante semPost en "wait_<pid>"
        char *pid_str = num_to_str((uint64_t)pid);
        if (pid_str) {
            int name_len = strlen("wait_") + strlen(pid_str) + 1;
            char *name = mm_alloc(name_len);
            if (name) {
                my_strcpy(name, "wait_");
                catenate(name, pid_str);
                Sem s = semOpen(name, 0);
                if (s) {
                    print("[kill] posting to wait sem\n");
                    semPost(s);
                    semClose(s);
                }
                mm_free(name);
            }
        }
        
        _sti();
        print("[kill] about to _force_scheduler_interrupt()\n");
        _force_scheduler_interrupt();
        
        // NUNCA debería llegar aquí
        print("[kill] ERROR: returned from interrupt\n");
        for(;;) _hlt();
    }

    // Para procesos que no son el running, limpiar de forma segura
    print("[kill] killing other process\n");
    p->state = TERMINATED;
    
    // Notificar al padre
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

    extern void remove_process_from_scheduler(Process* p);
    print("[kill] removing from scheduler\n");
    remove_process_from_scheduler(p);
    
    if (p->stackBase != NULL) {
        mm_free(p->stackBase);
        p->stackBase = NULL;
    }

    remove_from_process_table(pid);
    mm_free(p);

    _sti();
    print("[kill] done\n");
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
    if (!name) {
        //habria que liberar
        return -1;
    }
    my_strcpy(name, "wait_");
    catenate(name, pid_str);

    Sem s = semOpen(name, 0);
    if (!s) return -1;
    int r = semWait(s);
    semClose(s);
    
    // IMPORTANTE: Remover este hijo del array de children del padre
    // para que wait_all_children() no intente esperarlo de nuevo
    Process *cur = get_current_process();
    if (cur) {
        for (int i = 0; i < cur->child_count; ++i) {
            if (cur->children[i] == child_pid) {
                // Shift todos los hijos subsiguientes una posición hacia la izquierda
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
                if (p->stackBase) { mm_free(p->stackBase); p->stackBase = NULL; }
                remove_from_process_table(p->pid);
                mm_free(p);
            }
        }
    }
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
