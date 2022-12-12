#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <blog.h>

#define DEBUG_ENABLE    1
#define os_trace if (DEBUG_ENABLE == 1) printf

#if (DEBUG_ENABLE == 1)
    #define USER_LOG_LEVEL	ESP_LOG_VERBOSE
#else
    #define USER_LOG_LEVEL	ESP_LOG_NONE
#endif

/* config for type of device, that will be used to parse json message */
#define APP_MAX_CHANNEL_SUPPORT 4
#define EXTRAS              1514
#define FIRMWARE_VERSION    "221007"
#define HARDWARE_VERSION    "210807"

#endif /*_USER_CONFIG_H_*/