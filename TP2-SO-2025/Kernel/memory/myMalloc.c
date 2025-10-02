// #include "alloc.h"
// #include "myMalloc.h"

// #define GET_BLOCK_PTR(block) ((uint8_t *) block + sizeof(struct memoryBlock))
// #define GET_BLOCK_SIZE(block) ( (struct memoryBlock *) ((uint8_t *) block + sizeof(struct memoryBlock) + block->size))
// #define GET_BLOCK_NEXT(block) ( (struct memoryBlock *) ((uint8_t *) block + sizeof(struct memoryBlock) + block->size + sizeof(struct memoryBlock)))

// struct memoryManagerCDT {
//     uint8_t * start;
//     uint64_t size;
//     uint64_t used;
//     struct memoryBlock * first;
// };

// struct memoryBlock {
//     struct memoryBlock * next;
//     uint64_t size;
// };

// memoryManagerADT mmInitMemoryAllocator(void * start, uint64_t size) {
//     if (start == NULL || size <= sizeof(struct memoryManagerCDT) + sizeof(struct memoryBlock)) {
//         return NULL;
//     }
//     memoryManagerADT mm = start;
//     mm->start = start;
//     mm->size = size;
//     mm->used = 0;
//     mm->first = (struct memoryBlock *) (sizeof(struct memoryManagerCDT) + start);
//     mm->first->next = NULL;
//     mm->first->size = 0;
//     return mm;
// }

// void * mmMalloc(memoryManagerADT mm, unsigned int bytes) {
//     if (mm == NULL || bytes <= 0 || mm->used + bytes > mm->size) {
//         return NULL;
//     }

//     struct memoryBlock * iter = mm->first;
    
//     while (iter != NULL && iter->next != NULL) {
//         iter = iter->next;
//     }

//     struct memoryBlock * newBlock = GET_BLOCK_NEXT(iter);
//     iter->next = newBlock;
//     iter->next->size = bytes;
//     iter->next->next = NULL;
//     mm->used += bytes + sizeof(struct memoryBlock);

//     return GET_BLOCK_PTR(newBlock);
// }

// void mmFree(memoryManagerADT mm, void * ptr) {
//     if (mm == NULL || ptr == NULL) {
//         return;
//     }

//     struct memoryBlock * iter = mm->first;
//     struct memoryBlock * prev = NULL;

//     while (iter != NULL && GET_BLOCK_PTR(iter) != ptr) {
//         prev = iter;
//         iter = iter->next;
//     }

//     if (iter == NULL) {
//         return; // Pointer not found
//     }

//     if (prev != NULL) {
//         prev->next = iter->next;
//     } else {
//         mm->first = iter->next;
//     }

//     //todo Zero out the memory (Testing only). Remove
//     for (int i = 0; i < iter->size; i++) {
//         *((uint8_t *) iter + sizeof(struct memoryBlock) + i) = 0;
//     }

//     mm->used -= iter->size + sizeof(struct memoryBlock);
// }
