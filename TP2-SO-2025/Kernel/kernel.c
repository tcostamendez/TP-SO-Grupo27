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
#include "panic.h"

extern void _sti(); // (De interrupts.asm) Habilita interrupciones
extern void _hlt(); // (De interrupts.asm) Detiene la CPU hasta la próxima interrupción

extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

extern void _force_scheduler_interrupt();

static const uint64_t PageSize = 0x1000;
//static void *const HEAP_END_ADDRESS = (void *)0x10600000; //256mb
static void *const HEAP_END_ADDRESS = (void *)0x20000000;  //512mb
static void *const shellModuleAddress = (void *)0x400000;

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
      shellModuleAddress
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


// ================================
// Kernel-side tests (process/semaphore/wait)
// ================================

static void worker_proc(int argc, char** argv) {
  const char *name = (argc > 0) ? argv[0] : "worker";
  int pid = get_current_process()->pid;
  for (int i = 0; i < 10; i++) {
    print("[" ); print(name); print("] pid="); printDec(pid); print(" iter="); printDec(i+1); print("\n");
    for(volatile int d=0; d<20000000; d++);
    yield_cpu();
  }
}

static void kernel_sem_child(int argc, char** argv) {
  const char* sem_parent = (argc > 1) ? argv[1] : "k_sem_parent";
  const char* sem_child  = (argc > 2) ? argv[2] : "k_sem_child";
  int pid = get_current_process()->pid;
  int rounds = 5;
  for (int i = 0; i < rounds; i++) {
    Sem sp = sem_open((char*)sem_parent, 0);
    Sem sc = sem_open((char*)sem_child, 0);
    if (sp && sc) {
      sem_wait(sp);
      print("  [sem child pid="); printDec(pid); print("] pong "); printDec(i+1); print("\n");
      sem_post(sc);
      sem_close(sp);
      sem_close(sc);
    }
    yield_cpu();
  }
}

static void kernel_sem_parent(int argc, char** argv) {
  const char* sem_parent_name = "k_sem_parent";
  const char* sem_child_name  = "k_sem_child";
  int rounds = 5;

  // Create semaphores
  Sem sp = sem_open((char*)sem_parent_name, 1);
  Sem sc = sem_open((char*)sem_child_name, 0);
  if (sp == NULL || sc == NULL) {
    print("[sem parent] ERROR opening semaphores\n");
    return;
  }

  // Launch child that will alternate with us
  char* child_args[] = {"k_sem_child", (char*)sem_parent_name, (char*)sem_child_name};
  Process* child = create_process(3, child_args, kernel_sem_child, 1);
  if (child == NULL) {
    print("[sem parent] ERROR creating child\n");
    return;
  }

  int ppid = get_current_process()->pid;
  for (int i = 0; i < rounds; i++) {
    // Wait for child to print
    sem_wait(sc);
    print("[sem parent pid="); printDec(ppid); print("] ping "); printDec(i+1); print("\n");
    sem_post(sp);
    yield_cpu();
  }

  // Wait child termination then close sems
  wait_child(child->pid);
  sem_close(sp);
  sem_close(sc);
}

static void launch_kernel_tests(void) {
  print("\n=== Kernel tests: process creation ===\n");
  char* w1[] = {"W1"};
  char* w2[] = {"W2"};
  char* w3[] = {"W3"};
  create_process(1, w1, worker_proc, 0);
  create_process(1, w2, worker_proc, 1);
  create_process(1, w3, worker_proc, 2);

  print("\n=== Kernel tests: semaphores ping-pong ===\n");
  char* sp_args[] = {"sem_parent"};
  create_process(1, sp_args, kernel_sem_parent, 1);

  print("\n=== Kernel tests: waits (parent/children) ===\n");
  char* p_args[] = {"parent_test"};
  create_process(1, p_args, parent_process_test, 1);
}

void default_process(int argc, char** argv) {
  print("Default process running\n");
  while(1) {
    _hlt();
  }
}

int main() {
  load_idt();

  setFontSize(1);
  void *heapStartOriginal = (void *)((uint64_t)&endOfKernel + PageSize * 8);

  uint64_t base = ((uint64_t)heapStartOriginal + 0x1FFFFF) & ~((uint64_t)0x1FFFFF);
  if (base < 0x600000) { // Nos aseguramos de saltar los módulos
      base = 0x600000;
  }
  
  void *heapStart = (void*) base;
  uint64_t heapSize = (uint64_t)HEAP_END_ADDRESS - (uint64_t)heapStart;
  
  print("Llego aca al menos?\n");

  /* IMPORTANTE: Inicializar el gestor de memoria ANTES de cualquier mm_alloc */
  mm_init(heapStart, heapSize);
  print("INICIALIZO EL MM\n");

  /* Inicializar subsistema de semáforos (usa mm_alloc) */
  if (init_sem_queue() != 0) {
    panic("Failed to init sem queue\n");
  } else {
    print("Sem queue initialized\n");
  }

  print("INICIALIZO EL SEM QUEUE\n");

  /* Inicializar scheduler (crea procesos y usa mm_alloc) */
  if (init_scheduler() != 0) {
    panic("Failed to initialize scheduler");
  } else {
    print("Scheduler initialized\n");
  }

  print("INICIALIZO EL SCHEDULER\n");
  
  // Crea proceso default para saber que terminaron los tests
  char* default_args[] = {"default"};
  Process* default_proc = create_process(1, default_args, default_process, 0);
  if (default_proc == NULL) {
    panic("Failed to create default process");
  }
  // Lanzar pruebas desde kernel (todo en contexto de procesos del kernel)
  launch_kernel_tests();
  
 

  _sti();

  _force_scheduler_interrupt();

  while (1) {
     _hlt(); // Espera la próxima interrupción
  }
  __builtin_unreachable();
  return 0;
}
