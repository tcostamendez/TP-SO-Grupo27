// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "pipe.h"

#include "array.h"
#include "panic.h"
#include "process.h"
#include "sem.h"
#include "strings.h"
#include "memory_manager.h"

#define SEM_NAME_LONG 12

#define NEXT_IDX(i) (((i) + 1) % (PIPE_BUFFER_SIZE))
#define PREVIOUS_IDX(i) ((i) == 0 ? ((PIPE_BUFFER_SIZE) - 1) : ((i) - 1))

struct pipe {
    uint8_t id;
    uint8_t buffer[PIPE_BUFFER_SIZE];
    uint64_t idxToRead;
    uint64_t idxToWrite;
    uint8_t waitingToRead;
    uint8_t waitingToWrite;
    Sem canRead;
    Sem canWrite;
    uint8_t attached;

    // Contadores por rol
    uint16_t readers;
    uint16_t writers;

    // Expectativas iniciales (evitan EOF antes de que conecte el otro extremo)
    uint8_t exp_readers;
    uint8_t exp_writers;
};

static Pipe createPipe(void);
static void buildSemName(uint8_t id, char suffix, char out[SEM_NAME_LONG]);
static int getValidPipe(uint8_t id, Pipe *buffer);
static int deletePipe(Pipe pipeToDelete);
static void freePipe(Pipe pipeToFree);

static int nextId = 3;

static ArrayADT pipeStorage;

int initPipeStorage(void) {
    pipeStorage = createArray(sizeof(struct pipe *));
    if (pipeStorage == NULL) {
        panic("Not enough memory to create pipes.");
        return -1;
    }

    return 0;
}

int openPipe(void) {
    if (nextId == MAX_PIPES) {
        return -1;
    }

    Pipe pipe = createPipe();

    if (pipe == NULL) {
        return -1;
    }

    if (NULL == arrayAdd(pipeStorage, &pipe)) {
        return -1;
    }

    return pipe->id;
}

int attachReader(uint8_t id) {
    Pipe p;
    if (-1 == getValidPipe(id, &p)) return -1;
    p->readers++;
    p->attached++;
    if (p->exp_readers > 0) p->exp_readers--;
    return 0;
}

int attachWriter(uint8_t id) {
    Pipe p;
    if (-1 == getValidPipe(id, &p)) return -1;
    p->writers++;
    p->attached++;
    if (p->exp_writers > 0) p->exp_writers--;
    return 0;
}

static void wake_all(Sem s) {
    int users = semGetUsersCount(s);
    while (users-- > 0) semPost(s);
}

void detachReader(uint8_t id, int pid) {
    Pipe p;
    if (-1 == getValidPipe(id, &p)) return;

    removeFromSemaphore(p->canRead, pid);
    removeFromSemaphore(p->canWrite, pid);

    if (p->readers > 0) {
        p->readers--;
        if (p->attached > 0) p->attached--;
    }

    if (p->readers == 0 && p->waitingToWrite) {
        wake_all(p->canWrite);
        p->waitingToWrite = 0;
    }

    if (p->readers == 0 && p->writers == 0 && p->exp_readers == 0 && p->exp_writers == 0) {
        deletePipe(p);
    }
}

void detachWriter(uint8_t id, int pid) {
    Pipe p;
    if (-1 == getValidPipe(id, &p)) return;

    removeFromSemaphore(p->canRead, pid);
    removeFromSemaphore(p->canWrite, pid);

    if (p->writers > 0) {
        p->writers--;
        if (p->attached > 0) p->attached--;
    }

    if (p->writers == 0 && p->waitingToRead) {
        wake_all(p->canRead);
        p->waitingToRead = 0;
    }

    if (p->readers == 0 && p->writers == 0 && p->exp_readers == 0 && p->exp_writers == 0) {
        deletePipe(p);
    }
}

int readPipe(uint8_t id, uint8_t *buffer, uint64_t bytes) {
    Pipe pipeToRead;

    if (-1 == getValidPipe(id, &pipeToRead) || buffer == NULL) {
        return -1;
    }

    if (pipeToRead->idxToRead == pipeToRead->idxToWrite) {
        if (pipeToRead->writers == 0 && pipeToRead->exp_writers == 0) {
            return 0; 
        } else {
            pipeToRead->waitingToRead = 1;
            if (-1 == semWait(pipeToRead->canRead)) {
                panic("Pipe's sem didn't work.");
                return -1;
            }
        }
    }

    int readBytes = 0;

    while (readBytes < bytes && pipeToRead->idxToRead != pipeToRead->idxToWrite) {
        *(buffer + readBytes) = pipeToRead->buffer[pipeToRead->idxToRead];
        pipeToRead->idxToRead = NEXT_IDX(pipeToRead->idxToRead);
        readBytes++;
    }

    if (readBytes > 0 && pipeToRead->waitingToWrite) {
        pipeToRead->waitingToWrite = 0;
        if (-1 == semPost(pipeToRead->canWrite)) {
            panic("Pipe's sem didn't work.");
            return -1;
        }
    }

    return readBytes;
}

int writePipe(uint8_t id, const uint8_t *buffer, uint64_t bytes) {
    Pipe pipeToWrite;

    if (-1 == getValidPipe(id, &pipeToWrite) || buffer == NULL) {
        return -1;
    }

    int writtenBytes = 0;

    while (writtenBytes < (int)bytes) {
        if (pipeToWrite->idxToWrite == PREVIOUS_IDX(pipeToWrite->idxToRead)) {
            pipeToWrite->waitingToWrite = 1;
            if (-1 == semWait(pipeToWrite->canWrite)) {
                panic("Pipe's sem didn't work.");
                return -1;
            }
        }

        pipeToWrite->buffer[pipeToWrite->idxToWrite] = *(buffer + writtenBytes);
        pipeToWrite->idxToWrite = NEXT_IDX(pipeToWrite->idxToWrite);
        writtenBytes++;
    }

    if (writtenBytes > 0 && pipeToWrite->waitingToRead) {
        pipeToWrite->waitingToRead = 0;
        if (-1 == semPost(pipeToWrite->canRead)) {
            panic("Pipe's sem didn't work.");
            return -1;
        }
    }

    return writtenBytes;
}

void unblockPipeReader(uint8_t id) {
    Pipe pipeToUnblock;

    if (-1 == getValidPipe(id, &pipeToUnblock)) {
        return;
    }

    if (pipeToUnblock->waitingToRead > 0) {
        pipeToUnblock->waitingToRead = 0;
        if (-1 == semPost(pipeToUnblock->canRead)) {
            panic("Pipe's sem didn't work.");
        }
    }
}

int closePipe(uint8_t id) {
    Pipe pipeToClose;

    if (-1 == getValidPipe(id, &pipeToClose)) {
        return -1;
    }

    if ((id != STDIN && id != STDOUT) && pipeToClose->attached <= 1) {
        int users = semGetUsersCount(pipeToClose->canRead);
        while (users--) {
            semPost(pipeToClose->canRead);
        }
        users = semGetUsersCount(pipeToClose->canWrite);
        while (users--) {
            semPost(pipeToClose->canWrite);
        }
    }

    if (pipeToClose->attached == 0 || ((id == STDIN || id == STDOUT) && pipeToClose->attached == 1)) {
        if (-1 == deletePipe(pipeToClose)) {
            panic("Couldn't delete pipe.");
            return -1;
        }
    }

    return 0;
}

void freePipeStorage(void) {
    resetPipeStorage();
    arrayFree(pipeStorage);
}

void resetPipeStorage(void) {
    nextId = 0;
    arrayResetSize(pipeStorage);
}

void setNextId(uint8_t id) { nextId = id; }

static Pipe createPipe(void) {
    Pipe newPipe = mm_alloc(sizeof(struct pipe));
    if (newPipe == NULL) {
        return NULL;
    }

    newPipe->id = nextId;

    newPipe->idxToRead = 0;
    newPipe->idxToWrite = 0;

    newPipe->waitingToRead = 0;
    newPipe->waitingToWrite = 0;

    newPipe->attached = 0;
    newPipe->readers = 0;
    newPipe->writers = 0;
    newPipe->exp_readers = 1;
    newPipe->exp_writers = 1;

    char name[SEM_NAME_LONG];
    buildSemName(newPipe->id, 'R', name);
    newPipe->canRead = semOpen(name, 0);
    if (newPipe->canRead == NULL) {
        freePipe(newPipe);
        return NULL;
    }

    buildSemName(newPipe->id, 'W', name);
    newPipe->canWrite = semOpen(name, 0);
    if (newPipe->canWrite == NULL) {
        semClose(newPipe->canRead);
        freePipe(newPipe);
        return NULL;
    }

    newPipe->attached = 0;

    nextId++;

    return newPipe;
}

static void buildSemName(uint8_t id, char suffix, char out[SEM_NAME_LONG]) {
    out[0]='s'; out[1]='e'; out[2]='m'; out[3]='P'; out[4]='i'; out[5]='p'; out[6]='e';
    out[7] = '0' + (id / 10);
    out[8] = '0' + (id % 10);
    out[9] = suffix;
    out[10] = '\0';
}

static int getValidPipe(uint8_t id, Pipe *buffer) {
    if (buffer == NULL || pipeStorage == NULL) {
        return -1;
    }
    uint16_t size = arraySize(pipeStorage);
    for (uint16_t i = 0; i < size; i++) {
        Pipe tmp = NULL;
        if (getElemByIndex(pipeStorage, i, &tmp) == NULL) {
            panic("Invalid pipe array.");
            return -1;
        }
        if (tmp != NULL && tmp->id == id) {
            *buffer = tmp;
            return 0;
        }
    }
    return -1; 
}

static int deletePipe(Pipe pipeToDelete) {
    if (pipeToDelete == NULL || pipeStorage == NULL) {
        return -1;
    }
    uint16_t size = arraySize(pipeStorage);
    for (uint16_t i = 0; i < size; i++) {
        Pipe tmp = NULL;
        if (getElemByIndex(pipeStorage, i, &tmp) == NULL) {
            panic("Invalid pipe array.");
            return -1;
        }
        if (tmp == pipeToDelete) {
            Pipe nullValue = NULL;
            if (setElemByIndex(pipeStorage, i, &nullValue) == NULL) {
                return -1;
            }
            freePipe(pipeToDelete);
            return 0;
        }
    }
    return -1; 
}

static void freePipe(Pipe pipeToFree) {
    if (pipeToFree != NULL) {
        semClose(pipeToFree->canRead);
        semClose(pipeToFree->canWrite);
        mm_free(pipeToFree);
    }
}

void debugPipe(uint8_t id) { (void)id; }
