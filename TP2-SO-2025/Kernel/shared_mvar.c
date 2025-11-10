#include "shared_mvar.h"

// Single shared byte for MVar value storage
// Synchronization is handled in userland via semaphores
static char shared_mvar_value = 0;

void set_mvar_value(char value) {
    shared_mvar_value = value;
}

char get_mvar_value(void) {
    return shared_mvar_value;
}

