#include <fonts.h>
#include <idtLoader.h>
#include <lib.h>
#include <moduleLoader.h>
#include <sound.h>
#include <stdint.h>
#include <string.h>
#include <syscallDispatcher.h>
#include <video.h>
#include <test.h>
#include <alloc.h>

#include "process.h"
#include "scheduler.h"
#include "queue.h"
#include "process.h"
#include "memory_manager.h"

extern void _sti(); // (De interrupts.asm) Habilita interrupciones
extern void _hlt(); // (De interrupts.asm) Detiene la CPU hasta la próxima interrupción

extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;
//static void *const HEAP_END_ADDRESS = (void *)0x10600000; //256mb
static void *const HEAP_END_ADDRESS = (void *)0x20000000;  //512mb
static void *const shellModuleAddress = (void *)0x400000;
static void *const snakeModuleAddress = (void *)0x500000;

typedef int (*EntryPoint)();


void clearBSS(void *bssAddress, uint64_t bssSize) {
  memset(bssAddress, 0, bssSize);
}

void *getStackBase() {
  return (void *)((uint64_t)&endOfKernel +
                  PageSize * 8       // The size of the stack itself, 32KiB
                  - sizeof(uint64_t) // Begin at the top of the stack
  );
}

void *initializeKernelBinary() {
  void *moduleAddresses[] = {
      shellModuleAddress,
      snakeModuleAddress,
  };

  loadModules(&endOfKernelBinary, moduleAddresses);

  clearBSS(&bss, &endOfKernel - &bss);

  return getStackBase();
}

int main() {
  load_idt();

  setFontSize(4);
  void *heapStartOriginal = (void *)((uint64_t)&endOfKernel + PageSize * 8);

  uint64_t base = ((uint64_t)heapStartOriginal + 0x1FFFFF) & ~((uint64_t)0x1FFFFF);
  if (base < 0x600000) { // Nos aseguramos de saltar los módulos
      base = 0x600000;
  }
  
  void *heapStart = (void*) base;
  uint64_t heapSize = (uint64_t)HEAP_END_ADDRESS - (uint64_t)heapStart;
  

  mm_init(heapStart, heapSize);

  if (init_scheduler() != 0) {
    panic("Failed to initialize scheduler");
    return 1;
  }


  _sti();
  print("Kernel IDLE. Waiting for interrupt...\n");
  while (1) {
     _hlt(); // Espera la próxima interrupción
  }
  __builtin_unreachable();
  return 0;
}
