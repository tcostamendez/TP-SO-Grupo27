// QueueADT

#include "queue.h"

struct Node {
    struct Node *next;
    uint8_t * data;
};

struct QueueCDT {
    struct Node *head;
    struct Node *tail;
    size_t dataSize;
    size_t elemCount;
};

QueueADT createQueue(size_t elemSize) {
    QueueADT queue = (QueueADT) malloc(sizeof(struct QueueCDT));
    if (queue == NULL) {
        return NULL;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->dataSize = elemSize;
    queue->elemCount = 0;
    return queue;
}

void enqueue(QueueADT queue, void * data) {
    struct Node * new = (struct Node *) malloc(sizeof(struct Node));
    if (new == NULL) {
        return ;
    }

    new->data = malloc(queue->dataSize);
    if (new->data == NULL) {
        free(new);
        return ;
    }

    memcpy(new->data, data, queue->dataSize);
    queue->tail->next = new;
    queue->tail = new;
    new->next = NULL;

    if (queue->head == NULL) {
        queue->head = new;
        queue->tail = new;
    }

    queue->elemCount++;
    return ;
}


void * dequeue(QueueADT queue, void * buffer) {
    if (queue->head == NULL) {
        return NULL;
    }

    struct Node * temp = queue->head;

    queue->head = queue->head->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }


    memcpy(buffer, temp->data, queue->dataSize);
    free(temp->data);
    free(temp);

    queue->elemCount--;
    return buffer;
}

void * peek(QueueADT queue, void * buffer) {
    if (queue->head == NULL) {
        return NULL;
    }
    memcpy(buffer, queue->head->data, queue->dataSize);
    return buffer;
}
