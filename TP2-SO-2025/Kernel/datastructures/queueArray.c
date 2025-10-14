#include "queueArray.h"
#include "memory_manager.h"

#define UNINITIALIZED_QUEUE_ARRAY_ITERATOR -1

typedef struct QueueArrayCDT {
	QueueADT *queues;
	int queueCount;
	int currentQueueIndex;
	int remainingQueueWeight;
	int elemCount;
	int (*getQueueWeight)(int queueIndex, QueueADT queue);
} QueueArrayCDT;

QueueArrayADT createQueueArray(QueueElemCmpFn cmp, size_t elemSize, int queueCount,
							   int (*getQueueWeight)(int queueIndex, QueueADT queue)) {
	if (queueCount <= 0 || elemSize <= 0) {
		return NULL;
	}

	QueueArrayADT queueArray = (QueueArrayADT)mm_alloc(sizeof(QueueArrayCDT));
	if (queueArray == NULL) {
		return NULL;
	}

	queueArray->queues = (QueueADT *)mm_alloc(queueCount * sizeof(QueueADT));
	if (queueArray->queues == NULL) {
		mm_free(queueArray);
		return NULL;
	}

	for (int i = 0; i < queueCount; i++) {
		queueArray->queues[i] = createQueue(cmp, elemSize);
		if (queueArray->queues[i] == NULL) {
			for (int j = 0; j < i; j++) {
				queueFree(queueArray->queues[j]);
			}
			mm_free(queueArray->queues);
			mm_free(queueArray);
			return NULL;
		}
	}

	queueArray->queueCount = queueCount;
	queueArray->currentQueueIndex = UNINITIALIZED_QUEUE_ARRAY_ITERATOR;
	queueArray->elemCount = 0;
	queueArray->remainingQueueWeight = 0;
	queueArray->getQueueWeight = getQueueWeight;
	return queueArray;
}

void freeQueueArray(QueueArrayADT queueArray) {
	if (queueArray == NULL) {
		return;
	}

	for (int i = 0; i < queueArray->queueCount; i++) {
		queueFree(queueArray->queues[i]);
	}

	mm_free(queueArray->queues);
	mm_free(queueArray);
}

QueueArrayADT queueArrayAddByIndex(QueueArrayADT queueArray, int queueIndex, void *data) {
	if (queueArray == NULL || data == NULL || queueIndex < 0 || queueIndex >= queueArray->queueCount) {
		return NULL;
	}

	QueueADT queue = queueArray->queues[queueIndex];
	if (queue == NULL) {
		return NULL;
	}

	enqueue(queue, data);

	if (queueArray->currentQueueIndex != UNINITIALIZED_QUEUE_ARRAY_ITERATOR && !queueIteratorIsInitialized(queue)) {
		queueBeginCyclicIter(queue);
	}

	queueArray->elemCount++;
	return queueArray;
}

QueueArrayADT queueArrayRemove(QueueArrayADT queueArray, int queueIndex, void *data) {
	if (queueArray == NULL || data == NULL || queueIndex < 0 || queueIndex >= queueArray->queueCount) {
		return NULL;
	}

	QueueADT queue = queueArray->queues[queueIndex];
	if (queue == NULL) {
		return NULL;
	}

	QueueADT result = queueRemove(queue, data);
	if (result != NULL) {
		queueArray->elemCount--;
	}

	return queueArray;
}

QueueArrayADT queueArrayBeginWeightedIter(QueueArrayADT queueArray) {
	if (queueArray == NULL) {
		return NULL;
	}

	queueArray->currentQueueIndex = 0;
	queueBeginCyclicIter(queueArray->queues[queueArray->currentQueueIndex]);
	queueArray->remainingQueueWeight =
		queueArray->getQueueWeight(queueArray->currentQueueIndex, queueArray->queues[queueArray->currentQueueIndex]);
	return queueArray;
}

QueueArrayADT queueArrayNextWeightedIter(QueueArrayADT queueArray, void *buffer) {
	if (queueArray == NULL || queueArray->currentQueueIndex == UNINITIALIZED_QUEUE_ARRAY_ITERATOR ||
		queueArray->getQueueWeight == NULL) {
		return NULL;
	}

	QueueADT queue = queueArray->queues[queueArray->currentQueueIndex];
	int keepSearching = 1;

	for (int iterCount = 0; iterCount < queueArray->queueCount + 1 && keepSearching; iterCount++) {
		if (queueArray->remainingQueueWeight <= 0 || (keepSearching = queueNextCyclicIter(queue, buffer) == NULL)) {
			queueArray->currentQueueIndex = (queueArray->currentQueueIndex + 1) % queueArray->queueCount;
			queue = queueArray->queues[queueArray->currentQueueIndex];
			if (queue != NULL) {
				queueArray->remainingQueueWeight = queueArray->getQueueWeight(queueArray->currentQueueIndex, queue);
				if (!queueIteratorIsInitialized(queue)) {
					queueBeginCyclicIter(queue);
				}
			};
		}
	}

	if (!keepSearching) {
		queueArray->remainingQueueWeight--;
		return buffer;
	}

	return NULL;
}

int queueArrayIsEmpty(QueueArrayADT queueArray) {
	if (queueArray == NULL) {
		return 1;
	}

	return queueArray->elemCount == 0;
}
