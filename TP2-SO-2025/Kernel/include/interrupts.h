 /*
 *  Created on: Apr 18, 2010
 *      Author: anizzomc
 */

#ifndef INTERRUPS_H
#define INTERRUPS_H

#include <stdint.h>

extern void (*_irq00Handler) (void);
extern void (*_irq01Handler) (void);
extern void (*_irq80Handler) (void);

extern void (*_exceptionHandler00) (void);
extern void (*_exceptionHandler06) (void);
extern void (*_exceptionHandler08) (void); 
extern void (*_exceptionHandler0D) (void); 
extern void (*_exceptionHandler0E) (void); 

/**
 * @brief Disable interrupts (CLI).
 */
void _cli(void);

/**
 * @brief Enable interrupts (STI).
 */
void _sti(void);

/**
 * @brief Halt CPU until next interrupt (HLT).
 */
void _hlt(void);

/**
 * @brief Mask IRQ lines on master PIC.
 * @param mask Bitmask (1 = masked/disabled).
 */
void picMasterMask(uint8_t mask);

/**
 * @brief Mask IRQ lines on slave PIC.
 * @param mask Bitmask (1 = masked/disabled).
 */
void picSlaveMask(uint8_t mask);

#define TIMER_PIC_MASTER 0xFE
#define KEYBOARD_PIC_MASTER 0xFD
#define NO_INTERRUPTS 0xFF

#endif
