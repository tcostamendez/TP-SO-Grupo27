#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdint.h>

#include "alloc.h"
#include "lib.h"

typedef struct List * ListADT;

ListADT listCreate(int (*cmp)(void *, void *), uint64_t sizeOfElem);
ListADT listAdd(ListADT list, void * data);
ListADT listRemove(ListADT list, void * data);
void listFree(ListADT list);
void * listRemoveFromHead(ListADT list); // Removes head node and returns its data pointer
void listAddToTail(ListADT list, void * data); // Convenience tail insertion (non-ordered)
ListADT listBeginIter(ListADT list);
int hasNext(ListADT list);
void * listGetNext(ListADT list, void * buffer);
uint64_t listGetSize(ListADT list);

#endif