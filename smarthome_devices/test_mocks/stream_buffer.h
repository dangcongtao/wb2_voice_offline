#ifndef _MOCK_STREAM_BUFFER_H_
#define _MOCK_STREAM_BUFFER_H_
#include "FreeRTOS.h"
StreamBufferHandle_t xStreamBufferCreateStatic(size_t xBufferSizeBytes, size_t xTriggerLevelBytes, uint8_t *pucStreamBufferStorageArea, StaticStreamBuffer_t *pxStaticStreamBuffer);
size_t xStreamBufferReceive(StreamBufferHandle_t xStreamBuffer, void *pvRxData, size_t xBufferLengthBytes, TickType_t xTicksToWait);
size_t xStreamBufferReset(StreamBufferHandle_t xStreamBuffer);
size_t xStreamBufferSendFromISR(StreamBufferHandle_t xStreamBuffer, const void *pvTxData, size_t xDataLengthBytes, BaseType_t *pxHigherPriorityTaskWoken);
#endif
