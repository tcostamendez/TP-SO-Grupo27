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
#include "process.h"
#include "scheduler.h"
#include <video.h>
#include "queue.h"
#include "process.h"
#include "sem.h"
#include "pipe.h"
#include "fd.h"

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
    // lo que debe disparar el semPost() en "wait_<pid>" y despertar al padre.
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


// ===== PIPE DEMO =====
static int atoi_simple(const char *s) {
  int v = 0; if (!s) return 0; while (*s >= '0' && *s <= '9') { v = v*10 + (*s - '0'); s++; } return v;
}

static int contains_END(const uint8_t *buf, int n) {
  if (!buf || n < 3) return 0;
  for (int i = 0; i <= n-3; i++) {
    if (buf[i]=='E' && buf[i+1]=='N' && buf[i+2]=='D') return 1;
  }
  return 0;
}

void pipe_writer(int argc, char **argv) {
  // argv[0] contains pipe id as string
  int pipeId = (argc > 0 && argv && argv[0]) ? atoi_simple(argv[0]) : -1;
  Process *self = get_current_process();
  if (pipeId >= 0 && self) {
    setWriteTarget(self->targetByFd, (uint8_t)pipeId);
  }
  const char *msgs[] = {"[pipe-writer] Hola 1\n", "[pipe-writer] Hola 2\n", "[pipe-writer] END\n"};
  for (int i=0;i<3;i++) {
    const char *m = msgs[i];
    // Enviar por el WRITE_FD a través del pipe mapeado
    fd_write(WRITE_FD, (const uint8_t*)m, (int)strlen(m));
    for (volatile int j=0;j<30000000;j++); // pequeña espera
  }
}

void pipe_reader(int argc, char **argv) {
  int pipeId = (argc > 0 && argv && argv[0]) ? atoi_simple(argv[0]) : -1;
  Process *self = get_current_process();
  if (pipeId >= 0 && self) {
    attach((uint8_t)pipeId);
    setReadTarget(self->targetByFd, (uint8_t)pipeId);
  }
  uint8_t buf[128];
  for (;;) {
    int n = fd_read(READ_FD, buf, (int)sizeof(buf));
    if (n <= 0) {
      // EOF o error
      break;
    }
    // Mostrar lo leído a STDOUT
    fd_write(WRITE_FD, buf, n);
    if (contains_END(buf, n)) {
      break;
    }
  }
}

static void run_pipe_demo(void) {
  print("\n[PipeDemo] Creando pipe y lanzando productor/consumidor...\n");
  int id = openPipe();
  if (id < 0) {
    print("[PipeDemo] ERROR: no se pudo crear pipe\n");
    return;
  }
  // Asegurar attached>=2 para evitar EOF inmediato cuando el buffer está vacío
  attach((uint8_t)id);
  attach((uint8_t)id);

  char *id_str = num_to_str((uint64_t)id);
  char *wargv[] = { id_str };
  char *rargv[] = { id_str };

  Process *reader = create_process(1, rargv, pipe_reader, 0);
  Process *writer = create_process(1, wargv, pipe_writer, 0);

  if (!reader || !writer) {
    print("[PipeDemo] ERROR: creando procesos de pipe\n");
    return;
  }
  print("[PipeDemo] Pipe id="); printDec(id); print(" | reader PID="); printDec(reader->pid); print(" writer PID="); printDec(writer->pid); print("\n");
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

  /* Inicializar subsistema de semáforos antes de crear procesos que usen semOpen */
  if (initSemQueue() < 0) {
    print("Failed to init sem queue\n");
  } else {
    print("Sem queue initialized\n");
  }

  // Inicializar almacenamiento de pipes
  if (initPipeStorage() < 0) {
    print("Failed to init pipe storage\n");
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

  //print("[main] after create_process(procA)\n");

  // Lanzar demo de pipes
  run_pipe_demo();

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
