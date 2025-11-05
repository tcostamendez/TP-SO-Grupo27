#include "libsys.h"

int _exit(int argc, char *argv[]) {
    shutdown();
    return 0;
}