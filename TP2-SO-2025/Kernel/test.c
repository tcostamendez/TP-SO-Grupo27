#include "test.h"
#include "interrupts.h"

#include "fonts.h"
#include "alloc.h"

#include "queue.h"
#include "list.h"

#define assert(x) \
    if (!(x)) { \
        print("Assert failed on line:"); printDec(__LINE__); print(" @ Kernel "); print(__FILE__); print("\n"); print(#x); \
        __asm__("cli; hlt"); \
    }

void memoryManagerTests() {
    assert(1 == 1);
    
    assert(malloc(0x1) != NULL);
    assert(malloc(0x1) != malloc(0x1));
    assert(malloc(0) == NULL);

    int * m1 = malloc(sizeof(int));
    int * m2 = malloc(sizeof(int));
    assert(m1 != m2);
    *m1 = 0x12345678;
    *m2 = 0x87654321;
    assert(*m1 != *m2);
    assert(*m1 == 0x12345678);
    assert(*m2 == 0x87654321);

    free(m1);

    assert(m1 != NULL);
    assert(*m1 == 0);
}

int cmpInts(void * a, void * b) {
    return (*(int *)a - *(int *)b);
}

void queueDataStructureTests() {
    QueueADT queue = createQueue(sizeof(int));
    int aux;
    aux = 0x12345678; enqueue(queue, (void *) &aux);
    aux = 0x87654321; enqueue(queue, (void *) &aux);

    int buffer = -1;
    assert(*(int *) peek(queue, &buffer) == 0x12345678);
    assert(*(int *) dequeue(queue, &buffer) == 0x12345678);
    assert(*(int *) dequeue(queue, &buffer) == 0x87654321);
    assert( dequeue(queue, &buffer) == NULL);
}

void listDataStructureTests() {
    ListADT list = listCreate(cmpInts, sizeof(int));
    int a = 1, b = 2, c = 3;
    // Test in order insertions
    int buffer;
    listAdd(list, &a);
    listAdd(list, &b);
    listAdd(list, &c);
    assert(listGetSize(list) == 3);

    listBeginIter(list);
    assert(hasNext(list) == 1);
    assert(* (int *) listGetNext(list, &buffer) == a);
    assert(* (int *) listGetNext(list, &buffer) == b);
    assert(* (int *) listGetNext(list, &buffer) == c);
    assert(hasNext(list) == 0);

    listRemove(list, &a);
    listRemove(list, &b);
    listRemove(list, &c);
    
    assert(listGetSize(list) == 0);
    
    // Test out of order insertions
    listAdd(list, &a);
    listAdd(list, &c);
    listAdd(list, &b);

    listBeginIter(list);
    assert(hasNext(list) == 1);
    assert(* (int *) listGetNext(list, &buffer) == a);
    assert(* (int *) listGetNext(list, &buffer) == b);
    assert(* (int *) listGetNext(list, &buffer) == c);
    assert(hasNext(list) == 0);

    listFree(list);
}

void runTests() {
    memoryManagerTests();
    queueDataStructureTests();
    listDataStructureTests();
}
