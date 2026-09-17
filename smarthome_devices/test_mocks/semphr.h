#ifndef _MOCK_SEMPHR_H_
#define _MOCK_SEMPHR_H_
#include "FreeRTOS.h"
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pxMutexBuffer);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);
#endif
