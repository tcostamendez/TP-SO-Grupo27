#ifndef FIRST_FIT_MM_H
#define FIRST_FIT_MM_H

#include "memory_manager.h" 
#include <string.h> 

typedef long Align; 

typedef union header {
    struct {
        union header *ptr;  
        size_t size;         
    } s;
    Align x; 
} Header;


#endif 