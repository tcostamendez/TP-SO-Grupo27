#ifndef ARRAY_H
#define ARRAY_H

#include "lib.h"
#include "alloc.h"
#include "stddef.h"

typedef struct ArrayCDT * ArrayADT;

ArrayADT createArray(uint64_t sizeOfElem);

ArrayADT arrayAdd(ArrayADT array, void * data);

void * getElemByIndex(ArrayADT array, uint16_t index, void * buffer);

void * setElemByIndex(ArrayADT array, uint16_t index, void* data);

uint16_t arraySize(ArrayADT array);

void arrayFree(ArrayADT array);

ArrayADT arrayBeginIter(ArrayADT array);

int arrayHasNext(ArrayADT array);

void * arrayGetNext(ArrayADT array, void * buffer);

void arrayResetSize(ArrayADT array);

#endif