#include "../include/buddy_system_mm.h"
#include "../include/memory_manager.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

free_block_t *buddy_lists[MAX_ORDER + 1];
static void *start_addr = NULL;
static size_t total_managed_bytes = 0;
static void remove_from_list(free_block_t *block_to_remove, int order);
static int is_buddy_free(free_block_t *buddy, int order);

void mm_init(void *base_address, size_t total_size) {
  for (int i = 0; i <= MAX_ORDER; i++) {
    buddy_lists[i] = NULL;
  }

  int k = 0;
  while ((((uint64_t)1 << k) * MIN_BLOCK_SIZE <= total_size) &&
         (k <= MAX_ORDER)) {
    k++;
  }
  int max_k = k - 1;

    if (max_k < 0) {
        total_managed_bytes = 0;
        return;
    }
  
    start_addr = base_address;
    free_block_t *initial_block = (free_block_t *)base_address;
    initial_block->next = NULL;
  
    buddy_lists[max_k] = initial_block;

    // Bytes totales realmente manejados por el buddy
    total_managed_bytes = ((size_t)1ULL << max_k) * (size_t)MIN_BLOCK_SIZE;
}

void *mm_alloc(size_t size) {
  if (size == 0)
    return NULL;

  size_t required_space = size + sizeof(alloc_metadata_t);
  if (required_space < sizeof(free_block_t)) {
    required_space = sizeof(free_block_t);
  }

  int target_order = 0;
  uint64_t block_size = MIN_BLOCK_SIZE;
  while (block_size < required_space) {
    block_size <<= 1;
    target_order++;
  }

  if (target_order > MAX_ORDER)
    return NULL;

  int current_order = target_order;
  while (current_order <= MAX_ORDER) {
    if (buddy_lists[current_order] != NULL) {
      free_block_t *block = buddy_lists[current_order];
      buddy_lists[current_order] = block->next;

      while (current_order > target_order) {
        current_order--;
        uint64_t half_size = (uint64_t)MIN_BLOCK_SIZE << current_order;

        free_block_t *buddy = (free_block_t *)((uintptr_t)block + half_size);
        buddy->next = buddy_lists[current_order];
        buddy_lists[current_order] = buddy;
      }

      alloc_metadata_t *metadata = (alloc_metadata_t *)block;
      metadata->order = (uint8_t)target_order;

      return (void *)((uintptr_t)block + sizeof(alloc_metadata_t));
    }
    current_order++;
  }

  return NULL;
}

void mm_free(void *ptr) {
  if (ptr == NULL)
    return;

  alloc_metadata_t *metadata =
      (alloc_metadata_t *)((uintptr_t)ptr - sizeof(alloc_metadata_t));
  free_block_t *block_to_free = (free_block_t *)metadata;
  int current_order = metadata->order;

  while (current_order < MAX_ORDER) {
    uint64_t block_size = (uint64_t)MIN_BLOCK_SIZE << current_order;

    uintptr_t relative_addr = (uintptr_t)block_to_free - (uintptr_t)start_addr;
    uintptr_t buddy_relative_addr = relative_addr ^ block_size;
    free_block_t *buddy =
        (free_block_t *)(buddy_relative_addr + (uintptr_t)start_addr);

    if (is_buddy_free(buddy, current_order)) {
      remove_from_list(buddy, current_order);

      if ((uintptr_t)buddy < (uintptr_t)block_to_free) {
        block_to_free = buddy;
      }

      current_order++;
    } else {
      break;
    }
  }

  block_to_free->next = buddy_lists[current_order];
  buddy_lists[current_order] = block_to_free;
}

static int is_buddy_free(free_block_t *buddy, int order) {
  free_block_t *current = buddy_lists[order];
  while (current != NULL) {
    if (current == buddy) {
      return 1;
    }
    current = current->next;
  }
  return 0;
}

static void remove_from_list(free_block_t *block_to_remove, int order) {
  free_block_t *current = buddy_lists[order];
  free_block_t *prev = NULL;

  while (current != NULL) {
    if (current == block_to_remove) {
      if (prev == NULL) {
        buddy_lists[order] = current->next;
      } else {
        prev->next = current->next;
      }
      return;
    }
    prev = current;
    current = current->next;
  }
}

MemoryStats mm_get_stats() {
    MemoryStats s = {0, 0, 0};
    s.total_memory = total_managed_bytes;

    size_t free_bytes = 0;
    for (int order = 0; order <= MAX_ORDER; order++) {
        uint64_t block_size = ((uint64_t)MIN_BLOCK_SIZE) << order;
        for (free_block_t *node = buddy_lists[order]; node != NULL; node = node->next) {
            free_bytes += (size_t)block_size;
        }
    }

    s.free_memory = free_bytes;
    s.occupied_memory = (s.total_memory >= s.free_memory)? (s.total_memory - s.free_memory): 0;
    return s;
}