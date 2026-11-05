#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <blog.h>

#define DEBUG_ENABLE 1
#define AIIO_WDT_TIMEOUT_MS 60000U

void m_log(const char *fmt, ...);
#define os_trace           \
    if (DEBUG_ENABLE == 1) \
    printf

/* config for type of device, that will be used to parse json message */
#define EXTRAS 1514
#define FIRMWARE_VERSION "261105"
#define FIRMWARE_VERSION_NUMBER 240702

#ifndef BUILD_TEST
#define BUILD_TEST 0 // 1 dev, 0 release
#endif

#define HARDWARE_VERSION "230303"

#endif /*_USER_CONFIG_H_*/