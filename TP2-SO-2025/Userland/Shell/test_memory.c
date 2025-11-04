/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libsys.h>

int main(int argc, char** argv) {
    printf("=== Memory Management Test ===\n");
    
    if (argc < 2) {
        printf("Usage: test_memory <num_allocations>\n");
        return 1;
    }
    
    int num_allocs = atoi(argv[1]);
    if (num_allocs <= 0) {
        printf("Number of allocations must be positive\n");
        return 1;
    }
    
    printf("Allocating %d memory blocks...\n", num_allocs);
    
    void** ptrs = allocMemory(sizeof(void*) * num_allocs);
    if (ptrs == NULL) {
        printf("Failed to allocate array for pointers\n");
        return 1;
    }
    
    // Allocate memory blocks
    for (int i = 0; i < num_allocs; i++) {
        size_t size = (i % 10 + 1) * 100; // sizes from 100 to 1000 bytes
        ptrs[i] = allocMemory(size);
        
        if (ptrs[i] == NULL) {
            printf("Failed to allocate block %d (size %zu)\n", i, size);
        } else {
            // Write some data to verify it works
            memset(ptrs[i], i % 256, size);
            printf("Allocated block %d: %p (size %zu)\n", i, ptrs[i], size);
        }
    }
    
    printf("\nWaiting 3 seconds before freeing...\n");
    sleep(3000);
    
    // Free half the blocks
    printf("\nFreeing first half of blocks...\n");
    for (int i = 0; i < num_allocs / 2; i++) {
        if (ptrs[i] != NULL) {
            freeMemory(ptrs[i]);
            printf("Freed block %d\n", i);
        }
    }
    
    printf("\nWaiting 2 seconds...\n");
    sleep(2000);
    
    // Free remaining blocks
    printf("\nFreeing remaining blocks...\n");
    for (int i = num_allocs / 2; i < num_allocs; i++) {
        if (ptrs[i] != NULL) {
            freeMemory(ptrs[i]);
            printf("Freed block %d\n", i);
        }
    }
    
    // Free the pointer array
    freeMemory(ptrs);
    
    printf("\nMemory test completed!\n");
    return 0;
}
*/