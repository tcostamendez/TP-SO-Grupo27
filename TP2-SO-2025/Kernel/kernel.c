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

void test_proc_A(int argc, char** argv) {
  const char *msg = (argc > 0) ? argv[1] : "A";
  while(1){
    print(msg);
      for(volatile int i=0; i<1000000; i++);
  }
}
void test_proc_B(int argc, char** argv) {
  const char *msg = (argc > 0) ? argv[1] : "B";
  while(1){
    print(msg);
    for(volatile int i=0; i<1000000; i++);
  }
}


int main() {
  load_idt();


  void *heapStartOriginal = (void *)((uint64_t)&endOfKernel + PageSize * 8);

  uint64_t base = ((uint64_t)heapStartOriginal + 0x1FFFFF) & ~((uint64_t)0x1FFFFF);
  if (base < 0x600000) { // Nos aseguramos de saltar los módulos
      base = 0x600000;
  }
  
  void *heapStart = (void*) base;
  uint64_t heapSize = (uint64_t)HEAP_END_ADDRESS - (uint64_t)heapStart;
  
  print("Heap Start: "); printHex((uint64_t)heapStart); print("\n");
  print("Heap Size:  "); printDec(heapSize); print(" bytes\n");
  
  mm_init(heapStart, heapSize);

  init_scheduler();

  char* arga[] ={"A","TOMAS\n"};
  char* argb[]={"B","SANTI\n"};

  //create_process(2, arga, test_proc_A);
  create_process(2,argb, test_proc_B);

  _sti();
  print("Kernel IDLE. Waiting for interrupt...\n");
  while (1) {
     _hlt(); // Espera la próxima interrupción
  }
  __builtin_unreachable();
  return 0;
}

//funciones para calcular el heap end address segun gemini
//para usarlo hay que agregar el llamado en main y cambiar HEAP_END_ADDRESS

// #include <stdint.h>

// // Definimos la estructura de una entrada del mapa E820
// // __attribute__((packed)) le dice al compilador que no agregue
// // "padding" extra, para que la estructura coincida exactamente
// // con los datos en memoria.
// typedef struct {
//     uint64_t base_address;  // 8 bytes (Dirección de inicio)
//     uint64_t length;        // 8 bytes (Tamaño de la región)
//     uint32_t type;          // 4 bytes (Tipo 1 = Usable)
    
//     // El manual dice 24 bytes. 8+8+4 = 20. 
//     // Faltan 4 bytes, que son los "atributos extendidos" de ACPI.
//     // Los incluimos para que el tamaño sea correcto, aunque no los usemos.
//     uint32_t extended_attributes; 
// } __attribute__((packed)) E820Entry;

// #define E820_MAP_START ((E820Entry *)0x4000)
// #define E820_TYPE_USABLE 1

// /**
//  * @brief Lee el mapa E820 para encontrar el final de la memoria física usable.
//  * @return La dirección física más alta que se puede usar (fin del último bloque usable).
//  */
// uint64_t detect_memory_end() {
//     E820Entry *entry = E820_MAP_START;
//     uint64_t max_memory_end = 0;

//     // Iteramos mientras la entrada no sea "en blanco" (length > 0)
//     while (entry->length > 0) {
        
//         // Buscamos solo las regiones de Tipo 1 (Usable)
//         if (entry->type == E820_TYPE_USABLE) {
            
//             // Calculamos dónde termina esta región
//             uint64_t region_end = entry->base_address + entry->length;

//             // Guardamos la dirección final más alta que hayamos encontrado
//             if (region_end > max_memory_end) {
//                 max_memory_end = region_end;
//             }
//         }

//         // Avanzamos al siguiente registro.
//         // Como el puntero 'entry' es del tipo E820Entry*,
//         // 'entry++' avanza automáticamente 24 bytes.
//         entry++;
//     }

//     return max_memory_end;
// }
