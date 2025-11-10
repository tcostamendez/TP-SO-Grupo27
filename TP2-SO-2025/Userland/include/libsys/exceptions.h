#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

/**
 * @brief Trigger a divide-by-zero exception (INT 0).
 */
void _divzero(void);
/**
 * @brief Trigger an invalid opcode exception (INT 6).
 */
void _invalidopcode();

#endif

