#ifndef FREERTOS_QUEUE_MOCK_H
#define FREERTOS_QUEUE_MOCK_H

// Heap-allocated ring-buffer mock for FreeRTOS queues.
// Works for queues of pointer-sized items (sizeof(void*) == itemSize).

#include "FreeRTOS.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MOCK_QUEUE_CAPACITY 64

extern int mockQueueCreateCallCount;
extern int mockQueueCreateFailOnCall;

typedef struct {
    void *items[MOCK_QUEUE_CAPACITY];
    unsigned int head;
    unsigned int tail;
    unsigned int count;
} MockFrtosQueue_t;

static inline QueueHandle_t xQueueCreate(unsigned int length, unsigned int itemSize) {
    (void)length;
    (void)itemSize;
    mockQueueCreateCallCount++;
    if ((mockQueueCreateFailOnCall > 0) &&
        (mockQueueCreateCallCount == mockQueueCreateFailOnCall)) {
        return NULL;
    }
    MockFrtosQueue_t *q = (MockFrtosQueue_t *)calloc(1, sizeof(MockFrtosQueue_t));
    return (QueueHandle_t)q;
}

static inline BaseType_t xQueueSend(QueueHandle_t xQueue,
                                    const void *pvItemToQueue,
                                    unsigned int timeout) {
    (void)timeout;
    MockFrtosQueue_t *q = (MockFrtosQueue_t *)xQueue;
    if (q == NULL || q->count >= MOCK_QUEUE_CAPACITY) {
        return pdFALSE;
    }
    // Items are pointer-sized; copy the pointer stored at pvItemToQueue.
    void *item = NULL;
    memcpy(&item, pvItemToQueue, sizeof(void *));
    q->items[q->tail] = item;
    q->tail = (q->tail + 1) % MOCK_QUEUE_CAPACITY;
    q->count++;
    return pdPASS;
}

static inline BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, unsigned int timeout) {
    (void)timeout;
    MockFrtosQueue_t *q = (MockFrtosQueue_t *)xQueue;
    if (q == NULL || q->count == 0) {
        return pdFALSE;
    }
    memcpy(pvBuffer, &q->items[q->head], sizeof(void *));
    q->head = (q->head + 1) % MOCK_QUEUE_CAPACITY;
    q->count--;
    return pdTRUE;
}

static inline UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue) {
    MockFrtosQueue_t *q = (MockFrtosQueue_t *)xQueue;
    if (q == NULL) {
        return 0u;
    }
    return (UBaseType_t)q->count;
}

#endif  // FREERTOS_QUEUE_MOCK_H
