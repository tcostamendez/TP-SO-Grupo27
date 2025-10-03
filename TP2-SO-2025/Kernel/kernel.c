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
#include "memory_manager.h"
#include <alloc.h>

// extern uint8_t text;
// extern uint8_t rodata;
// extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;
static void * const HEAP_END_ADDRESS = (void *)0x10600000; //256mb
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

// int main() {
//   load_idt();

//   void *heapStart = (void *)((uint64_t)&endOfKernel + PageSize * 8);
//   uint64_t heapSize = (uint64_t)HEAP_END_ADDRESS - (uint64_t)heapStart;

//   mm_init(heapStart, heapSize);
//   setFontSize(2);

//   //runTests();
  
//   ((EntryPoint)shellModuleAddress)();

//   __builtin_unreachable();

//   return 0;
// } //este main funciona para first_fit_mm con 128mb

// int main() {
//   load_idt();

//   // El último módulo pre-cargado (Snake) está en 0x500000.
//   // Dejamos un margen de seguridad y empezamos nuestro heap en 6MB (0x600000),
//   // una zona que garantizamos que está libre.
//   void *heapStart = (void *)0x600000;

//   // El tamaño del heap se calcula desde el inicio hasta el final definido.
//   uint64_t heapSize = (uint64_t)HEAP_END_ADDRESS - (uint64_t)heapStart;

//   // Inicializamos el gestor de memoria con esta región de memoria limpia.
//   mm_init(heapStart, heapSize);

//   setFontSize(2);

//   // Iniciar la shell
//   ((EntryPoint)shellModuleAddress)();

//   __builtin_unreachable();
//   return 0;
// } //este test no llega a funcionar con first_fit_mm para 128mb, llegue a probar 120 que funciona

// En kernel.c, reemplazá tu función main por esta

int main() {
  load_idt();

  // 1. Calculamos dónde empieza la memoria libre de forma segura,
  //    justo después del kernel y su stack.
  void *heapStartOriginal = (void *)((uint64_t)&endOfKernel + PageSize * 8);

  // 2. Para el buddy system, necesitamos que la dirección de inicio
  //    y el tamaño sean potencias de dos. Redondeamos hacia ARRIBA
  //    el inicio del heap a la siguiente potencia de 2 más cercana (ej: a 2MB).
  //    Esto le da al buddy un bloque "limpio" y alineado para trabajar.
  //    NOTA: 0x200000 es 2MB. Si &endOfKernel es más grande, se usará un valor mayor.
  uint64_t base = ((uint64_t)heapStartOriginal + 0x1FFFFF) & ~((uint64_t)0x1FFFFF);
  if (base < 0x600000) { // Nos aseguramos de saltar los módulos
      base = 0x600000;
  }
  
  void *heapStart = (void*) base;
  uint64_t heapSize = (uint64_t)HEAP_END_ADDRESS - (uint64_t)heapStart;

  // 3. Inicializamos el gestor.
  //    - first_fit tomará toda la memoria que le demos.
  //    - buddy_system encontrará la potencia de 2 más grande dentro de heapSize.
  mm_init(heapStart, heapSize);

  setFontSize(2);

  // Iniciar la shell
  ((EntryPoint)shellModuleAddress)();

  __builtin_unreachable();

  return 0;
}
