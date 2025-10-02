#ifndef _MYMALLOC_H
#define _MYMALLOC_H

#include <stdint.h>
#include <stddef.h>

#include "alloc.h"

memoryManagerADT mmInitMemoryAllocator(void * start, uint64_t size);
void * mmMalloc(memoryManagerADT mm, unsigned int bytes);
void mmFree(memoryManagerADT mm, void * ptr);

#endif