#ifndef _SHARED_MVAR_H_
#define _SHARED_MVAR_H_

/**
 * @brief Minimal kernel support for a shared byte (MVar).
 * Synchronization is handled in userland via semaphores.
 */

/**
 * @brief Set the shared MVar byte value.
 * @param value New value to set.
 */
void set_mvar_value(char value);
/**
 * @brief Get the shared MVar byte value.
 * @return Current value.
 */
char get_mvar_value(void);

#endif

