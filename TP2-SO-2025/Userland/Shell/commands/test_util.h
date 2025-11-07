#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdint.h>
#include <libsys.h>

// Macro wrappers for test compatibility
#define my_getpid()                  getMyPid()
#define my_create_process(name, prio, argv) createProcess(1, argv, (void (*)(int, char**))name, prio, (int[]){0, 1, 2}, 0)
#define my_kill(pid)                 killProcess(pid)
#define my_block(pid)                blockProcess(pid)
#define my_unblock(pid)              unblockProcess(pid)
#define my_nice(pid, prio)           setProcessPriority(pid, prio)
#define my_yield()                   yieldCPU()
#define my_wait(pid)                 waitPid(pid)
#define my_sem_open(name, val)       semOpen(name, val)
#define my_sem_wait(sem)             semWait(sem)
#define my_sem_post(sem)             semPost(sem)
#define my_sem_close(sem)            semClose(sem)

uint32_t GetUint();
uint32_t GetUniform(uint32_t max);
uint8_t memcheck(void *start, uint8_t value, uint32_t size);
int64_t satoi(char *str);
void bussy_wait(uint64_t n);
void endless_loop();
void endless_loop_print(uint64_t wait);

#endif // TEST_UTIL_H
