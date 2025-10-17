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
#include "process.h"
#include "scheduler.h"
#include <video.h>

extern void _sti(); // (De interrupts.asm) Habilita interrupciones
extern void _hlt(); // (De interrupts.asm) Detiene la CPU hasta la próxima interrupción
// extern uint8_t text;
// extern uint8_t rodata;
// extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;
// static void * const HEAP_END_ADDRESS = (void *)0x10600000; //256mb
static void * const HEAP_END_ADDRESS = (void *)0x800000; // 8MB
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

void test_proc_A(void) {
    while(1) {
       //print("A");
        for (volatile int j = 0; j < 500000; j++);
    }
}
void test_proc_B(void) {
    while(1) {
        //print("B");
        for (volatile int j = 0; j < 500000; j++);
    }
}

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
  
  
  print("Heap Start: "); printHex((uint64_t)heapStart); print("\n");
  print("Heap Size:  "); printDec(heapSize); print(" bytes\n");
  
  
  // 3. Inicializamos el gestor.
  //    - first_fit tomará toda la memoria que le demos.
  //    - buddy_system encontrará la potencia de 2 más grande dentro de heapSize.
  mm_init(heapStart, heapSize);
  //lo comento y abajo pongo lo que me paso gemini
  // setFontSize(2);
  // // Iniciar la shell
  // ((EntryPoint)shellModuleAddress)();

  // --- INICIA EL NUEVO CÓDIGO ---

    // 4. Inicializar los nuevos subsistemas
    init_pcb();
    init_scheduler();

    // 5. Crear el primer proceso (la shell)
    //    Este será el proceso con PID 1.
    // char *shell_argv[] = {"shell"};
    // int shell_argc = 1;
    // int shell_pid = create_process(
    //     "shell",                  // Nombre (para debugging)
    //     (EntryPoint)shellModuleAddress, // Puntero a la función
    //     shell_argc,               // argc
    //     shell_argv                // argv
    // );

    create_process("proc_A", test_proc_A);
    create_process("proc_B", test_proc_B);
    // --- FIN DEBUG ---

    // if (shell_pid == -1) {
    //     // Pánico: No se pudo crear el proceso de la shell
    //     // (Probablemente falló mm_alloc)
    //     // (Aquí podrías usar tu función panic() si tienes una)
    //     return -1; 
    // }

    // 6. Habilitar interrupciones
    // ¡A partir de este punto, el 'timer interrupt' (irq00) comenzará a ejecutarse!
    _sti();

    // 7. Lanzar el "proceso" IDLE del kernel.
    // En lugar de llamar a la shell, el kernel se pone a "dormir" (hlt)
    // en un bucle infinito. La primera interrupción de timer (int 0x20)
    // nos sacará de aquí, llamará a schedule(), y schedule() elegirá
    // a la shell (PID 1) como el primer proceso a ejecutar.

    print("Kernel IDLE. Waiting for interrupt...\n");
    while (1) {
      //print("k");
      _hlt(); // Espera la próxima interrupción
    }
    __builtin_unreachable();
    return 0;
}
