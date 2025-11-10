#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>
#include <stddef.h>

#define MAX_PIPES 100
#define PIPE_BUFFER_SIZE (16 * 1024)

#define STDIN  0
#define STDOUT 1
#define STDERR 2

typedef struct pipe * Pipe;

/**
 * @brief Initialize global pipe storage.
 * @return 0 on success, -1 on error.
 */
int initPipeStorage(void);

/**
 * @brief Create a new pipe and return its id.
 * @return Pipe id (uint8_t promoted to int) on success, -1 on error.
 */
int openPipe(void);

/**
 * @brief Attach current process as reader for a pipe.
 * @param id Pipe id.
 * @return 0 on success, -1 on error.
 */
int attachReader(uint8_t id);
/**
 * @brief Attach current process as writer for a pipe.
 * @param id Pipe id.
 * @return 0 on success, -1 on error.
 */
int attachWriter(uint8_t id);
/**
 * @brief Detach current process as reader from a pipe.
 * @param id Pipe id.
 * @param pid Process id to detach.
 */
void detachReader(uint8_t id, int pid);
/**
 * @brief Detach current process as writer from a pipe.
 * @param id Pipe id.
 * @param pid Process id to detach.
 */
void detachWriter(uint8_t id, int pid);

/**
 * @brief Read from a pipe.
 * @param id Pipe id.
 * @param buffer Destination buffer.
 * @param bytes Max bytes to read.
 * @return Number of bytes read, or -1 on error.
 */
int readPipe(uint8_t id, uint8_t * buffer, uint64_t bytes);

/**
 * @brief Write to a pipe.
 * @param id Pipe id.
 * @param buffer Source buffer.
 * @param bytes Bytes to write.
 * @return Number of bytes written, or -1 on error.
 */
int writePipe(uint8_t id, const uint8_t * buffer, uint64_t bytes);

/**
 * @brief Unblock a reader waiting on a pipe.
 * @param id Pipe id.
 */
void unblockPipeReader(uint8_t id);

/**
 * @brief Close a pipe by id.
 * @param id Pipe id.
 * @return 0 on success, -1 on error.
 */
int closePipe(uint8_t id);

/**
 * @brief Free all global pipe storage.
 */
void freePipeStorage(void);

/**
 * @brief Reset pipe storage to an initial state.
 */
void resetPipeStorage(void);

/**
 * @brief Set the next id that will be assigned to a new pipe.
 * @param id New next id.
 */
void setNextId(uint8_t id);

#endif
