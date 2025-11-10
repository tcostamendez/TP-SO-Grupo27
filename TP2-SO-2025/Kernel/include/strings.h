#ifndef STRINGS_H
#define STRINGS_H
#include <stdint.h>
#include <stddef.h>
#include "memory_manager.h"

/**
 * @brief Compute the length of a C string (excluding null terminator).
 * @param str Null-terminated string.
 * @return Number of characters.
 */
size_t strlen(const char *str);

/**
 * @brief Copy a C string including null terminator.
 * @param dest Destination buffer.
 * @param src Source string.
 */
void my_strcpy(char *dest, const char *src);

/**
 * @brief Convert an unsigned integer to a decimal string.
 * @param num Number to convert.
 * @return Newly allocated string representation. Free with mm_free().
 */
char * num_to_str(uint64_t num);

/**
 * @brief Concatenate src at the end of dest.
 * @param dest Destination buffer (must have room).
 * @param src Source string to append.
 */
void catenate(char * dest, const char * src);

/**
 * @brief Compare two C strings lexicographically.
 * @param str1 First string.
 * @param str2 Second string.
 * @return 0 if equal, <0 if str1<str2, >0 otherwise.
 */
int my_strcmp(const char * str1, const char * str2);
#endif
