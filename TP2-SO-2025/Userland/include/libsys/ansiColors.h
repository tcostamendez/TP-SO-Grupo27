#ifndef _ANSI_COLORS_H_
#define _ANSI_COLORS_H_

#ifdef ANSI_4_BIT_COLOR_SUPPORT

#include <stdint.h>

/**
 * @brief Parse ANSI escape sequence starting at string[i] and apply effects.
 * @param string Input string containing escape sequences.
 * @param i In/out index into string; advanced past the sequence.
 */
void parseANSI(const char * string, int * i);

#endif

#endif