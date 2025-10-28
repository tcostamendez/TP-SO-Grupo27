#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>
#include <stddef.h>

#define MAX_PIPES 100
#define PIPE_BUFFER_SIZE (16 * 1024)

// Conventional std fds (used as special targets)
#define STDIN  0
#define STDOUT 1

// Opaque handle
typedef struct pipe * Pipe;

int initPipeStorage(void);

int openPipe(void);

int attach(uint8_t id);

int readPipe(uint8_t id, uint8_t * buffer, uint64_t bytes);

int writePipe(uint8_t id, const uint8_t * buffer, uint64_t bytes);

void removeAttached(uint8_t id, int pid);

void unblockPipeReader(uint8_t id);

int closePipe(uint8_t id);

void freePipeStorage(void);

void resetPipeStorage(void);

void setNextId(uint8_t id);

#endif
