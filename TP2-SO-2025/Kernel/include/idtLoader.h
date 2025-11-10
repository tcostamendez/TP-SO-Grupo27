#ifndef _IDT_LOADER_H_
#define _IDT_LOADER_H_

#include <stdint.h>

#include <defs.h>
#include <interrupts.h>

/**
 * @brief Load and activate the Interrupt Descriptor Table (IDT).
 */
void load_idt(void);

#endif
