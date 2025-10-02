#ifndef _LIST_H
#define _LIST_H

#include <stddef.h>
#include <stdint.h>

#include "alloc.h"
#include "lib.h"

typedef struct QueueCDT * QueueADT;

QueueADT createQueue(size_t elemSize);
void enqueue(QueueADT queue, void * data);
void * dequeue(QueueADT queue, void * buffer);
void * peek(QueueADT queue, void * buffer);

#endif
