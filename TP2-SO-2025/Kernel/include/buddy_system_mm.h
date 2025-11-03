#ifndef BUDDY_SYSTEM_MM_H
#define BUDDY_SYSTEM_MM_H

#include "memory_manager.h"
#include <stdint.h>

#define MIN_BLOCK_SIZE 0x1000 // 4KB
#define MAX_ORDER 16

typedef struct free_block {
   struct free_block *next;
} free_block_t;


typedef struct alloc_metadata {
   uint8_t order;
   uint8_t __padding[7]; 
} alloc_metadata_t;


extern free_block_t *buddy_lists[MAX_ORDER + 1];

#endif