#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h> 
#include "queue.h"
#include "scheduler.h"
#include "strings.h"
#include "panic.h"

// Default per-process stack size (8 KiB)
#define PROCESS_STACK_SIZE (1024 * 8) 

#define MAX_PROCESSES 64
#define MAX_CHILDREN 32

#define MAX_PROCESS_NAME 32

#define MIN_PRIORITY 0
#define MAX_PRIORITY 3
#define DEFAULT_PRIORITY 0

#define DEFAULT_QUANTUM 1

#define INIT_PID 1

#define FOREGROUND 1
#define BACKGROUND 0

typedef void (*ProcessEntryPoint)(int argc, char**argv);

typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED 
} ProcessState;

/**
 * @brief Process Control Block (PCB)
 * Holds all per-process state needed by the kernel scheduler and services.
 */
typedef struct Process {
    int pid;                    
    int ppid;                 
    ProcessState state;    
    int argc;
    char ** argv;
    uint64_t rsp;
    void *stackBase;
    ProcessEntryPoint rip;  
    int priority;             
    int original_priority;      
    int quantum_remaining;    
    int wait_ticks;             
    
    int ground;               
    uint64_t rbp;             
    uint8_t targetByFd[3];
    int children[MAX_CHILDREN];
    int child_count;          
} Process;

typedef struct ProcessInfo {
    int pid;
    int ppid;
    char name[MAX_PROCESS_NAME];
    ProcessState state;

    uint64_t rsp;
    void *stackBase;
    
    int priority;
    
    int ground; 
} ProcessInfo;

/**
 * @brief Initialize global PCB/process subsystem.
 * Must be called once during kernel initialization.
 */
void init_pcb(void);

/**
 * @brief Create a new process.
 * 1) Allocates a stack (via mm_alloc).
 * 2) Allocates and initializes a Process structure.
 * 3) Builds the initial fake stack (stackInit in ASM).
 * 4) Enqueues the process into the scheduler.
 *
 * @param argc Argument count for the entry point.
 * @param argv Argument vector for the entry point. Strings are copied.
 * @param entry_point Function to run as process entry.
 * @param priority Initial process priority [MIN_PRIORITY..MAX_PRIORITY].
 * @param targets I/O targets by FD index (READ_FD, WRITE_FD, ERR_FD).
 * @param hasForeground Non-zero for foreground, zero for background.
 * @return Pointer to the created Process, or NULL on error.
 */
Process* create_process(int argc, char** argv, ProcessEntryPoint entry_point, int priority, int targets[], int hasForeground);

/**
 * @brief Get a process PID from its descriptor.
 * @param p Process pointer.
 * @return PID of the process.
 */
int get_pid(Process* p);

/**
 * @brief Voluntarily yield the CPU.
 * Triggers a scheduler interrupt.
 */
void yield_cpu(void);


/**
 * @brief Get a process by PID.
 * @param pid PID to look up.
 * @return Pointer to Process, or NULL if it does not exist.
 */
Process* get_process(int pid);

/**
 * @brief Change the priority of a process.
 * @param pid Process PID.
 * @param new_priority New priority [MIN_PRIORITY..MAX_PRIORITY].
 * @return 0 on success, -1 on error.
 */
int set_priority(int pid, int new_priority);

/**
 * @brief Get the priority of a process.
 * @param pid Process PID.
 * @return Priority value, or -1 if process does not exist.
 */
int get_priority(int pid);

/**
 * @brief Get the currently running process.
 * @return Pointer to the RUNNING process.
 */
Process* get_current_process();

/**
 * @brief Count processes in the system.
 * @return Number of active processes (READY, RUNNING, BLOCKED).
 */
int get_process_count();

/**
 * @brief Iterate over all processes.
 * @param callback Function called for each non-NULL process.
 * @param arg Opaque user argument passed to callback.
 */
void foreach_process(void (*callback)(Process* p, void* arg), void* arg);

/**
 * @brief Populate a snapshot of processes into the provided array.
 * @param process_info Buffer of size MAX_PROCESSES to be filled with process info.
 * @return 0 on success, -1 on error.
 */
int ps(ProcessInfo* process_info); 

/**
 * @brief Terminate a process by PID.
 * Frees resources and marks the process as TERMINATED.
 * @param pid PID of the process to terminate.
 * @return 0 on success, -1 if process does not exist.
 */
int kill_process(int pid);

/**
 * @brief Set whether a process runs in foreground or background.
 * @param pid Process PID.
 * @param ground FOREGROUND or BACKGROUND.
 * @return 0 on success, -1 if process does not exist.
 */
int set_ground(int pid, int ground);

/**
 * @brief Get whether a process runs in foreground or background.
 * @param pid Process PID.
 * @return FOREGROUND or BACKGROUND, or -1 if it does not exist.
 */
int get_ground(int pid);

/**
 * @brief Kill the current process if it is in foreground.
 * Typically used to handle Ctrl+C.
 * @return 0 if killed or not in foreground, -1 on error.
 */
int kill_foreground_processes();

/**
 * @brief Wait for a specific child to terminate.
 * @param child_pid Child PID to wait for.
 * @return 0 on success, -1 on error.
 */
int wait_child(int child_pid);

/**
 * @brief Wait for all children of the current process to terminate.
 * @return 0 on success, -1 on error.
 */
int wait_all_children(void);

/**
 * @brief Get process info for a specific PID.
 * @param info Output structure to fill.
 * @param pid PID to query.
 * @return 1 on success, -1 on error.
 */
int get_process_info(ProcessInfo * info, int pid);

/**
 * @brief Reap (free) all processes already marked as TERMINATED.
 */
void reap_terminated_processes(void);

/**
 * @brief Terminator routine invoked when a process returns from its entry point.
 * Performs cleanup and yields CPU forever.
 */
void process_terminator(void);
#endif 