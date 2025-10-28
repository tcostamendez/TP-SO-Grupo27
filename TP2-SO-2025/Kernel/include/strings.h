#ifndef STRINGS_H
#define STRINGS_H
#include <stdint.h>
#include <stddef.h>
#include "memory_manager.h"

size_t strlen(const char *str);

void my_strcpy(char *dest, const char *src);

char * num_to_str(uint64_t num);

void catenate(char * dest, const char * src);

int my_strcmp(const char * str1, const char * str2);
#endif
