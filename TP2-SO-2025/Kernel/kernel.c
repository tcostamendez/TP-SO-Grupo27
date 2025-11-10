// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <fonts.h>
#include <idtLoader.h>
#include <lib.h>
#include <moduleLoader.h>
#include <sound.h>
#include <stdint.h>
#include <string.h>
#include <syscallDispatcher.h>
#include <video.h>
#include "memory_manager.h"
#include "process.h"
#include "scheduler.h"
#include <video.h>
#include "queue.h"
#include "process.h"
#include "sem.h"
#include "pipe.h"
#include "fd.h"
#include "strings.h"
#include "panic.h"
#include "keyboard.h"

extern void _sti();  
extern void _hlt(); 
extern void _force_scheduler_interrupt();

extern Process* shell_proc;

extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;
static void *const HEAP_END_ADDRESS = (void *)0x20000000;  
void *const shellModuleAddress = (void *)0x400000;

typedef int (*EntryPoint)();

void clearBSS(void *bssAddress, uint64_t bssSize) {
  memset(bssAddress, 0, bssSize);
}

void *getStackBase() {
  return (void *)((uint64_t)&endOfKernel +
                  PageSize * 8       
                  - sizeof(uint64_t) 
  );
}

void *initializeKernelBinary() {
  void *moduleAddresses[] = {
      shellModuleAddress,
  };

  loadModules(&endOfKernelBinary, moduleAddresses);

  clearBSS(&bss, &endOfKernel - &bss);
  return getStackBase();
}

void create_shell(void) {
  char * argv[]= {"shell"};
  int argc = sizeof(argv) / sizeof(argv[0]);
  int targets []= {STDIN, STDOUT, ERR_FD};
  shell_proc = create_process(argc, argv, shellModuleAddress, MIN_PRIORITY, targets, 1);
}

void init_process(int argc, char **argv) {    
    while (1) {
        if (shell_proc == NULL || shell_proc->state == TERMINATED) {
            create_shell();
        }
        
        yield_cpu();
        for (volatile int i = 0; i < 10000000; i++);
    }
}

Process* create_init(void) {
  char * init_argv[] = {"init"};
  int init_argc = sizeof(init_argv) / sizeof(init_argv[0]);
  int init_targets[] = {STDIN, STDOUT, ERR_FD};
  return create_process(init_argc, init_argv, init_process, MIN_PRIORITY, init_targets, 0);
}

int main() {
  load_idt();
  setFontSize(2);

  void *heapStartOriginal = (void *)((uint64_t)&endOfKernel + PageSize * 8);
  uint64_t base = ((uint64_t)heapStartOriginal + 0x1FFFFF) & ~((uint64_t)0x1FFFFF);
  if (base < 0x600000) { 
      base = 0x600000;
  }
  
  void *heapStart = (void*) base;
  uint64_t heapSize = (uint64_t)HEAP_END_ADDRESS - (uint64_t)heapStart;
  
  mm_init(heapStart, heapSize);
  initSemQueue();
  initPipeStorage();
  setNextId(2);
  init_scheduler();
  keyboard_sem_init();
  
  Process* init = create_init();
  if (init == NULL) {
    panic("Failed to create init process");
  }
  
  _sti();

  _force_scheduler_interrupt();

  __builtin_unreachable();
  return 0;
}
