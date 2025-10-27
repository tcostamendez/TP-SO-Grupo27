#ifndef STRINGS_H
#define STRINGS_H
#include <stddef.h>

int strlen(const char *str);

void my_strcpy(char *dest, const char *src);

char * num_to_str(uint64_t num);

void catenate(char * dest, const char * src);

#endif
