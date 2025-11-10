#ifndef VIDEO_DRIVER_H
#define VIDEO_DRIVER_H

#include <stdint.h>

/**
 * @brief Set a pixel at position (x,y) with the given color.
 * @param hexColor 0xRRGGBB color value.
 * @param x Column in pixels.
 * @param y Row in pixels.
 */
void putPixel(uint32_t hexColor, uint64_t x, uint64_t y);
/**
 * @brief Draw a circle inside the bounding box defined by top-left and diameter.
 * @param hexColor 0xRRGGBB color value.
 * @param topLeftX Top-left X pixel.
 * @param topLeftY Top-left Y pixel.
 * @param diameter Diameter in pixels.
 */
void drawCircle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter);
/**
 * @brief Draw a filled rectangle.
 * @param hexColor 0xRRGGBB color value.
 * @param width Rectangle width in pixels.
 * @param height Rectangle height in pixels.
 * @param initial_pos_x Top-left X.
 * @param initial_pos_y Top-left Y.
 */
void drawRectangle(uint32_t hexColor, uint64_t width, uint64_t height, uint64_t initial_pos_x, uint64_t initial_pos_y);
/**
 * @brief Fill the entire video memory with a color.
 * @param hexColor 0xRRGGBB color value.
 */
void fillVideoMemory(uint32_t hexColor);

/**
 * @brief Get display width in pixels.
 */
uint16_t getWindowWidth(void);
/**
 * @brief Get display height in pixels.
 */
uint16_t getWindowHeight(void);

/**
 * @brief Scroll video memory up by a pixel count and fill emptied area.
 * @param scroll Pixels to scroll up.
 * @param fillColor 0xRRGGBB color value.
 */
void scrollVideoMemoryUp(uint16_t scroll, uint32_t fillColor);

#endif