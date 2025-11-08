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
	if (sQueue == NULL) {
		return;
	}
	/* Proteger la operación de liberación global */
	semLock(&sQueue->lock);
	if (sQueue->sems != NULL) {
		/* Vaciar y liberar cada semáforo dentro de la cola */
		while (queueSize(sQueue->sems) > 0) {
			Sem s = NULL;
			if (dequeue(sQueue->sems, &s) != NULL && s != NULL) {
				/* liberar las estructuras internas del semáforo */
				if (s->blockedProcesses)
					queueFree(s->blockedProcesses);
				/* no debemos tocar s->lock después de mm_free, así que no hacemos semUnlock aquí */
				mm_free(s);
			}
		}
		queueFree(sQueue->sems);
	}
	semUnlock(&sQueue->lock);
	mm_free(sQueue);
	sQueue = NULL;
}

Sem semOpen(const char *name, uint16_t value) {
    if (name == NULL || strlen(name) >= MAX_SEM_LENGTH) {
        return NULL;
    } 
    
    Sem newSem;

	if (sQueue == NULL) {
		return NULL;
	}

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
	if (sQueue == NULL) {
		return -1;
	}
	if (!validSem(semToClose)) {
		return -1;
	}

	/* Para evitar deadlocks, adquirir primero sQueue->lock y luego el sem->lock */
	semLock(&sQueue->lock);
	semLock(&semToClose->lock);

	/* Decrementar contador de usuarios */
	semToClose->users--;

	if (semToClose->users != 0) {
		/* Otros usuarios siguen usando el semáforo, no liberarlo */
		semUnlock(&semToClose->lock);
		semUnlock(&sQueue->lock);
		return 0;
	}

	/* Último usuario: intentar remover de la cola global */
	Sem aux = semToClose;
	if (queueRemove(sQueue->sems, &aux) == NULL) {
		/* FALLO: rollback del decremento para mantener consistencia */
		semToClose->users++;
		semUnlock(&semToClose->lock);
		semUnlock(&sQueue->lock);
		return -1;
	}

	/* Fue removido exitosamente de la cola global. Mientras aún tenemos el sem->lock,
	   podemos liberar sus estructuras internas de forma segura. */
	if (semToClose->blockedProcesses) {
		queueFree(semToClose->blockedProcesses);
		semToClose->blockedProcesses = NULL;
	}

	/* Liberar el lock del semáforo antes de liberar la memoria */
	semUnlock(&semToClose->lock);

	/* Ya no formamos parte de la cola global; liberar la estructura */
	mm_free(semToClose);

	/* Finalmente liberar el lock global */
	semUnlock(&sQueue->lock);

	return 0;
}


int semPost(Sem semToPost) {
	int toReturn = -1;
	if (sQueue == NULL) {
		return -1;
	}
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
	if (sQueue == NULL) {
		return -1;
	}
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
	if (sQueue == NULL) {
		return -1;
	}
	if (validSem(semToGet)) {
		semLock(&semToGet->lock);
		toReturn = semToGet->value;
		semUnlock(&semToGet->lock);
	}
	return toReturn;
}

int semGetUsersCount(Sem semToGet) {
	int toReturn = -1;
	if (sQueue == NULL) {
		return -1;
	}
	if (validSem(semToGet)) {
		semLock(&semToGet->lock);
		toReturn = semToGet->users;
		semUnlock(&semToGet->lock);
	}
	return toReturn;
}

int semGetBlockedProcessesCount(Sem semToGet) {
	int toReturn = -1;
	if (sQueue == NULL) {
		return -1;
	}
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
	if (semToValid != NULL && sQueue != NULL) {
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
	Sem a = *((Sem *)psem1);
	Sem b = *((Sem *)psem2);
	if (a < b) return -1;
	if (a > b) return 1;
	return 0;
}