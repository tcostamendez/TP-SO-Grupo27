#ifndef _ALLOC_H
#define _ALLOC_H

#include <stdint.h>

typedef struct memoryManagerCDT * memoryManagerADT;

void initMemoryAllocator(void * start, uint64_t size);
void * malloc(unsigned int bytes);
void free(void * ptr);

#endif