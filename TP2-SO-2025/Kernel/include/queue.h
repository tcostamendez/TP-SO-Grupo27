#ifndef _LIST_H
#define _LIST_H

#include <stddef.h>
#include <stdint.h>

#include "lib.h"

typedef struct QueueCDT * QueueADT; 
typedef int (*QueueElemCmpFn)(void *, void *);

/**
 * @brief Create a queue with element comparison and element size.
 * @param cmp Comparison function used for search/removal (can be NULL).
 * @param elemSize Size of each element stored.
 * @return QueueADT handle or NULL on error.
 */
QueueADT createQueue(QueueElemCmpFn cmp, size_t elemSize);
/**
 * @brief Enqueue a copy of an element at the back.
 * @param queue Queue handle.
 * @param data Pointer to element to copy.
 * @return queue on success, NULL on error.
 */
QueueADT enqueue(QueueADT queue, void * data);
/**
 * @brief Dequeue front element into buffer.
 * @param queue Queue handle.
 * @param buffer Destination buffer (size >= elemSize).
 * @return buffer on success, NULL if empty.
 */
void * dequeue(QueueADT queue, void * buffer);
/**
 * @brief Peek front element without removing.
 * @param queue Queue handle.
 * @param buffer Destination buffer.
 * @return buffer on success, NULL if empty.
 */
void * queuePeek(QueueADT queue, void * buffer);
/**
 * @brief Remove the first matching element from the queue.
 * @param queue Queue handle.
 * @param data Element to remove (compared using cmp).
 * @return Removed element copied into internal temp and returned or NULL.
 */
void * queueRemove(QueueADT queue, void * data);
/**
 * @brief Free the queue and its internal storage.
 * @param queue Queue handle.
 */
void queueFree(QueueADT queue);
/**
 * @brief Get number of elements in the queue.
 * @param queue Queue handle.
 * @return Size of the queue.
 */
size_t queueSize(QueueADT queue);
/**
 * @brief Begin cyclic iteration over the queue.
 * @param queue Queue handle.
 * @return queue on success, NULL on error.
 */
QueueADT queueBeginCyclicIter(QueueADT queue);
/**
 * @brief Get next element in a cyclic iteration.
 * @param queue Queue handle.
 * @param buffer Destination buffer.
 * @return buffer on success, NULL if no elements.
 */
void * queueNextCyclicIter(QueueADT queue, void * buffer);
/**
 * @brief Check if cyclic iteration looped back to the start.
 * @param queue Queue handle.
 * @return 1 if looped, 0 otherwise.
 */
int queueCyclicIterLooped(QueueADT queue);
/**
 * @brief Check if queue is empty.
 * @param queue Queue handle.
 * @return 1 if empty, 0 otherwise.
 */
int queueIsEmpty(QueueADT queue);
/**
 * @brief Check if the iterator has been initialized.
 * @param queue Queue handle.
 * @return 1 if initialized, 0 otherwise.
 */
int queueIteratorIsInitialized(QueueADT queue);
/**
 * @brief Check if an element exists in the queue.
 * @param queue Queue handle.
 * @param data Element to search.
 * @return 1 if exists, 0 otherwise.
 */
int queueElementExists(QueueADT queue, void * data);

#endif