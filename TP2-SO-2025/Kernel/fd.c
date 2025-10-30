#include "fd.h"
#include "pipe.h"
#include "process.h"
#include "fonts.h"
#include "keyboard.h"

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

    // Map FD to target; if STDIN, read from keyboard buffer
    uint8_t target = proc->targetByFd[fd];
    if (fd == READ_FD && target == STDIN) {
        int32_t i = 0;
        int8_t c;
        // Mimic sys_read semantics: block until RETURN or count reached
        while (i < count && (c = getKeyboardCharacter(AWAIT_RETURN_KEY | SHOW_BUFFER_WHILE_TYPING)) != EOF) {
            *(userBuff + i) = c;
            i++;
        }
        return i;
    }

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
