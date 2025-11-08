#ifndef _MVAR_H_
#define _MVAR_H_

#include "sem.h"
#include "stdint.h"

// Initialize the MVar with specified number of readers and writers
int init_mvar(int readers, int writers);

// Put a value into the MVar (blocks if full)
int put_mvar(char value);

// Get a value from the MVar (blocks if empty)
char get_mvar(void);

// Cleanup and destroy the MVar
int close_mvar(void);

#endif

