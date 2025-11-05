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
  };

  loadModules(&endOfKernelBinary, moduleAddresses);

  clearBSS(&bss, &endOfKernel - &bss);

  return getStackBase();
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

  int targetsReader[3] = {id, STDOUT, STDOUT};
  int targetsWriter[3] = {STDIN, id, STDOUT};
  Process *reader = create_process(1, rargv, pipe_reader, 0, targetsReader, 0);
  Process *writer = create_process(1, wargv, pipe_writer, 0, targetsWriter, 0);

  if (!reader || !writer) {
    print("[PipeDemo] ERROR: creando procesos de pipe\n");
    return;
  }
  print("[PipeDemo] Pipe id="); printDec(id); print(" | reader PID="); printDec(reader->pid); print(" writer PID="); printDec(writer->pid); print("\n");
}


int main() {
  load_idt();

  setFontSize(2);

  void *heapStartOriginal = (void *)((uint64_t)&endOfKernel + PageSize * 8);

  uint64_t base = ((uint64_t)heapStartOriginal + 0x1FFFFF) & ~((uint64_t)0x1FFFFF);
  if (base < 0x600000) { // Nos aseguramos de saltar los módulos
      base = 0x600000;
  }
  
  void *heapStart = (void*) base;
  uint64_t heapSize = (uint64_t)HEAP_END_ADDRESS - (uint64_t)heapStart;
  
  mm_init(heapStart, heapSize);

  /* Inicializar subsistema de semáforos antes de crear procesos que usen semOpen */
  initSemQueue();

  // Inicializar almacenamiento de pipes
  initPipeStorage();
  init_scheduler();

  char * argv[]= {"shell"};
  int targets []= {STDIN, STDOUT, ERR_FD};
  create_process(1, argv, shellModuleAddress,3, targets, 1);
  
  _sti();

  extern void _force_scheduler_interrupt();
  _force_scheduler_interrupt();

  __builtin_unreachable();
  return 0;
}
