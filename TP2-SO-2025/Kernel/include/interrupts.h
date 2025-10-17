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
extern void (*_exceptionHandler08) (void); // <-- AÑADIR
extern void (*_exceptionHandler0D) (void); // <-- AÑADIR
extern void (*_exceptionHandler0E) (void); // <-- AÑADIR

void _cli(void);

void _sti(void);

void _hlt(void);

void picMasterMask(uint8_t mask);

void picSlaveMask(uint8_t mask);

void _load_idt_asm(void *idtr);

#define TIMER_PIC_MASTER 0xFE
#define KEYBOARD_PIC_MASTER 0xFD
#define NO_INTERRUPTS 0xFF

#endif
