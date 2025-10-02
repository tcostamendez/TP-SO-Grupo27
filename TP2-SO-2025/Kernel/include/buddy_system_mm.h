#ifndef BUDDY_SYSTEM_MM_H
#define BUDDY_SYSTEM_MM_H

#include "memory_manager.h" // Incluimos la interfaz común

// El tamaño de bloque más pequeño que asignaremos (ej: 4KB o 2KB)
#define MIN_BLOCK_SIZE 0x1000 // 4KB (Una página)

// El orden (order) máximo de bloques. Si la RAM es 128MB (2^27 bytes), 
// y MIN_BLOCK_SIZE es 2^12, el número máximo de órdenes es 27 - 12 = 15.
#define MAX_ORDER 15 

// La estructura de un nodo de la lista libre
// (Solo necesitamos el puntero al siguiente, ya que el arreglo nos dice el tamaño)
typedef struct free_block {
    struct free_block *next;
} free_block_t;

// Estructura para almacenar metadatos de asignación
// Se guarda al inicio de un bloque *asignado* para saber su tamaño.
typedef struct alloc_metadata {
    uint8_t order; // El orden (0 a MAX_ORDER) del bloque actual
    // Podrías añadir un flag 'is_allocated' si fuera necesario
} alloc_metadata_t;


// El arreglo de listas: Cada elemento es la cabeza de una lista de bloques libres de un tamaño 2^k
extern free_block_t *buddy_lists[MAX_ORDER + 1];

#endif