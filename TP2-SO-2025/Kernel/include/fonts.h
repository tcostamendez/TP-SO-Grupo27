#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>
#include <video.h>

#define DEFAULT_TEXT_COLOR 0x00FFFFFF
#define DEFAULT_ERROR_COLOR 0x00DE382B
#define DEFAULT_BACKGROUND_COLOR 0x00000000

#define DEFAULT_GLYPH_SIZE_X 8
#define DEFAULT_GLYPH_SIZE_Y 8

#define NEW_LINE_CHAR '\n'
#define TABULATOR_CHAR '\t'
#define CARRIAGE_RETURN_CHAR '\r'
#define ESCAPE_CHAR '\e'
#define TAB_SIZE 4

/**
 * @brief Write a single character to the screen at the current cursor.
 * @param ascii Character to print.
 */
void putChar(char ascii);
/**
 * @brief Print a null-terminated string.
 * @param string Text to print.
 */
void print(const char * string);
/**
 * @brief Write a string to a file descriptor (e.g., STDOUT or pipe).
 * @param fd Descriptor.
 * @param string Text to write.
 * @param count Max bytes to write (-1 for full).
 * @return Number of bytes written or -1 on error.
 */
int32_t printToFd(int32_t fd, const char * string, int32_t count);
/**
 * @brief Advance to a new line.
 */
void newLine();
/**
 * @brief Print an unsigned integer in decimal.
 */
void printDec(uint64_t value);
/**
 * @brief Print an unsigned integer in hexadecimal.
 */
void printHex(uint64_t value);
/**
 * @brief Print an unsigned integer in binary.
 */
void printBin(uint64_t value);
/**
 * @brief Clear the screen and reset cursor.
 */
void clear(void);

/**
 * @brief Show the cursor.
 */
void showCursor(void);
/**
 * @brief Hide the cursor.
 */
void hideCursor(void);
/**
 * @brief Move cursor one position back.
 */
void retractPosition();
/**
 * @brief Clear the previous character (backspace effect).
 */
void clearPreviousCharacter(void);
/**
 * @brief Get the current X position of the text cursor buffer.
 * @return X position in characters.
 */
uint16_t getXBufferPosition(void);

/**
 * @brief Increase current font size (bounded).
 * @return New size.
 */
uint8_t increaseFontSize(void);
/**
 * @brief Decrease current font size (bounded).
 * @return New size.
 */
uint8_t decreaseFontSize(void);
/**
 * @brief Set current font size.
 * @param size New size.
 * @return Final size (may be clamped).
 */
uint8_t setFontSize(int8_t size);
/**
 * @brief Get current font size.
 */
uint8_t getFontSize(void);
/**
 * @brief Set text color.
 * @param color 0xRRGGBB color.
 */
void setTextColor(uint32_t color);
/**
 * @brief Set background color.
 * @param color 0xRRGGBB color.
 */
void setBackgroundColor(uint32_t color);
/**
 * @brief Get text color.
 * @return 0xRRGGBB color.
 */
uint32_t getTextColor(void);
/**
 * @brief Get background color.
 * @return 0xRRGGBB color.
 */
uint32_t getBackgroundColor(void);

#endif