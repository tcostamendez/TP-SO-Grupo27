#ifndef _LIBC_STDIO_H_
#define _LIBC_STDIO_H_

#include <string.h>
#include <stdarg.h>

#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

#define EOF -1

/**
 * @brief Print a string followed by newline to STDOUT.
 */
void puts(const char * str);
/**
 * @brief Printf with va_list.
 */
void vprintf(const char * str, va_list args);
/**
 * @brief Print formatted output to STDOUT.
 */
void printf(const char * str, ...);
/**
 * @brief Print formatted output to a file descriptor.
 */
void fprintf(int fd, const char * str, ...);
/**
 * @brief Scan input with va_list.
 */
int vscanf(const char * format, va_list args);
/**
 * @brief Scan from buffer with va_list.
 */
int vsscanf(const char * buffer, const char * format, va_list args);
/**
 * @brief Scan from string.
 */
int sscanf(const char * str, const char * format, ...);
/**
 * @brief Scan from STDIN.
 */
int scanf(const char * format, ...);
/**
 * @brief Read a character from STDIN.
 */
int getchar(void);
/**
 * @brief Write a character to STDOUT.
 */
void putchar(const char c);

#endif