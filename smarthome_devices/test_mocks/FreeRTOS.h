#ifndef _MOCK_FREERTOS_H_
#define _MOCK_FREERTOS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef void* QueueHandle_t;
typedef void* StreamBufferHandle_t;
typedef void* StaticQueue_t;
typedef void* StaticStreamBuffer_t;
typedef void* StaticTask_t;
typedef uint32_t StackType_t;
typedef uint32_t TickType_t;
typedef int BaseType_t;
typedef void* SemaphoreHandle_t;
typedef void* StaticSemaphore_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portYIELD_FROM_ISR(x) ((void)(x))
#define portMAX_DELAY ((TickType_t)0xffffffffUL)
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)

#endif
