#include "fd.h"
#include "pipe.h"
#include "process.h"
#include "fonts.h"

int setReadTarget(uint8_t targetByFd[2], uint8_t target) {
    if (targetByFd == NULL) {
        return -1;
    }

    targetByFd[READ_FD] = target;

    return 0;
}

int setWriteTarget(uint8_t targetByFd[2], uint8_t target) {
    if (targetByFd == NULL) {
        return -1;
    }

    targetByFd[WRITE_FD] = target;

    return 0;
}

int fd_read(int32_t fd, uint8_t *userBuff, int32_t count) {
    Process *proc = get_current_process();

    if (proc == NULL) {
        return -1;
    }

    // For now, only pipe-backed reads are supported. STDIN not handled here.
    // Map FD to target pipe id
    uint8_t target = proc->targetByFd[fd];
    return readPipe(target, userBuff, (uint64_t)count);
}

int fd_write(int32_t fd, const uint8_t *userBuff, int32_t count) {
    Process *proc = get_current_process();

    if (proc == NULL) {
        return -1;
    }

    uint8_t target = proc->targetByFd[fd];

    // Fast path: write to terminal if targeting STDOUT directly
    if (fd == WRITE_FD && target == STDOUT) {
        // printToFd can handle raw bytes with count
        return printToFd(STDOUT, (const char *)userBuff, count);
    }

    return writePipe(target, userBuff, (uint64_t)count);
}
