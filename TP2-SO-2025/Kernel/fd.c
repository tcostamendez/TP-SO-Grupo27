// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
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

    /* Validaciones básicas de los argumentos */
    if (userBuff == NULL || count < 0) return -1;
    if (fd < 0 || fd > ERR_FD) return -1;

    uint8_t target = proc->targetByFd[fd];
    if (fd == READ_FD && target == STDIN) {
        int32_t i = 0;
        int c;
        while (i < count && (c = getKeyboardCharacter(AWAIT_RETURN_KEY | SHOW_BUFFER_WHILE_TYPING)) != EOF) {
            *(userBuff + i) = (uint8_t)c;
            i++;
        }
        return i;
    }
    // Leer desde pipe: si count > tamaño buffer interno, hacer lecturas parciales
    return readPipe(target, userBuff, (uint64_t)count);
}

int fd_write(int32_t fd, const uint8_t *userBuff, int32_t count) {
    Process *proc = get_current_process();

    if (proc == NULL) {
        return -1;
    }

    /* Validaciones básicas de los argumentos */
    if (userBuff == NULL || count < 0) return -1;
    if (fd < 0 || fd > ERR_FD) return -1;

    uint8_t target = proc->targetByFd[fd];

    if ((fd == WRITE_FD && target == STDOUT) || (fd == ERR_FD && target == STDERR) ){
        return printToFd(target, (const char *)userBuff, count);
    }
    // Escritura a pipe: bloqueo hasta escribir todos los bytes (writePipe ya bloquea si lleno)
    return writePipe(target, userBuff, (uint64_t)count);
}
