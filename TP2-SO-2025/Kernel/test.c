// #include "test.h"
// #include "interrupts.h"
// #include "first_fit_mm.h"
// #include "memory_manager.h"
// #include "fonts.h"
// #include "alloc.h"

// #include "queue.h"
// #include "list.h"

// #define assert(x) \
//     if (!(x)) { \
//         print("Assert failed on line:"); printDec(__LINE__); print(" @ Kernel "); print(__FILE__); print("\n"); print(#x); \
//         __asm__("cli; hlt"); \
//     }

// void memoryManagerTests() {
//     assert(1 == 1);
    
//     assert(mm_alloc(0x1) != NULL);
//     assert(mm_alloc(0x1) != mm_alloc(0x1));
//     assert(mm_alloc(0) == NULL);

//     int * m1 = mm_alloc(sizeof(int));
//     int * m2 = mm_alloc(sizeof(int));
//     assert(m1 != m2);
//     *m1 = 0x12345678;
//     *m2 = 0x87654321;
//     assert(*m1 != *m2);
//     assert(*m1 == 0x12345678);
//     assert(*m2 == 0x87654321);

//     mm_free(m1);

//     assert(m1 != NULL);

//     int exitCode = mm_self_test();
//     switch (exitCode){
//         case 0:
//             assert(exitCode == 0);
//             break;
//         case 1:
//             assert(exitCode == 2);
//             break;
//         case 2:
//             assert(exitCode == 3);
//             break;
//         case 3:
//             assert(exitCode == 4);
//             break;
//         case 4:
//             assert(exitCode == 5);
//             break;
//         case 5:
//             assert(exitCode == 6);
//             break;
//         case 6:
//             assert(exitCode == 7);
//             break;
//         case 7:
//             assert(exitCode == 8);
//             break;
//         default:
//             break;
//     }
//     //assert(*m1 == 0); logica vieja
// }

// int cmpInts(void * a, void * b) {
//     return (*(int *)a - *(int *)b);
// }

// // void queueDataStructureTests() {
// //     QueueADT queue = createQueue(sizeof(int));
// //     int aux;
// //     aux = 0x12345678; enqueue(queue, (void *) &aux);
// //     aux = 0x87654321; enqueue(queue, (void *) &aux);

// //     int buffer = -1;
// //     assert(*(int *) peek(queue, &buffer) == 0x12345678);
// //     assert(*(int *) dequeue(queue, &buffer) == 0x12345678);
// //     assert(*(int *) dequeue(queue, &buffer) == 0x87654321);
// //     assert( dequeue(queue, &buffer) == NULL);
// // }

// // void listDataStructureTests() {
// //     ListADT list = listCreate(cmpInts, sizeof(int));
// //     int a = 1, b = 2, c = 3;
// //     // Test in order insertions
// //     int buffer;
// //     listAdd(list, &a);
// //     listAdd(list, &b);
// //     listAdd(list, &c);
// //     assert(listGetSize(list) == 3);

// //     listBeginIter(list);
// //     assert(hasNext(list) == 1);
// //     assert(* (int *) listGetNext(list, &buffer) == a);
// //     assert(* (int *) listGetNext(list, &buffer) == b);
// //     assert(* (int *) listGetNext(list, &buffer) == c);
// //     assert(hasNext(list) == 0);

// //     listRemove(list, &a);
// //     listRemove(list, &b);
// //     listRemove(list, &c);
    
// //     assert(listGetSize(list) == 0);
    
// //     // Test out of order insertions
// //     listAdd(list, &a);
// //     listAdd(list, &c);
// //     listAdd(list, &b);

// //     listBeginIter(list);
// //     assert(hasNext(list) == 1);
// //     assert(* (int *) listGetNext(list, &buffer) == a);
// //     assert(* (int *) listGetNext(list, &buffer) == b);
// //     assert(* (int *) listGetNext(list, &buffer) == c);
// //     assert(hasNext(list) == 0);

// //     listFree(list);
// // }

// void runTests() {
//     memoryManagerTests();
//     //queueDataStructureTests();
//     //listDataStructureTests();
// }
