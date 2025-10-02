#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdint.h>

#include "alloc.h"
#include "lib.h"

typedef struct List * ListADT;

ListADT listCreate(int (*cmp)(void * elemA, void * elemB), uint64_t sizeOfElem);
ListADT listAdd(ListADT list, void * data);
ListADT listRemove(ListADT list, void * data);
void listFree(ListADT list);
ListADT listBeginIter(ListADT list);
int hasNext(ListADT list);
void * listGetNext(ListADT list, void * buffer);
uint64_t listGetSize(ListADT list);

#endif