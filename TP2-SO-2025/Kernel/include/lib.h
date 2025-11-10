#ifndef LIB_H
#define LIB_H

#include <stdint.h>
#include <fonts.h>

#define EOF -1

/**
 * @brief Set a block of memory to a byte value.
 * @param destination Pointer to the memory area.
 * @param character Byte value to set (interpreted as unsigned).
 * @param length Number of bytes to set.
 * @return destination.
 */
void * memset(void * destination, int32_t character, uint64_t length);
/**
 * @brief Copy a block of memory.
 * @param destination Destination buffer.
 * @param source Source buffer.
 * @param length Number of bytes to copy.
 * @return destination.
 */
void * memcpy(void * destination, const void * source, uint64_t length);

/**
 * @brief Get last key read from keyboard buffer (raw).
 * @return Raw key code or 0 if none.
 */
uint8_t getKeyboardBuffer(void);
/**
 * @brief Get keyboard controller status.
 * @return Status byte.
 */
uint8_t getKeyboardStatus(void);

/**
 * @brief Read current RTC second.
 * @return Second [0..59].
 */
uint8_t getSecond(void);
/**
 * @brief Read current RTC minute.
 * @return Minute [0..59].
 */
uint8_t getMinute(void);
/**
 * @brief Read current RTC hour.
 * @return Hour [0..23].
 */
uint8_t getHour(void);

#endif