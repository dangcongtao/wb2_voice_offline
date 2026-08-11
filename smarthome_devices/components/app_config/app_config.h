#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <blog.h>

#define DEBUG_ENABLE 1

#define AIIO_UART0_RX_LOG_ENABLE 1
#define AIIO_UART0_RX_PIN 7U
#define AIIO_UART0_RX_BAUD_RATE 115200U
#define AIIO_UART0_RX_STREAM_SIZE 512U
#define AIIO_UART0_RX_IDLE_TIMEOUT_MS 10U

void m_log(const char *fmt, ...);
#define os_trace           \
    if (DEBUG_ENABLE == 1) \
    printf

/* config for type of device, that will be used to parse json message */
#define EXTRAS 1514
#define FIRMWARE_VERSION "260210"
#define FIRMWARE_VERSION_NUMBER 240702

#define BUILD_TEST 0 // 1 dev, 0 release

#define HARDWARE_VERSION "230303"

#endif /*_USER_CONFIG_H_*/