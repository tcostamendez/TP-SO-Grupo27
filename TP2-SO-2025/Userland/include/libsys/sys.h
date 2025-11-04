#ifndef _SYS_H_
#define _SYS_H_

#include <stdint.h>
#include <stddef.h>
#include <syscalls.h>


void startBeep(uint32_t nFrequence);
void stopBeep(void);
void setTextColor(uint32_t color);
void setBackgroundColor(uint32_t color);
uint8_t increaseFontSize(void);
uint8_t decreaseFontSize(void);
uint8_t setFontSize(uint8_t size);
void getDate(int * hour, int * minute, int * second);
void clearScreen(void);


void drawCircle(uint32_t color, long long int topleftX, long long int topLefyY, long long int diameter);
void drawRectangle(uint32_t color, long long int width_pixels, long long int height_pixels, long long int initial_pos_x, long long int initial_pos_y);
void fillVideoMemory(uint32_t hexColor);
int32_t exec(int32_t (*fnPtr)(void));
int32_t execProgram(int32_t (*fnPtr)(void));
void registerKey(enum REGISTERABLE_KEYS scancode, void (*fn)(enum REGISTERABLE_KEYS scancode));
void clearInputBuffer(void);
int getWindowWidth(void);
int getWindowHeight(void);
void sleep(uint32_t milliseconds);
int32_t getRegisterSnapshot(int64_t * registers);
int32_t getCharacterWithoutDisplay(void);

// Memory management functions
void* allocMemory(size_t size);
void freeMemory(void* ptr);

// Process management functions
int createProcess(int argc, char** argv, void (*entry_point)(int, char**), int priority, int* targets, int hasForeground);
int getMyPid(void);
int killProcess(int pid);
void setProcessPriority(int pid, int priority);
void listProcesses(void);
void blockProcess(int pid);
void unblockProcess(int pid);
void yieldCPU(void);
int waitPid(int pid);
int waitForChildren(void);
int sys_get_process_info(ProcessInfo* info, int pid);

// Pipe helpers (userland wrappers)
int pipeOpen(void);
int pipeAttach(uint8_t id);
int pipeClose(uint8_t id);
int setReadTarget(uint8_t id);
int setWriteTarget(uint8_t id);

// Semaphore helpers (opaque handle)
typedef void* sem_t;
sem_t semOpen(const char *name, uint16_t initialValue);
int   semClose(sem_t s);
int   semWait(sem_t s);
int   semPost(sem_t s);

#endif