#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h> 

typedef struct {
    size_t total_memory;
    size_t free_memory;
    size_t occupied_memory;
} MemoryStats;

/**
 * @brief Free-list/allocator diagnostic information.
 */
typedef struct {
    size_t free_blocks;        
    size_t largest_free_block; 
    size_t smallest_free_block;
    size_t total_free_bytes;   
} MemoryFreeListInfo;

/**
 * @brief Initialize memory manager over a contiguous region.
 * @param base_address Start address of managed region.
 * @param total_size Size in bytes of managed region.
 */
void mm_init(void * base_address, size_t total_size);

/**
 * @brief Allocate a block of memory.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated block, or NULL on failure.
 */
void* mm_alloc(size_t size);

/**
 * @brief Free a previously allocated block.
 * @param ptr Pointer returned by mm_alloc.
 */
void mm_free(void *ptr);

/**
 * @brief Get aggregate memory usage statistics.
 * @return MemoryStats snapshot.
 */
MemoryStats mm_get_stats();

/**
 * @brief Get internal free-list details for diagnostics.
 * @return MemoryFreeListInfo snapshot.
 */
MemoryFreeListInfo mm_get_free_list_info();

#endif 