#ifndef _LIBC_STDLIB_H_
#define _LIBC_STDLIB_H_

#include <stddef.h>
#include <stdint.h>

int rand(void);

void srand(unsigned int seed);

int atoi(const char *str);

void *memcpy(void *destination, const void *source, uint64_t length);

int get_free_bytes(void);

int get_used_bytes(void);

int get_total_bytes(void);

#endif