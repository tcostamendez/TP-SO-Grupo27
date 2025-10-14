#include "list.h"
#include "memory_manager.h"

typedef struct Node {
	struct Node *next;
	uint8_t *data;
} Node;

typedef struct List {
	Node *head;
	Node *iter;
	size_t size;
	int (*cmp)(void *, void *);
	uint64_t sizeOfElem;
} List;

static inline void copyElementToBuffer(uint8_t *buffer, uint8_t *data, size_t size) {
	// print("copyElementToBuffer: "); printHex((uint64_t) buffer); print(" "); printHex((uint64_t) data); print(" "); printDec(size); print(" - elem: "); printDec(*(int*)data); print("\n");
	if (buffer == NULL || data == NULL) {
		return;
	}
	memcpy(buffer, data, size);
}

ListADT listCreate(int (*cmp)(void *, void *), uint64_t sizeOfElem) {
	ListADT list = (ListADT)mm_alloc(sizeof(List));

	if (!list) {
		return NULL;
	}

	list->head = NULL;
	list->size = 0;
	list->cmp = cmp;
	list->sizeOfElem = sizeOfElem;
	return list;
}

ListADT listAdd(ListADT list, void *data) {
	Node *prev;
	Node *curr = list->head;

	while (curr != NULL && list->cmp(curr->data, data) < 0) {
		prev = curr;
		curr = curr->next;
	}

	if (curr != NULL && list->cmp(curr->data, data) == 0) {
		// Element already exists, do not add it again
		return list;
	}

	Node *aux = mm_alloc(sizeof(struct Node));

	if (aux == NULL) {
		return NULL;
	}

	aux->data = mm_alloc(list->sizeOfElem);

	if (aux->data == NULL) {
		mm_free(aux);
		return NULL;
	}

	copyElementToBuffer(aux->data, data, list->sizeOfElem);
	aux->next = curr;

	if (curr == list->head) {
		list->head = aux;
	} else {
		prev->next = aux;
	}

	list->size++;
	return list;
}

ListADT listRemove(ListADT list, void *data) {
	if (list == NULL || list->head == NULL) {
		return NULL;
	}

	Node *tmp = list->head;
	Node *prev = NULL;

	while (tmp != NULL) {
		if (list->cmp(tmp->data, data) == 0) {
			if (prev == NULL) {
				list->head = tmp->next;
			} else {
				prev->next = tmp->next;
			}
			mm_free(tmp->data);
			mm_free(tmp);
			list->size--;
			return list;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	return list;
}

void listFree(ListADT list) {
	if (list == NULL) {
		return;
	}

	Node *tmp = list->head;
	Node *next;

	while (tmp != NULL) {
		next = tmp->next;
		mm_free(tmp->data);
		mm_free(tmp);
		tmp = next;
	}

	mm_free(list);
	return;
}

ListADT listBeginIter(ListADT list) {
	if (list == NULL) {
		return NULL;
	}
	list->iter = list->head;
	return list;
}

int listHasNext(ListADT list) {
	if (list == NULL) {
		return 0;
	}

	return list->iter != NULL;
}

void *listGetNext(ListADT list, void *buffer) {
	if (list->iter == NULL) {
		return NULL;
	}

	copyElementToBuffer(buffer, list->iter->data, list->sizeOfElem);

	list->iter = list->iter->next;
	return buffer;
}

uint64_t listGetSize(ListADT list) { return list->size; }
