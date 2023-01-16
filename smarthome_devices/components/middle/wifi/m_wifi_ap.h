#ifndef _WIFI_AP_H_
#define _WIFI_AP_H_

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <FreeRTOS.h>
#include "portmacro.h"
#include <task.h>
#include <timers.h>
#include "event_groups.h"
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <easyflash.h>

#include <hal_sys.h>
#include <bl_wifi.h>
#include <hal_wifi.h>
#include <bl_timer.h>
#include <bl_sys_time.h>
#include <bl_sys.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include <wifi_mgmr_ext.h>
#include "FreeRTOS.h"
#include "portmacro.h"
#include "event_groups.h"
#include "lwip/err.h"
#include "string.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "web_server.h"

#define WIFI_RECONN_INTERVAL_SEC 15
#define WIFI_RECONN_REPEAT_TIMES 10

#define IP_SET_ADDR(a, b, c, d) (((uint32_t)((a)&0xff)) |       \
                                 ((uint32_t)((b)&0xff) << 8) |  \
                                 ((uint32_t)((c)&0xff) << 16) | \
                                 ((uint32_t)((d)&0xff) << 24))

void m_wifi_ap_start(void);
void m_wifi_ap_stop(void);
void m_wifi_ap_event_cb(input_event_t* event, void* private_data);

#endif 