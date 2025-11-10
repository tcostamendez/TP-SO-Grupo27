#ifndef _LIBC_STRING_H_
#define _LIBC_STRING_H_

#include <stddef.h>

/**
 * @brief Get string length.
 */
int strlen(const char * str);
/**
 * @brief Compare strings.
 */
int strcmp(const char * str1, const char * str2);
/**
 * @brief Case-insensitive string compare.
 */
int strcasecmp(const char * str1, const char * str2);
/**
 * @brief Copy string (including null terminator).
 */
void strcpy(char * dest, char * src);
/**
 * @brief Copy at most n characters.
 */
void strncpy(char * dest, const char * src, int n);
/**
 * @brief Print error message.
 */
void perror(const char * s1);
/**
 * @brief Tokenize string.
 */
char * strtok(char * s1, const char * s2);
/**
 * @brief Convert string to long.
 */
int strtol(const char* str, char** endptr, int base);

#endif