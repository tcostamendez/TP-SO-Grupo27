// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
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

// Global process table - stores pointers to all processes
static Process* process_table[MAX_PROCESSES] = {NULL};

/**
 * @brief Build the semaphore name used to synchronize wait() on a PID.
 * @param pid PID of the process.
 * @return Dynamically allocated semaphore name, or NULL on failure.
 *         Caller must free it with mm_free().
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
    
    for (int i = 0; i < dying->child_count; i++) {
        int child_pid = dying->children[i];
        Process* child = get_process(child_pid);
        
        if (child != NULL && child->state != TERMINATED) {
            child->ppid = 1;
            
            if (init->child_count < MAX_CHILDREN) {
                init->children[init->child_count++] = child_pid;
            }
        }
    }

    dying->child_count = 0;
}

/**
 * @brief Free all memory and resources associated with a process.
 * Centralizes process teardown and cleanup logic.
 * @param p Process to clean up.
 * @param remove_from_scheduler Whether the process must be removed from scheduler.
 */
static void free_process_resources(Process* p, int remove_from_scheduler) {
    if (p == NULL) {
        return;
    }
    if (p->argv != NULL) {
        for (int i = 0; i < p->argc; i++) {
            if (p->argv[i] != NULL) {
                mm_free(p->argv[i]);
            }
        }
        mm_free(p->argv);
    }
    
    if (p->stackBase != NULL) {
        mm_free(p->stackBase);
    }

    if (remove_from_scheduler) {
        remove_process_from_scheduler(p);
    }
    
    mm_free(p);
}

/**
 * @brief Assign a PID by scanning the process table.
 * Finds an empty slot (NULL) or one with a TERMINATED process.
 * @return Assigned PID [0..MAX_PROCESSES-1], or -1 if the table is full.
 */
static int assign_pid() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] == NULL || process_table[i]->state == TERMINATED) {
            return i;
        }
    }
    return -1; // Table full
}

/**
 * @brief Add a process into the global process table.
 * @param p Process pointer.
 * @return 0 on success, -1 if the table is full.
 */
static int add_to_process_table(Process* p) {
    if (p == NULL || p->pid < 0 || p->pid >= MAX_PROCESSES) {
        return -1;
    }
    process_table[p->pid] = p;
    return 0;
}

/**
 * @brief Remove a process from the global table.
 * @param pid PID of the process to remove.
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
    p->wait_ticks = 0;                 

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
        char *sem_name = build_wait_semaphore_name(p->pid);
        if (sem_name) {
            semOpen(sem_name, 0);
            mm_free(sem_name);
        }
    }
    
    p->targetByFd[READ_FD] = targets[0];
    p->targetByFd[WRITE_FD] = targets[1];
    p->targetByFd[ERR_FD] = targets[2];

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

void yield_cpu(void) {
    Process* current = get_current_process();
    if (current != NULL && current->state == RUNNING) {
        _cli();
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
    if (pid == 0 || pid == 1) {
        return -1;
    }
    
    Process* p = get_process(pid);
    if (p == NULL) {
        return -1;
    }
    
    Process* running = get_running_process();
    
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
    
    reparent_children(pid);
    
    char *sem_name = build_wait_semaphore_name(pid);
    if (sem_name) {
        Sem s = semOpen(sem_name, 0);
        if (s) {
            semPost(s);
            semClose(s);
        }
        mm_free(sem_name);
    }

    _cli();
    p->state = TERMINATED;
    remove_from_process_table(pid);
    if (shell_proc == p) {
        shell_proc = NULL;
    }
    free_process_resources(p, 1);
    _sti();
    
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
    Process* current = get_current_process();
    
    if (current == NULL) {
        return -1;
    }
    
    if (current->ground == FOREGROUND && current->pid != 0 && current->pid != 1) {
        return kill_process(current->pid);
    }
    
    return 0;
}

int wait_child(int child_pid) {

    Process *child = get_process(child_pid);
    if (child == NULL) {
        return -1;
    }
    
    char *sem_name = build_wait_semaphore_name(child_pid);
    if (!sem_name) {
        return -1;
    }
    
    Sem s = semOpen(sem_name, 0);
    if (s == NULL) {
        mm_free(sem_name);
        return -1;
    }
    
    int result = semWait(s);
    
    semClose(s);
    mm_free(sem_name);

    reap_terminated_processes();
    
    if (result != 0) {
        return -1;
    }
    
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
            remove_from_process_table(p->pid);
            free_process_resources(p, 0);
        }
    }
}

int ps(ProcessInfo* process_info) {
    if (process_info == NULL) {
        return -1;
    }

    // Initialize all slots with sentinel value (-1) to distinguish from pid 0 (idle process)
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_info[i].pid = -1;
    }

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
    if(info == NULL || pid >= MAX_PROCESSES || process_table[pid] == NULL){
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
    
    reparent_children(pid);

    char *sem_name = build_wait_semaphore_name(pid);
    if (sem_name) {
        Sem s = semOpen(sem_name, 0);
        if (s) {
            semPost(s);
            semClose(s);
        }
        mm_free(sem_name);
    }

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
    
    yield_cpu();
    
    for(;;) _hlt();
}
