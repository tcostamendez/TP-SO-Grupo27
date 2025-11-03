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
   
    Process *cur = get_current_process();
    if (cur == NULL) {
        for(;;) _hlt();
    }

    int pid = cur->pid;
    print("[process_terminator] pid="); printDec(pid); print("\n");

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
    
    cur->state = TERMINATED;
    
    extern QueueADT ready_queue;
    extern QueueADT blocked_queue;
    extern Process* running_process;
    
    queueRemove(ready_queue, &cur);
    queueRemove(blocked_queue, &cur);
    if (running_process == cur) {
        running_process = NULL;
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
    
    if (sem_name) {
        Sem s = semOpen(sem_name, 0);
        if (s) {
            semPost(s);
            semClose(s);
        }
        mm_free(sem_name);
    }

    _force_scheduler_interrupt();
    
    print("[process_terminator] ERROR: returned from interrupt (should never happen)\n");
    for(;;) _hlt();
}

Process* create_process(int argc, char** argv, ProcessEntryPoint entry_point, int priority) {
    _cli();
    
    Process* p = (Process*) mm_alloc(sizeof(Process));
    if (p == NULL) {
        print("PCB_ALLOC_FAIL\n");
        _sti();
        return NULL;
    }

    p->stackBase = mm_alloc(PROCESS_STACK_SIZE);
    if (p->stackBase == NULL) {
        print("STACK_ALLOC_FAIL\n");
        mm_free(p);
        _sti();
        return NULL;
    }

    p->pid = pid++;
    print("[create_process] assigned PID=");
    printDec(p->pid);
    print("\n");
    
    _sti();
    
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
        p->ppid = 0;
    }
    p->state = READY;
    p->rip = entry_point;
    p->ground = BACKGROUND; 
    p->rbp = 0;              
    
    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
        priority = DEFAULT_PRIORITY;
    }
    p->priority = priority;
    p->quantum_remaining = p->priority + 1;

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

    
    p->rsp = stackInit(stack_top, p->rip, process_terminator, p->argc, p->argv);
    
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
    
    if (add_to_process_table(p) != 0) {
        print("Error adding process to process table\n");
        mm_free(p->stackBase);
        mm_free(p);
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
    
    p->targetByFd[READ_FD] = STDIN;
    p->targetByFd[WRITE_FD] = STDOUT;

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
    p->quantum_remaining = new_priority + 1;
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
    print("[kill] entry pid="); printDec(pid); print("\n");
    Process* p = get_process(pid);
    if (p == NULL) {
        print("[kill] pid not found\n");
        return -1;
    }
    
    extern Process* get_running_process();
    Process* running = get_running_process();

    _cli();
    print("[kill] after cli, state="); printDec(p->state); print("\n");

    if (running == p) {
        print("[kill] killing self - marking TERMINATED\n");
        p->state = TERMINATED;
        
        extern QueueADT ready_queue;
        extern QueueADT blocked_queue;
        extern Process* running_process;
        
        queueRemove(ready_queue, &p);
        queueRemove(blocked_queue, &p);
        if (running_process == p) {
            running_process = NULL;
        }
        print("[kill] removed from scheduler\n");
        
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
        
        print("[kill] ERROR: returned from interrupt\n");
        for(;;) _hlt();
    }

    print("[kill] killing other process\n");
    p->state = TERMINATED;
    
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
    
    if (p) {
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
    }
    
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

int wait_child(int child_pid) {
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
    if (!s) return -1;
    int r = semWait(s);
    semClose(s);
    
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
    char ** ans = mm_alloc(sizeof(char*) * 7);
    char * name = mm_alloc(16);
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
