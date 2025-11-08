#include "mvar.h"

#define MVAR_SEM_MUTEX_NAME "mvar_mutex"
#define MVAR_SEM_EMPTY_NAME "mvar_empty"
#define MVAR_SEM_FULL_NAME "mvar_full"

typedef struct Mvar {
    char value;           // The actual data storage (single character)
    int readers;
    int writers;
    Sem mutex;
    Sem empty;
    Sem full;
} Mvar;

typedef Mvar* MvarADT;

static MvarADT mvar;

int init_mvar(int readers, int writers) {
    if (readers <= 0 || writers <= 0) {
        return -1;
    }

    mvar = mm_alloc(sizeof(Mvar));
    if (mvar == NULL) {
        return -1;
    }

    mvar->readers = readers;
    mvar->writers = writers;
    mvar->value = 0;

    mvar->empty = semOpen(MVAR_SEM_EMPTY_NAME, 1);
    if (mvar->empty == NULL) {
        mm_free(mvar);
        return -1;
    }

    // Initialize full with 0: no data available for reading yet
    mvar->full = semOpen(MVAR_SEM_FULL_NAME, 0);
    if (mvar->full == NULL) {
        semClose(mvar->empty);
        mm_free(mvar);
        return -1;
    }

    return 0;
}

// Put a value into the MVar (blocking if full)
int put_mvar(char value) {
    if (mvar == NULL) {
        return -1;
    }

    // Wait until MVar is empty
    semWait(mvar->empty);
    
    // Write the value
    mvar->value = value;
    
    // Signal that MVar now has data
    semPost(mvar->full);
    
    return 0;
}

// Get a value from the MVar (blocking if empty)
char get_mvar(void) {
    if (mvar == NULL) {
        return 0;
    }

    // Wait until MVar has data
    semWait(mvar->full);
    
    // Read the value
    char value = mvar->value;
    mvar->value = 0;
    
    // Signal that MVar is now empty
    semPost(mvar->empty);
    
    return value;
}

// Cleanup the MVar
int close_mvar(void) {
    if (mvar == NULL) {
        return -1;
    }

    semClose(mvar->empty);
    semClose(mvar->full);
    mm_free(mvar);
    mvar = NULL;

    return 0;
}
