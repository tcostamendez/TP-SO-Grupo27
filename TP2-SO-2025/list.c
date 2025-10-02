#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

// ENDOF LIST_H

typedef int * elemType;

typedef struct Node {
    struct Node * next;
    elemType data;
} Node;

typedef struct {
    Node * head;
    size_t size;
    int (*cmp)(elemType, elemType);
    Node * iter;
} List;

typedef List * ListADT;

ListADT listCreate(int (*cmp)(elemType, elemType));
ListADT listAdd(ListADT list, elemType data);
ListADT listRemove(ListADT list, elemType data);
ListADT listDestroy(ListADT list);
ListADT listBeginIter(ListADT list);
int hasNext(ListADT list);
elemType listGetNext(ListADT list);

ListADT listCreate(int (*cmp)(elemType, elemType)) {
    ListADT list = (ListADT) malloc(sizeof(List));
    
    if (!list) {
        return NULL;
    }

    list->head = NULL;
    list->size = 0;
    list->cmp = cmp;
    return list;
}

ListADT listAdd(ListADT list, elemType data) {
    if (list == NULL || list->cmp == NULL) {
        return NULL;
    }

    Node * newNode = (Node *) malloc(sizeof(Node));
    
    if (!newNode) {
        return NULL;
    }

    newNode->data = data;
    Node * tmp = list->head;
    
    if (tmp == NULL) {
        list->head = newNode;
        newNode->next = NULL;
        list->size++;
        return list;
    }

    while (tmp->next != NULL) {
        if (list->cmp(tmp->data, data) < 0) {
            break;
        }
        tmp = tmp->next;
    }

    if (tmp->next == NULL) {
        tmp->next = newNode;
        newNode->next = NULL;
    } else {
        newNode->next = tmp->next;
        tmp->next = newNode;
    }
    
    list->size++;
    return list;
}

ListADT listRemove(ListADT list, elemType data) {
    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    Node * tmp = list->head;
    Node * prev = NULL;

    while (tmp != NULL) {
        if (list->cmp(tmp->data, data) == 0) {
            if (prev == NULL) {
                list->head = tmp->next;
            } else {
                prev->next = tmp->next;
            }
            free(tmp);
            list->size--;
            return list;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    return list;
}

ListADT listDestroy(ListADT list) {
    if (list == NULL) {
        return NULL;
    }
    Node * tmp = list->head;
    Node * next;

    while (tmp != NULL) {
        next = tmp->next;
        free(tmp);
        tmp = next;
    }
    
    free(list);
    return NULL;
}

ListADT listBeginIter(ListADT list) {
    if (list == NULL) {
        return NULL;
    }
    list->iter = list->head;
    return list;
}

int hasNext(ListADT list) {
    if (list->iter == NULL) {
        return 0;
    }
    return list->iter != NULL;
}

elemType listGetNext(ListADT list) {
    if (list->iter == NULL) {
        return NULL;
    }
    
    elemType data = list->iter->data;
    list->iter = list->iter->next;
    return data;
}

int cmp(int * a, int * b) {
    return *a - *b;
}

int main(void) {
    ListADT list = listCreate(cmp);
    int a = 1, b = 2, c = 3;
    listAdd(list, &a);
    listAdd(list, &c);
    listAdd(list, &b);

    listBeginIter(list);
    while (hasNext(list)) {
        printf("%d\n", *(int *)listGetNext(list));
    }

    listDestroy(list);
    return 0;
}

#endif