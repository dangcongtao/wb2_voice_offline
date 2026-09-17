#ifndef _MOCK_TASK_H_
#define _MOCK_TASK_H_
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
TaskHandle_t xTaskCreateStatic(void (*pxTaskCode)(void *), const char * const pcName, const uint32_t ulStackDepth, void * const pvParameters, uint32_t uxPriority, StackType_t * const puxStackBuffer, StaticTask_t * const pxTaskBuffer);
void vTaskDelay(TickType_t xTicksToDelay);
TickType_t xTaskGetTickCount(void);
#endif
