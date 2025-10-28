#ifndef FD_H
#define FD_H

#include <stdint.h>
#include "pipe.h" // for STDIN/STDOUT and Pipe API

#define READ_FD  0
#define WRITE_FD 1

int setReadTarget(uint8_t targetByFd[2], uint8_t target);
int setWriteTarget(uint8_t targetByFd[2], uint8_t target);

// Kernel-level helpers (not syscalls):
int fd_read(int32_t fd, uint8_t *userBuff, int32_t count);
int fd_write(int32_t fd, const uint8_t *userBuff, int32_t count);

#endif
