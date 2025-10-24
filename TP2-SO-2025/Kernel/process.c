#include "process.h"
#include "scheduler.h"      // Lo necesitamos para add_to_scheduler()
#include "memory_manager.h" // (Tu header) Lo necesitamos para mm_alloc() y mm_free()
#include <lib.h>            // (Tu header) Para memset()
#include <strings.h>        // (Tu header) Para my_strcpy() y strlen()
#include <interrupts.h>

/**
 * @brief Prepara un stack falso para un nuevo proceso.
 * (Esta función la crearemos en 'libasm.asm' más adelante).
 * * @param stack_top Puntero al tope del stack (ej: stackBase + STACK_SIZE)
 * @param rip       Puntero a la función a ejecutar (el entry point)
 * @param argc      Argument count
 * @param argv      Argument vector
 * @param terminator Puntero a la función (wrapper) que debe ejecutarse
 * cuando 'rip' retorne.
 * @return El nuevo valor de RSP para este contexto.
 */
extern uint64_t stackInit(uint64_t stack_top, ProcessEntryPoint rip, void (*terminator)());

/**
 * @brief Fuerza una interrupción de timer (int 0x20).
 * (Esta función ya la tienes en 'interrupts.asm' (o similar)).
 */
extern void _force_scheduler_interrupt();

static int pid = 0;

// Contador para el próximo PID a asignar.
static int next_pid = 1;

Process* create_process(char *name, ProcessEntryPoint entry_point) {
    Process* p = (Process*) mm_alloc(sizeof(Process));
    if (p == NULL) {
        print("PCB_ALLOC_FAIL\n"); // DEBUG
        return NULL;
    }

    p->stackBase = mm_alloc(PROCESS_STACK_SIZE);
    if (p->stackBase == NULL) {
        print("STACK_ALLOC_FAIL\n"); // DEBUG
        mm_free(p);
        return NULL;
    }

    // --- DEBUG ---
    // print("New Proc: "); print(name);
    // print(" Stack Base: "); printHex((uint64_t)p->stackBase);
    // print("\n");
    // --- FIN DEBUG ---

    p->pid = pid++;
    p->ppid = 0;
    p->state = READY;
    p->rip = entry_point;
    my_strcpy(p->name, name);
    
    uint64_t stack_top = (uint64_t)p->stackBase + PROCESS_STACK_SIZE;

    // --- CAMBIO ---
    // Llamada a stackInit simplificada
    p->rsp = stackInit(stack_top, p->rip, entry_point);
    
    //para asegurar que se cargue
    _cli();
    add_to_scheduler(p);
    _sti();
    return p;
}

int get_pid(Process* p) {
    return p->pid;
}

int get_parent_pid(Process * p){
    return p->ppid;
}

void yield_cpu() {
    _force_scheduler_interrupt();
}

Process* get_process(int pid) {
    if (pid < 1 || pid > MAX_PROCESSES) {
        return NULL;
    }
    return NULL;
}


