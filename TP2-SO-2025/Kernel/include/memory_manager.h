#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h> 

typedef struct {
    size_t total_memory;
    size_t free_memory;
    size_t occupied_memory;
} MemoryStats;

typedef struct {
    size_t free_blocks;        
    size_t largest_free_block; 
    size_t smallest_free_block;
    size_t total_free_bytes;   
} MemoryFreeListInfo;

void mm_init(void * base_address, size_t total_size);

void* mm_alloc(size_t size);

void mm_free(void *ptr);

MemoryStats mm_get_stats();

MemoryFreeListInfo mm_get_free_list_info();

#endif 