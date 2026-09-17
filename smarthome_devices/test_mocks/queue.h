#ifndef _MOCK_QUEUE_H_
#define _MOCK_QUEUE_H_
#include "FreeRTOS.h"
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueReset(QueueHandle_t xQueue);
QueueHandle_t xQueueCreateStatic(uint32_t uxQueueLength, uint32_t uxItemSize, uint8_t *pucQueueStorage, StaticQueue_t *pxQueueBuffer);
#endif
