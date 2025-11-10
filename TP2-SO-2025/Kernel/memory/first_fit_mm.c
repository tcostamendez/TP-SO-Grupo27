// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/first_fit_mm.h"
#include "../include/memory_manager.h"

static Header base;
static Header *freep = NULL;

static size_t total_mem_size = 0;
static size_t free_mem_size = 0;

#define UNIT_SIZE sizeof(Header)

void mm_init(void *base_address, size_t total_size) {
  Header *initial_block = (Header *)base_address;
  size_t units = total_size / UNIT_SIZE;
  initial_block->s.size = units;

  size_t usable_bytes = units * UNIT_SIZE;

  total_mem_size = usable_bytes;
  free_mem_size = usable_bytes;

  base.s.ptr = initial_block;
  base.s.size = 0;

  initial_block->s.ptr = &base;

  freep = &base;
}

#define MIN_ALLOC_UNITS 1

void *mm_alloc(size_t nbytes) {
  if (nbytes == 0) {
    return NULL;
  }

  size_t nunits = (nbytes + UNIT_SIZE - 1) / UNIT_SIZE + 1;
  if (nunits < MIN_ALLOC_UNITS) {
    nunits = MIN_ALLOC_UNITS;
  }

  Header *p, *prevp;
  prevp = freep;

  for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
    if (p->s.size >= nunits) {
      Header *allocated_block;

      if (p->s.size == nunits) {
        prevp->s.ptr = p->s.ptr;
        allocated_block = p;
      } else {
        p->s.size -= nunits;
        allocated_block = p + p->s.size;
        allocated_block->s.size = nunits;
      }

      freep = prevp;
      free_mem_size -= (nunits * UNIT_SIZE);
      return (void *)(allocated_block + 1);
    }

    if (p == freep) {
      return NULL;
    }
  }
}
void mm_free(void *ap) {
  if (ap == NULL)
    return;

  Header *bp = (Header *)ap - 1;

  free_mem_size += (bp->s.size * UNIT_SIZE);

  Header *p;
  for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) {
    if (p >= p->s.ptr && (bp > p || bp < p->s.ptr)) {
      break;
    }
  }

  if (bp + bp->s.size == p->s.ptr) {
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else {
    bp->s.ptr = p->s.ptr;
  }

  if (p + p->s.size == bp) {
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else {
    p->s.ptr = bp;
    p = bp;
  }

  freep = p;
}

MemoryStats mm_get_stats() {
  MemoryStats stats;
  stats.total_memory = total_mem_size;
  stats.free_memory = free_mem_size;

  if (total_mem_size >= free_mem_size) {
    stats.occupied_memory = total_mem_size - free_mem_size;
  } else {
    stats.occupied_memory = 0;
  }
  return stats;
}
