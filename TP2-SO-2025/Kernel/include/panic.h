#ifndef PANIC_H
#define PANIC_H

#include "fonts.h"

/**
 * @brief Halt the kernel with an error message and context (line/file).
 * Prints a red panic banner and disables interrupts before halting.
 */
#define panic(msg)                                      \
	{                                                   \
		setTextColor(0xFF0000);                       	\
		print("====== KERNEL PANIC (Line ");                 \
		printDec(__LINE__);                                \
		print(" @ "); print(__FILE__ ") ======\n");		\
		print(msg);                                     \
		print("\n");                                    \
		__asm__("cli; hlt");                            \
	}

#endif