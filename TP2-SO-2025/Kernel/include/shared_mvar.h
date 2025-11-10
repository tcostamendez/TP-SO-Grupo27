#ifndef _SHARED_MVAR_H_
#define _SHARED_MVAR_H_

// Minimal kernel support for MVar: just a shared byte storage
// All synchronization logic is in userland using semaphores

void set_mvar_value(char value);
char get_mvar_value(void);

#endif

