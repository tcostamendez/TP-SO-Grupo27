#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h> // Para size_t

// Estructura de estadísticas de memoria
typedef struct {
    size_t total_memory;
    size_t free_memory;
    size_t occupied_memory;
    // Se pueden añadir más campos específicos de cada gestor
} MemoryStats;

// Información adicional de la free list (debug)
typedef struct {
    size_t free_blocks;        // Cantidad de bloques libres
    size_t largest_free_block; // Tamaño del bloque libre más grande (bytes)
    size_t smallest_free_block;// Tamaño del bloque libre más pequeño (bytes)
    size_t total_free_bytes;   // Suma de bytes libres (debe coincidir con free_memory)
} MemoryFreeListInfo;

// Funciones de la Interfaz Común
void mm_init(void * base_address, size_t total_size);

void* mm_alloc(size_t size);

void mm_free(void *ptr);

MemoryStats mm_get_stats();

// Debug: información de la lista libre
MemoryFreeListInfo mm_get_free_list_info();

// Debug: imprime estado de la lista libre (opcional en ambiente kernel)
void mm_debug_print_free_list();

// Self test (retorna 0 si todo OK, !=0 si falla algo)
int mm_self_test();

#endif // MEMORY_MANAGER_H