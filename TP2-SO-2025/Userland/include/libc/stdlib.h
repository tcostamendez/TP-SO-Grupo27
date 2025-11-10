#ifndef _LIBC_STDLIB_H_
#define _LIBC_STDLIB_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Pseudo-random number generator.
 * @return Integer in implementation-defined range.
 */
int rand(void);

/**
 * @brief Seed the pseudo-random number generator.
 * @param seed Seed value.
 */
void srand(unsigned int seed);

/**
 * @brief Convert string to integer.
 * @param str Null-terminated string.
 * @return Converted value or 0 on error.
 */
int atoi(const char *str);

/**
 * @brief Convert integer to string in a base.
 * @param value Value to convert.
 * @param str Output buffer.
 * @param base Base (2..36).
 * @return str.
 */
char *itoa(int value, char *str, int base);

/**
 * @brief Set memory block to byte value.
 */
void *memset(void *destination, int32_t c, uint64_t length);

/**
 * @brief Copy memory block.
 */
void *memcpy(void *destination, const void *source, uint64_t length);

/**
 * @brief Get free bytes in userland heap.
 */
int get_free_bytes(void);

/**
 * @brief Get used bytes in userland heap.
 */
int get_used_bytes(void);

/**
 * @brief Get total bytes in userland heap.
 */
int get_total_bytes(void);

#endif