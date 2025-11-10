// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "array.h"

#include "memory_manager.h"
#define BLOCK_SIZE 1024

struct ArrayCDT {
	uint8_t *base;
	uint64_t sizeOfElem;
	uint16_t elemCount;
	uint16_t dim;
	uint16_t iter;
};

ArrayADT createArray(uint64_t sizeOfElem) {
	ArrayADT newArray = mm_alloc(sizeof(struct ArrayCDT));
	if (newArray == NULL) {
		return NULL;
	}

	newArray->base = mm_alloc(BLOCK_SIZE * sizeOfElem);
	if (newArray->base == NULL) {
		mm_free(newArray);
		return NULL;
	}
	newArray->sizeOfElem = sizeOfElem;
	newArray->elemCount = 0;
	newArray->dim = BLOCK_SIZE;

	return newArray;
}

ArrayADT arrayAdd(ArrayADT array, void *data) {
	if (array == NULL || array->base == NULL || data == NULL) {
		return NULL;
	}

	if (array->elemCount == array->dim) {
		uint8_t *newBase = mm_alloc((array->dim + BLOCK_SIZE) * array->sizeOfElem);
		if (newBase == NULL) {
			return NULL;
		}
		memcpy(newBase, array->base, array->dim * array->sizeOfElem);
		mm_free(array->base);
		array->base = newBase;
		array->dim += BLOCK_SIZE;
	}

	memcpy(array->base + (array->elemCount * array->sizeOfElem), data, array->sizeOfElem);

	array->elemCount++;

	return array;
}

void *getElemByIndex(ArrayADT array, uint16_t index, void *buffer) {
	if (array == NULL || array->base == NULL || index >= array->elemCount || buffer == NULL) {
		return NULL;
	}

	memcpy(buffer, array->base + (index * array->sizeOfElem), array->sizeOfElem);

	return buffer;
}

void *setElemByIndex(ArrayADT array, uint16_t index, void *data) {
	if (array == NULL || array->base == NULL || index >= array->elemCount || data == NULL) {
		return NULL;
	}

	memcpy(array->base + (index * array->sizeOfElem), data, array->sizeOfElem);

	return data;
}

uint16_t arraySize(ArrayADT array) {
	if (array == NULL) {
		return -1;
	}

	return array->elemCount;
}

void arrayFree(ArrayADT array) {
	if (array != NULL) {
		mm_free(array->base);
		mm_free(array);
	}
}

ArrayADT arrayBeginIter(ArrayADT array) {
	if (array == NULL) {
		return NULL;
	}

	array->iter = 0;
	return array;
}

int arrayHasNext(ArrayADT array) {
	if (array == NULL) {
		return 0;
	}

	return array->iter < array->elemCount;
}

void *arrayGetNext(ArrayADT array, void *buffer) {
	if (!arrayHasNext(array)) {
		return NULL;
	}

	if (NULL == getElemByIndex(array, array->iter, buffer)) {
		return NULL;
	}
	array->iter++;

	return buffer;
}

void arrayResetSize(ArrayADT array) {
	if (array != NULL) {
		array->elemCount = 0;
	}
}
