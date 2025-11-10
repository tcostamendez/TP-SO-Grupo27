#ifndef FD_H
#define FD_H

#include <stdint.h>
#include "pipe.h"  

#define READ_FD  0
#define WRITE_FD 1
#define ERR_FD 2


/**
 * @brief Set the read target (STDIN/pipe id) for a process FD mapping.
 * @param targetByFd Array mapping FD indices to targets.
 * @param target New read target (STDIN or pipe id).
 * @return 0 on success, -1 on error.
 */
int setReadTarget(uint8_t targetByFd[2], uint8_t target);
/**
 * @brief Set the write target (STDOUT/pipe id) for a process FD mapping.
 * @param targetByFd Array mapping FD indices to targets.
 * @param target New write target (STDOUT or pipe id).
 * @return 0 on success, -1 on error.
 */
int setWriteTarget(uint8_t targetByFd[2], uint8_t target);

/**
 * @brief Read from a file descriptor (STDIN or pipe).
 * @param fd Descriptor (READ_FD).
 * @param userBuff Destination buffer.
 * @param count Max bytes to read.
 * @return Number of bytes read, or -1 on error.
 */
int fd_read(int32_t fd, uint8_t *userBuff, int32_t count);
/**
 * @brief Write to a file descriptor (STDOUT or pipe).
 * @param fd Descriptor (WRITE_FD).
 * @param userBuff Source buffer.
 * @param count Bytes to write.
 * @return Number of bytes written, or -1 on error.
 */
int fd_write(int32_t fd, const uint8_t *userBuff, int32_t count);

#endif
