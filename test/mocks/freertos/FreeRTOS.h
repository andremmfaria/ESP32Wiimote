#ifndef FREERTOS_MOCK_H
#define FREERTOS_MOCK_H

// Minimal FreeRTOS type mock for native unit tests.
#include <stdint.h>

typedef void *QueueHandle_t;
typedef void *xQueueHandle;  // older FreeRTOS 8 alias

typedef int BaseType_t;
typedef unsigned int UBaseType_t;

#define pdTRUE ((BaseType_t)1)
#define pdFALSE ((BaseType_t)0)
#define pdPASS pdTRUE

#define portMAX_DELAY ((unsigned int)0xFFFFFFFFu)

#endif  // FREERTOS_MOCK_H
