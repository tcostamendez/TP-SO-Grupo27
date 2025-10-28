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
#include "sem.h"
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

void test_proc(int argc, char** argv) {
  const char *msg = (argc > 0) ? argv[1] : "Tambien estoy Hardoceado";
  // while(1){
  //   print(msg);
  //     for(volatile int i=0; i<1000000; i++);
  // }
  for(int j=0; j < 25; j++){
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

// ... (justo antes de tu función main() en kernel.c)

/**
 * @brief Proceso HIIJO de prueba.
 * Simula trabajo durante unos segundos y luego termina.
 */
void child_process_test(int argc, char** argv) {
    const char *name = (argc > 0) ? argv[0] : "Child";
    int pid = get_current_process()->pid; // Asumiendo que get_current_pid() es accesible

    print("  [Hijo "); print(name); print(" - PID "); printDec(pid); print("]: Iniciando.\n");
    
    // Simular trabajo (5 iteraciones de espera)
    for (int j = 0; j < 5; j++) {
        print("  [Hijo "); print(name); print("]: trabajando... ("); printDec(j+1); print("/5)\n");
        
        // Bucle de espera (busy-wait) para simular trabajo
        for (volatile int i = 0; i < 90000000; i++); 
    }
    
    print("  [Hijo "); print(name); print(" - PID "); printDec(pid); print("]: Trabajo completo. Terminando.\n");
    //print("[hijo] antes de intentar llamar a process_terminator directamente\n");
    //process_terminator(pid);
    //print("[hijo] volvimos tras llamar process_terminator (esto NO debería verse)\n");
    // Al retornar, el wrapper 'process_terminator' llamará a kill_process(),
    // lo que debe disparar el sem_post() en "wait_<pid>" y despertar al padre.
}

/**
 * @brief Proceso PADRE de prueba.
 * Lanza hijos y espera a que terminen usando wait_child y wait_all_children.
 */
void parent_process_test(int argc, char** argv) {
    int parent_pid = get_current_process()->pid;
    print("[Padre - PID "); printDec(parent_pid); print("]: Iniciando prueba de 'wait'.\n\n");

    // --- Prueba 1: wait_child() (esperar a un hijo específico) ---
    print("[Padre]: Lanzando Hijo A...\n");
    char* arg_a[] = {"HijoA"};
    Process* child_a_proc = create_process(1, arg_a, child_process_test, 0);
    
    if (child_a_proc == NULL) {
        print("[Padre]: ERROR al crear Hijo A.\n");
        return;
    }
    int child_a_pid = child_a_proc->pid;
    print("[Padre]: Hijo A creado con PID: "); printDec(child_a_pid); print("\n");

    print("[Padre]: ==> Llamando a wait_child("); printDec(child_a_pid); print("). El padre se bloqueara ahora.\n");
    
    // ¡Aquí probamos wait_child!
    wait_child(child_a_pid); 

    print("\n[Padre]: ==> wait_child() retornó! El Hijo A ha terminado.\n\n");

    // --- Prueba 2: wait_all_children() (esperar a todos los hijos restantes) ---
    print("[Padre]: Lanzando Hijo B e Hijo C...\n");
    char* arg_b[] = {"HijoB"};
    char* arg_c[] = {"HijoC"};
    
    Process* child_b_proc = create_process(1, arg_b, child_process_test, 0);
    Process* child_c_proc = create_process(1, arg_c, child_process_test, 0);

    if (child_b_proc == NULL || child_c_proc == NULL) {
        print("[Padre]: ERROR al crear Hijo B o C.\n");
        return;
    }

    print("[Padre]: Hijo B (PID "); printDec(child_b_proc->pid); 
    print(") e Hijo C (PID "); printDec(child_c_proc->pid); print(") creados.\n");

    print("[Padre]: ==> Llamando a wait_all_children(). El padre se bloqueara hasta que B y C terminen.\n");
    
    // ¡Aquí probamos wait_all_children!
    wait_all_children();
    
    print("\n[Padre]: ==> wait_all_children() retornó! Todos los hijos (B y C) han terminado.\n");
    print("[Padre]: Prueba de 'wait' completada. Terminando.\n");
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
  

  /* Inicializar subsistema de semáforos antes de crear procesos que usen sem_open */
  if (init_sem_queue() < 0) {
    print("Failed to init sem queue\n");
  } else {
    print("Sem queue initialized\n");
  }

  print("Llamando a init_scheduler()...\n");
  init_scheduler();
  print("init_scheduler() completo.\n"); 

  print("Iniciando prueba de 'wait' y semaforos...\n");

  // --- Iniciar el proceso de prueba ---
  char* arg_parent[] = {"TestPadre"};
  Process* parent_test = create_process(1, arg_parent, parent_process_test, 0);

  // Crear procesos con diferentes prioridades usando la interfaz correcta
  //char* arga[] ={"procA", "A"};
  // char* argb[]={"procB", "B"};

  //Process* procA = create_process(2, arga, test_proc, 0);
  // Process* procB = create_process(2, argb, test_proc, 0);
  mm_init(heapStart, heapSize);

  if (init_scheduler() != 0) {
    panic("Failed to initialize scheduler");
    return 1;
  }


  print("[main] after create_process(procA)\n");


  _sti();
  print("Kernel IDLE. Waiting for interrupt...\n");

  /* Force one scheduler interrupt to test scheduling without relying on PIT */
  print("[main] forcing scheduler interrupt to test...\n");
  _force_scheduler_interrupt();
  while (1) {
     _hlt(); // Espera la próxima interrupción
  }
  __builtin_unreachable();
  return 0;
}
