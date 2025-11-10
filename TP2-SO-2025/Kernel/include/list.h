#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdint.h>

#include "memory_manager.h"
#include "lib.h"

typedef struct List * ListADT;

/**
 * @brief Create a new list.
 * @param cmp Comparison function for elements.
 * @param sizeOfElem Size of each element.
 * @return ListADT handle or NULL on error.
 */
ListADT listCreate(int (*cmp)(void * elemA, void * elemB), uint64_t sizeOfElem);
/**
 * @brief Add an element to the list.
 * @param list List handle.
 * @param data Element to copy in.
 * @return list on success, NULL on error.
 */
ListADT listAdd(ListADT list, void * data);
/**
 * @brief Remove an element from the list.
 * @param list List handle.
 * @param data Element to remove (by cmp).
 * @return list on success, NULL if not found.
 */
ListADT listRemove(ListADT list, void * data);
/**
 * @brief Free the list and its storage.
 * @param list List handle.
 */
void listFree(ListADT list);
/**
 * @brief Begin iterating the list from the start.
 * @param list List handle.
 * @return list on success, NULL on error.
 */
ListADT listBeginIter(ListADT list);
/**
 * @brief Check if there is a next element during iteration.
 * @param list List handle.
 * @return 1 if next exists, 0 otherwise.
 */
int listHasNext(ListADT list);
/**
 * @brief Get the next element during iteration.
 * @param list List handle.
 * @param buffer Destination buffer (size >= element size).
 * @return buffer on success, NULL if no next.
 */
void * listGetNext(ListADT list, void * buffer);
/**
 * @brief Get number of elements in the list.
 * @param list List handle.
 * @return Element count.
 */
uint64_t listGetSize(ListADT list);

#endif