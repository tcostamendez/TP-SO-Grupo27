#ifndef QUEUE_ARRAY_H
#define QUEUE_ARRAY_H

#include <stddef.h>
#include <stdint.h>

#include "alloc.h"
#include "queue.h"

typedef struct QueueArrayCDT * QueueArrayADT;

QueueArrayADT createQueueArray(QueueElemCmpFn cmp, size_t elemSize, int queueCount, int (*getQueueWeight)(int queueIndex, QueueADT queue));
void freeQueueArray(QueueArrayADT queueArray);
QueueArrayADT queueArrayAddByIndex(QueueArrayADT queueArray, int queueIndex, void * data);
QueueArrayADT queueArrayRemove(QueueArrayADT queueArray, int queueIndex, void * data);
QueueArrayADT queueArrayBeginWeightedIter(QueueArrayADT queueArray);
QueueArrayADT queueArrayNextWeightedIter(QueueArrayADT queueArray, void * buffer);
int queueArrayIsEmpty(QueueArrayADT queueArray);

#endif