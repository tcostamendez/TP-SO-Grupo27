#ifndef ARRAY_H
#define ARRAY_H

#include "lib.h"
#include "stddef.h"

typedef struct ArrayCDT * ArrayADT;

/**
 * @brief Create a dynamic array for fixed-size elements.
 * @param sizeOfElem Size of each element.
 * @return ArrayADT handle or NULL on error.
 */
ArrayADT createArray(uint64_t sizeOfElem);

/**
 * @brief Append an element to the array.
 * @param array Array handle.
 * @param data Element to copy.
 * @return array on success, NULL on error.
 */
ArrayADT arrayAdd(ArrayADT array, void * data);

/**
 * @brief Get element by index.
 * @param array Array handle.
 * @param index Zero-based index.
 * @param buffer Destination buffer (size >= element size).
 * @return buffer on success, NULL if out of range.
 */
void * getElemByIndex(ArrayADT array, uint16_t index, void * buffer);

/**
 * @brief Set element by index (overwrite).
 * @param array Array handle.
 * @param index Zero-based index.
 * @param data Source element to copy.
 * @return buffer to internal storage or NULL on error.
 */
void * setElemByIndex(ArrayADT array, uint16_t index, void* data);

/**
 * @brief Get current array size (number of elements).
 */
uint16_t arraySize(ArrayADT array);

/**
 * @brief Free array and its storage.
 */
void arrayFree(ArrayADT array);

/**
 * @brief Begin iterating array elements from index 0.
 */
ArrayADT arrayBeginIter(ArrayADT array);

/**
 * @brief Check if there is a next element during iteration.
 * @return 1 if next exists, 0 otherwise.
 */
int arrayHasNext(ArrayADT array);

/**
 * @brief Get next element during iteration.
 * @param array Array handle.
 * @param buffer Destination buffer.
 * @return buffer on success, NULL if no next.
 */
void * arrayGetNext(ArrayADT array, void * buffer);

/**
 * @brief Reset internal size/capacity to reclaim memory if possible.
 */
void arrayResetSize(ArrayADT array);

#endif