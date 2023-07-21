#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <blog.h>

#define DEBUG_ENABLE    0
void m_log(const char *fmt, ...);
#define os_trace if (DEBUG_ENABLE == 1) m_log

/* config for type of device, that will be used to parse json message */
#define EXTRAS              1514
#define FIRMWARE_VERSION    "230721"
#define HARDWARE_VERSION    "230303"

#endif /*_USER_CONFIG_H_*/