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
#include "queue.h"
#include "process.h"

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
  const char *msg = (argc > 0) ? argv[1] : "Tambien estoy Hardoceado";
  while(1){
    print(msg);
      for(volatile int i=0; i<1000000; i++);
  }
}

void test_proc_B(int argc, char** argv) {
  const char *msg = (argc > 0) ? argv[1] : "Estoy hardcodeado";
  while(1){
    print(msg);
      for(volatile int i=0; i<1000000; i++);
  }
}

// Helper function to print process information
void print_process_info(Process* p, void* arg) {
    print("  PID: "); 
    printDec(p->pid);
    print(" | Name: ");
    print(p->argv[0]);
    print(" | Priority: ");
    printDec(p->priority);
    print(" | FG: ");
    print(p->ground == FOREGROUND ? "FOREGROUND" : "BACKGROUND");
    print(" | State: ");
    switch(p->state) {
        case READY: print("READY"); break;
        case RUNNING: print("RUNNING"); break;
        case BLOCKED: print("BLOCKED"); break;
        case TERMINATED: print("TERMINATED"); break;
    }
    print("\n");
}

// Test function to demonstrate process table functionality
void test_process_table() {
    print("\n=== TESTING PROCESS TABLE ===\n");
    
    // Test 1: Process count
    print("\n[TEST 1] Process count: ");
    int count = get_process_count();
    printDec(count);
    print(" processes\n");
    
    // Test 2: List all processes
    print("\n[TEST 2] Listing all processes:\n");
    foreach_process(print_process_info, NULL);
    
    // Test 3: Get specific process
    print("\n[TEST 3] Getting process with PID 1:\n");
    Process* p = get_process(1);
    if (p != NULL) {
        print("  Found: ");
        print(p->argv[0]);
        print("\n");
    } else {
        print("  Not found!\n");
    }
    
    // Test 4: Get current process
    print("\n[TEST 4] Current running process:\n");
    Process* current = get_current_process();
    print("  PID: "); printDec(current->pid);
    print(" Name: "); print(current->argv[0]);
    print("\n");
    
    // Test 5: Change priority
    print("\n[TEST 5] Changing priority of PID 2 from ");
    int old_prio = get_priority(2);
    printDec(old_prio);
    print(" to 3...\n");
    set_priority(2, 3);
    print("  New priority: ");
    printDec(get_priority(2));
    print("\n");
    
    // Test 6: Kill a process
    print("\n[TEST 6] Killing process with PID 3 (proc_C)...\n");
    print("  Process count before: ");
    printDec(get_process_count());
    print("\n");
    
    int kill_result = kill_process(3);
    if (kill_result == 0) {
        print("  Successfully killed process 3\n");
    } else {
        print("  Failed to kill process 3\n");
    }
    
    print("  Process count after: ");
    printDec(get_process_count());
    print("\n");
    
    print("  Trying to get killed process: ");
    Process* killed = get_process(3);
    if (killed == NULL) {
        print("NULL (correctly removed)\n");
    } else {
        print("ERROR: Process still exists!\n");
    }
    
    print("\n[TEST 7] Listing processes after kill:\n");
    foreach_process(print_process_info, NULL);
    
    print("\n=== PROCESS TABLE TESTS COMPLETE ===\n\n");
}

// Test blocking and scheduler queue management
void test_blocking_and_scheduler() {
    print("\n=== TESTING BLOCKING & SCHEDULER ===\n");
    
    // Test 1: Check queue sizes
    print("\n[TEST 1] Initial queue sizes:\n");
    print("  Ready queue: "); printDec(get_ready_process_count()); print("\n");
    print("  Blocked queue: "); printDec(get_blocked_process_count()); print("\n");
    
    // Test 2: Block a process (simulate blocking PID 1)
    print("\n[TEST 2] Simulating block of process 1 (proc_A)...\n");
    Process* p1 = get_process(1);
    if (p1 != NULL) {
        print("  Before: State = ");
        if (p1->state == READY) print("READY");
        else if (p1->state == RUNNING) print("RUNNING");
        else if (p1->state == BLOCKED) print("BLOCKED");
        print("\n");
        
        // Manually change state and move to blocked queue
        _cli();
        block_process(p1);
        _sti();
        
        print("  After: State = ");
        if (p1->state == READY) print("READY");
        else if (p1->state == RUNNING) print("RUNNING");
        else if (p1->state == BLOCKED) print("BLOCKED");
        print("\n");
    }
    
    // Test 3: Check queue sizes after blocking
    print("\n[TEST 3] Queue sizes after blocking:\n");
    print("  Ready queue: "); printDec(get_ready_process_count()); print("\n");
    print("  Blocked queue: "); printDec(get_blocked_process_count()); print("\n");
    
    unblock_process(p1);
    // Test 4: List all processes to see states
    print("\n[TEST 4] All processes after blocking test:\n");
    foreach_process(print_process_info, NULL);
    

    print("\n=== BLOCKING & SCHEDULER TESTS COMPLETE ===\n\n");
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

  // Crear procesos con diferentes prioridades usando la interfaz correcta
  char* arga[] ={"procA", "A"};
  char* argb[]={"procB", "B"};

  create_process(2, arga, test_proc_A, 2);
  create_process(2, argb, test_proc_A, 1);

  // Run process table tests before starting scheduler
  test_process_table();
  
  // Run blocking and scheduler tests
  test_blocking_and_scheduler();

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
