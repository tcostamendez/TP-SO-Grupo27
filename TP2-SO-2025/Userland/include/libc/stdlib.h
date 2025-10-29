#ifndef _LIBC_STDLIB_H_
#define _LIBC_STDLIB_H_

#include <stddef.h>
#include <stdint.h>

/* --------------- Random --------------- */
int rand(void);

void srand(unsigned int seed);

/* --------------- String --------------- */
int atoi(const char *str);

void *memcpy(void *destination, const void *source, uint64_t length);

#endif