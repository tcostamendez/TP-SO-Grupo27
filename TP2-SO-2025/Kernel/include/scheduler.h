#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "queue.h"
#include "array.h"
typedef struct Process Process;

#define AGING_THRESHOLD 50        
#define AGING_BOOST 1           
extern ArrayADT process_priority_table;
extern Process* running_process;
extern Process* idle_proc;
extern Process* shell_proc;

/**
 * @brief Initialize the scheduler.
 * For Round Robin with priorities, initializes ready queues and state.
 */
void init_scheduler();

/**
 * @brief Add a process to the scheduler READY queue.
 * @param p Process control block pointer.
 */
void add_to_scheduler(Process *p);

/**
 * @brief Remove a specific process from the scheduler queues.
 * @param p Process to remove from its priority queue.
 */
void remove_process_from_scheduler(Process* p);

/**
 * @brief Core scheduling function.
 * Called from the ASM timer handler (irq00Handler).
 * - Saves current RSP for the interrupted RUNNING process and requeues it.
 * - Picks the next READY process (Round Robin with priorities).
 * - Marks it RUNNING and returns its saved RSP.
 * @param current_rsp RSP of the interrupted process (from ASM).
 * @return RSP of the next process to run.
 */
uint64_t schedule(uint64_t current_rsp);

/**
 * @brief Get the currently running process.
 * @return Pointer to the RUNNING process.
 */
Process* get_running_process();

/**
 * @brief Get the idle process.
 * @return Pointer to the idle process.
 */
Process* get_idle_process();

/**
 * @brief Get the shell process, if present.
 * @return Pointer to the shell process, or NULL if not available.
 */
Process* get_shell_process();

/**
 * @brief Unblock a process and move it to the READY queue.
 * Changes its state to READY and re-enqueues it according to priority.
 * @param p Process to unblock.
 */
int unblock_process(Process* p);

/**
 * @brief Block a process.
 * Removes it from its priority queue and marks as BLOCKED.
 * When unblocked it must be re-added to the correct queue.
 * @param p Process to block.
 */
int block_process(Process* p);

int get_running_pid();

#endif // SCHEDULER_H