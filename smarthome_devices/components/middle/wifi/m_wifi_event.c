#include "m_wifi_event.h"
#include "m_wifi_station.h"
#include "m_wifi_ap.h"
#include "m_ap_config.h"
#include "system_typedef.h"

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

int g_wifi_mode = 0;

void m_wifi_event_cb(input_event_t *event, void *private_data)
{
    if (g_wifi_mode == WIFI_STATION)
    {
        m_wifi_sta_event_cb(event, private_data);
    }
    else if (g_wifi_mode == WIFI_SOFT_AP)
    {
        m_wifi_ap_event_cb(event, private_data);
    }
}
