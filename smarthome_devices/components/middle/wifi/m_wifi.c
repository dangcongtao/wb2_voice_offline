#include "m_wifi.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <event_groups.h>

#include <string.h>
#include <stdio.h>
#include <string.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/tcpip.h>
#include <wifi_mgmr_ext.h>
#include <hal_wifi.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <blog.h>
#include "bl_sys.h"

#include "app_config.h"
#include "m_database.h"
#include "m_led_stt.h"
#include "m_mqtt.h"
#include "m_ota.h"

#include "m_wifi_station.h"
#include "m_wifi_ap.h"
#include "m_ap_config.h"
#include "m_smartconfig.h"
#include "m_http_server.h"
#include "m_wifi_event.h"

extern void app_handler_msg_loop(void);

m_wifi_t g_wifi_obj;

static wifi_conf_t conf = {
    .country_code = "VN",
};

static void m_wifi_set_wifi_state(m_wifi_state_t newstate)
{
    g_wifi_obj.wifi_state = newstate;
}

m_wifi_state_t m_wifi_get_wifi_state(void)
{
    return g_wifi_obj.wifi_state;
}

static void m_wifi_start_smartconfig()
{
    g_wifi_obj.wifi_state = M_WIFI_SMARTCONFIG;
}

static void m_wifi_start_apconfig()
{
    g_wifi_obj.wifi_state = M_WIFI_APCONFIG;
}

static void m_wifi_set_ssid(char *ssid, size_t len)
{
    if (len < sizeof(g_wifi_obj._wifi_info._ssid))
    {
        memset(g_wifi_obj._wifi_info._ssid, 0, sizeof(g_wifi_obj._wifi_info._ssid));
        memcpy(g_wifi_obj._wifi_info._ssid, ssid, len);
    }
}

static void m_wifi_set_passwd(char *passwd, size_t len)
{
    if (len <= sizeof(g_wifi_obj._wifi_info._passwd))
    {
        memset(g_wifi_obj._wifi_info._passwd, 0, sizeof(g_wifi_obj._wifi_info._passwd));
        memcpy(g_wifi_obj._wifi_info._passwd, passwd, len);
    }
}

static void m_wifi_change_wifi(char *ssid, char *passwd)
{
    m_wifi_get_instance().wifi_set_ssid_cb(ssid, strlen(ssid));
    m_wifi_get_instance().wifi_set_passwd_cb(passwd, strlen(passwd));
    m_wifi_get_instance().wifi_set_state_cb(M_WIFI_DISCONNECT);
}

static void m_wifi_save_ip(char *ip, size_t len)
{
    if (len <= sizeof(g_wifi_obj._wifi_info._ip))
    {
        memset(g_wifi_obj._wifi_info._ip, 0, sizeof(g_wifi_obj._wifi_info._ip));
        memcpy(g_wifi_obj._wifi_info._ip, ip, len);
    }
}

static void wifi_initialize()
{
    aos_register_event_filter(EV_WIFI, m_wifi_event_cb, NULL);
    hal_wifi_start_firmware_task();
    wifi_mgmr_start_background(&conf);
}

static void m_wifi_sync_rssi(void)
{
    wifi_mgmr_rssi_get(&(g_wifi_obj._wifi_info._rssi));
}

void wifi_task(void *param)
{
    /* load database */
    while (true)
    {
        switch (g_wifi_obj.wifi_state)
        {
        case M_WIFI_START:
        {
            /* if device was configured */
            if (m_db_get_instance()->_get_dev_pair_state())
            {   
                m_wifi_sta_start(g_wifi_obj._wifi_info._ssid, g_wifi_obj._wifi_info._passwd);
                g_wifi_obj.wifi_state = M_WIFI_WAIT_EVENT;
            } 
            else {
                /* wait user config */
                g_wifi_obj.wifi_state = M_WIFI_SMARTCONFIG;
            }
        }
        break;

        case M_WIFI_SMARTCONFIG:
        {
            os_trace("m_wifi: Wifi Smartconfig\n");
            m_led_stt_get_instance()._set_led_stt_smart_config_cb();
            scan_ap_around();
            g_wifi_obj.wifi_state = M_WIFI_WAIT_SMARTCONFIG_DONE;
        }
        break;

        case M_WIFI_WAIT_SMARTCONFIG_DONE:
        {
            if (start_smartconfig() == SMARTCONFIG_DONE)
            {
                g_wifi_obj.wifi_state = M_WIFI_CONFIG_DONE;
            }
        }
        break;

        case M_WIFI_APCONFIG:
        {
            m_wifi_sta_stop();
            m_wifi_ap_start();
            g_wifi_obj.wifi_state = M_WIFI_WAIT_APCONFIG_DONE;
        }
        break;

        case M_WIFI_WAIT_APCONFIG_DONE:
        {
            if (is_ap_config_done())
                g_wifi_obj.wifi_state = M_WIFI_APCONFIG_DONE;
        }
        break;

        case M_WIFI_APCONFIG_DONE:
        {
            os_trace("Ap config done\n");
            g_wifi_obj.wifi_state = M_WIFI_CONFIG_DONE;
        }
        break;

        case M_WIFI_CONFIG_DONE:
        {
            os_trace("m_wifi: Wifi config done\n");
            vTaskDelay(pdMS_TO_TICKS(2000));
            bl_sys_reset_por();
        }
        break;

        case M_WIFI_DISCONNECT:
        {
            m_led_stt_get_instance()._set_led_stt_not_config_cb();
            g_wifi_obj.wifi_state = M_WIFI_WAIT_EVENT;
        }
        break;

        case M_WIFI_CONNECTED:
        {
            os_trace("m_wifi: Wifi connected\n");
            g_wifi_obj.wifi_state = M_WIFI_GET_MQTT;
        }
        break;

        case M_WIFI_GET_MQTT:
        {
            /* check mqtt info */
            if (strlen(m_mqtt_get_instance().mqtt_info.server)  == 0    || 
                strlen(m_mqtt_get_instance().mqtt_info.port) == 0       || 
                strlen(m_mqtt_get_instance().mqtt_info.username) == 0   || 
                strlen(m_mqtt_get_instance().mqtt_info.username) == 0)
            {
                /* get mqtt info if it was not existed */

            }
            /* jump to connect mqtt */
            g_wifi_obj.wifi_state = M_WIFI_CONN_MQTT;
        }
        break;

        case M_WIFI_CONN_MQTT: 
        {
            os_trace("m_wifi: Wifi connected\n");
            /* mqtt start */
            m_mqtt_get_instance()._mqtt_stop();
            m_mqtt_get_instance()._mqtt_start();
            g_wifi_obj.wifi_state = M_WIFI_DEV_ACTIVE;
        }

        case M_WIFI_DEV_ACTIVE:
        {
            m_wifi_sync_rssi();
            m_mqtt_get_instance()._mqtt_loop();
        }
        break;

        case M_WIFI_WAIT_EVENT:
        {
        }
        break;

        default:
            break;
        }

        static uint32_t old = 0;
        old += 100;
        if (old > 2000) {
            os_trace("[APP] Free memory: %d bytes\r\n", xPortGetFreeHeapSize());
            old = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); /* should be 500ms to make it's priority low */
    }
}

void m_wifi_init(void)
{
    memset(&g_wifi_obj, 0, sizeof(g_wifi_obj));

    g_wifi_obj.wifi_state = M_WIFI_START;

    g_wifi_obj.start_smart_config_cb = m_wifi_start_smartconfig;
    g_wifi_obj.start_ap_config_cb = m_wifi_start_apconfig;
    g_wifi_obj.wifi_set_state_cb = m_wifi_set_wifi_state;
    g_wifi_obj.wifi_get_state_cb = m_wifi_get_wifi_state;
    g_wifi_obj.wifi_set_ssid_cb = m_wifi_set_ssid;
    g_wifi_obj.wifi_set_passwd_cb = m_wifi_set_passwd;
    g_wifi_obj.wifi_change_wifi_cb = m_wifi_change_wifi;
    g_wifi_obj.wifi_save_ip = m_wifi_save_ip;
}

void m_wifi_start(void)
{
    wifi_initialize();
    wifi_task(NULL);
}

m_wifi_t m_wifi_get_instance(void)
{
    return g_wifi_obj;
}

m_wifi_info_t* m_wifi_get_info_reference(void)
{
    return &g_wifi_obj._wifi_info;
}