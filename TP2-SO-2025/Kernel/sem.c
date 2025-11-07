// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "sem.h"
#include "interrupts.h"
#include "process.h"
#include "scheduler.h"

extern void semLock(uint8_t *lock);
extern void semUnlock(uint8_t *lock);
extern void _force_scheduler_interrupt();

static semQueue *sQueue = NULL;

static Sem findSemByName(const char *name);
static int validSem(Sem semToValid);

static int cmpPid(void *pid1, void *pid2);
static int cmpSem(void *psem1, void *psem2);


int initSemQueue(void) {
    if (sQueue != NULL) {
        panic("Semaphore queue already initialized");
    }
    sQueue = mm_alloc(sizeof(semQueue));
    if (sQueue == NULL) {
        panic("Failed to allocate memory for semaphore queue");
    }
    sQueue->lock = 0;
    sQueue->sems = createQueue(cmpSem, sizeof(struct sem *));
    return sQueue->sems == NULL ? -1 : 1;
}

//! REVISAR ESTA FUNCION
void freeSemQueue(void) {
	if (sQueue == NULL || sQueue->sems == NULL) {
		return;
	}
	queueFree(sQueue->sems);
	mm_free(sQueue);
	sQueue = NULL;
}

Sem semOpen(const char *name, uint16_t value) {
    if (name == NULL || strlen(name) >= MAX_SEM_LENGTH) {
        return NULL;
    } 
    
    Sem newSem;

    semLock(&sQueue->lock);

    if (!queueIsEmpty(sQueue->sems)) {
        newSem = findSemByName(name);
        if (newSem) {
			semLock(&(newSem->lock));
			(newSem->users)++;
			semUnlock(&(newSem->lock));
			semUnlock(&(sQueue->lock));
			return newSem;
		}
    }

    newSem = mm_alloc(sizeof(sem));
    if (newSem == NULL) {
        semUnlock(&sQueue->lock);
        return NULL;
    }

    my_strcpy(newSem->name, name);
    newSem->value = value;
    newSem->lock = 0;
    newSem->users = 1;
    newSem->blockedProcesses = createQueue(cmpPid, sizeof(int));
    if (newSem->blockedProcesses == NULL) {
        mm_free(newSem);
        semUnlock(&sQueue->lock);
        return NULL;
    }

    enqueue(sQueue->sems, &newSem);
    semUnlock(&sQueue->lock);

    return newSem;
}

int semClose(Sem semToClose) {
	int toReturn = -1;
	if (validSem(semToClose)) {
		semLock(&semToClose->lock);
		semToClose->users--;
		Sem aux = semToClose;
		semLock(&sQueue->lock);
		if (semToClose->users != 0) {
			// Otros usuarios siguen usando el semáforo, no liberarlo
			semUnlock(&semToClose->lock);
			toReturn = 0;
		} else {
			// Último usuario, intentar remover de la cola global
			if (queueRemove(sQueue->sems, &aux) != NULL) {
				// Éxito: liberar todos los recursos
				queueFree(semToClose->blockedProcesses);
				mm_free(semToClose);
				toReturn = 0;
			} else {
				// FALLO: rollback del decremento para mantener consistencia
				semToClose->users++;
				semUnlock(&semToClose->lock);
				toReturn = -1;
			}
		}
		semUnlock(&sQueue->lock);
	}
	return toReturn;
}


int semPost(Sem semToPost) {
	int toReturn = -1;
	if (validSem(semToPost)) {
		semLock(&semToPost->lock);
		
		if (queueSize(semToPost->blockedProcesses) != 0) {
			// Hay procesos bloqueados esperando este semáforo
			int pid;
			if (dequeue(semToPost->blockedProcesses, &pid) != NULL) {
				
				// Obtener el proceso y desbloquearlo
				Process *proc = get_process(pid);
				if (proc != NULL) {
					unblock_process(proc);
					toReturn = 0;
				} else {
					// El proceso fue eliminado mientras estaba bloqueado.
					toReturn = 0;
				}
			}
		} else {
			// No hay procesos bloqueados, simplemente incrementar el contador
			semToPost->value++;
			toReturn = 0;
		}
		semUnlock(&semToPost->lock);
	}
	return toReturn;
}

int semWait(Sem semToWait) {
	int toReturn = -1;
	int blocked = 0;
	if (validSem(semToWait)) {
		semLock(&semToWait->lock);
		
		if (semToWait->value == 0) {
			Process *current = get_current_process();
			
            if (current == NULL) {
                semUnlock(&semToWait->lock);
                toReturn = -1;
            } else {
                int pid = get_pid(current);

                if (enqueue(semToWait->blockedProcesses, &pid) == NULL) {
                    semUnlock(&semToWait->lock);
                    toReturn = -1;
                } else {
                    // CRÍTICO: Liberar el lock ANTES de block_process()
                    semUnlock(&semToWait->lock);
                    __sync_synchronize(); // Memory barrier
                    
                    block_process(current);
                    blocked = 1;
                    toReturn = 0;
                }
            }
		} else {
			semToWait->value--;
			toReturn = 0;
			semUnlock(&semToWait->lock);
		}
		// El lock YA fue liberado en todos los casos antes de llegar aquí
		if (blocked) {
			_force_scheduler_interrupt();
		}
	}
	return toReturn;
}

int semGetValue(Sem semToGet) {
	int toReturn = -1;
	if (validSem(semToGet)) {
		semLock(&semToGet->lock);
		toReturn = semToGet->value;
		semUnlock(&semToGet->lock);
	}
	return toReturn;
}

int semGetUsersCount(Sem semToGet) {
	int toReturn = -1;
	if (validSem(semToGet)) {
		semLock(&semToGet->lock);
		toReturn = semToGet->users;
		semUnlock(&semToGet->lock);
	}
	return toReturn;
}

int semGetBlockedProcessesCount(Sem semToGet) {
	int toReturn = -1;
	if (validSem(semToGet)) {
		semLock(&semToGet->lock);
		toReturn = queueSize(semToGet->blockedProcesses);
		semUnlock(&semToGet->lock);
	}
	return toReturn;
}

int removeFromSemaphore(Sem s, int pid) {
	if (s == NULL) {
		return -1;
	}
    semLock(&s->lock);
    int result = queueRemove(s->blockedProcesses, &pid) == NULL ? -1 : 1;
    semUnlock(&s->lock);
    return result;
}

/**
 * Busca un semáforo por nombre en la cola global.
 * PRECONDICIÓN: El caller debe tener adquirido sQueue->lock
 * 
 * @param name Nombre del semáforo a buscar
 * @return Puntero al semáforo si existe, NULL en caso contrario
 */
static Sem findSemByName(const char *name) {
	// Validación de entrada
	if (name == NULL) {
		return NULL;
	}
	
	int found = 0;
	int size = queueSize(sQueue->sems);
	sem *current = NULL;  // Inicializar explícitamente para evitar variable no inicializada
	
	// Early return: si la cola está vacía, retornar inmediatamente
	if (size == 0) {
		return NULL;
	}
	
	queueBeginCyclicIter(sQueue->sems);
	for (int i = 0; i < size && !found; i++) {
		queueNextCyclicIter(sQueue->sems, &current);
		// Validación defensiva: verificar que current no sea NULL antes de acceder
		if (current != NULL && my_strcmp(current->name, name) == 0) {
			found = 1;
		}
	}
	
	return found ? current : NULL;
}

static int validSem(Sem semToValid) {
	int valid = 0;
	if (semToValid != NULL) {
		Sem aux = semToValid;
		semLock(&sQueue->lock);
		if (queueElementExists(sQueue->sems, &aux)) {
			valid = 1;
		}
		semUnlock(&sQueue->lock);
	}
	return valid;
}


static int cmpPid(void *pid1, void *pid2) { 
    return *((int *)pid1) - *((int *)pid2); 
}

static int cmpSem(void *psem1, void *psem2) { 
    return *((Sem *)psem1) - *((Sem *)psem2); 
}