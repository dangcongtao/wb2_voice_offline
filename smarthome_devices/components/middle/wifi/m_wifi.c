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

#define ESP_MAXIMUM_RETRY 5

static wifi_conf_t conf = {
    .country_code = "VN",
};

static m_wifi_t s_wifi_obj;

static void m_wifi_set_wifi_state(m_wifi_state_t newstate)
{
    s_wifi_obj.wifi_state = newstate;
}

m_wifi_state_t m_wifi_get_wifi_state(void)
{
    return s_wifi_obj.wifi_state;
}

static void m_wifi_start_smartconfig()
{
    os_trace("m_wifi_start_smartconfig \n");
    s_wifi_obj.wifi_state = WIFI_SMARTCONFIG;
}

static void m_wifi_start_apconfig()
{
    s_wifi_obj.wifi_state = WIFI_APCONFIG;
}

static void m_wifi_set_ssid(char *ssid, size_t len)
{
    if (len < sizeof(s_wifi_obj._wifi_info._ssid))
    {
        memset(s_wifi_obj._wifi_info._ssid, 0, sizeof(s_wifi_obj._wifi_info._ssid));
        memcpy(s_wifi_obj._wifi_info._ssid, ssid, len);
    }
}

static void m_wifi_set_passwd(char *passwd, size_t len)
{
    if (len <= sizeof(s_wifi_obj._wifi_info._passwd))
    {
        memset(s_wifi_obj._wifi_info._passwd, 0, sizeof(s_wifi_obj._wifi_info._passwd));
        memcpy(s_wifi_obj._wifi_info._passwd, passwd, len);
    }
}

static void m_wifi_change_wifi(char *ssid, char *passwd)
{
    m_wifi_get_instance().wifi_set_ssid_cb(ssid, strlen(ssid));
    m_wifi_get_instance().wifi_set_passwd_cb(passwd, strlen(passwd));
    m_wifi_get_instance().wifi_set_state_cb(WIFI_DISCONNECT);
}

static void m_wifi_save_ip(char *ip, size_t len)
{
    if (len <= sizeof(s_wifi_obj._wifi_info._ip))
    {
        memset(s_wifi_obj._wifi_info._ip, 0, sizeof(s_wifi_obj._wifi_info._ip));
        memcpy(s_wifi_obj._wifi_info._ip, ip, len);
    }
}

static void m_wifi_set_evt_source(m_wifi_event_source_t new_event)
{
    s_wifi_obj.event_source = new_event;
}

static void wifi_initialize()
{
    puts("[OS] Starting TCP/IP Stack...");
    tcpip_init(NULL, NULL);
    aos_register_event_filter(EV_WIFI, m_wifi_event_cb, NULL);
    hal_wifi_start_firmware_task();
    wifi_mgmr_start_background(&conf);
}

void m_wifi_connect(char *ssid, char *password)
{
}

void m_wifi_retry_connect(void)
{
    static uint32_t time_reconnect;

    time_reconnect += 50;
    if (time_reconnect < 5000)
    {
        return;
    }
    time_reconnect = 0;
}

static void m_wifi_sync_rssi(void)
{
}

void wifi_task(void *param)
{
    /* load database */
    while (true)
    {
        switch (s_wifi_obj.wifi_state)
        {
        case WIFI_START:
        {
            /* if device was configured */
            if (m_db_get_instance()->_get_dev_pair_state())
            {
                m_wifi_connect(s_wifi_obj._wifi_info._ssid, s_wifi_obj._wifi_info._passwd);
                s_wifi_obj.wifi_state = WIFI_WAIT_EVENT;
            } 
            else {
                /* wait user config */
                s_wifi_obj.wifi_state = WIFI_APCONFIG;
            }
        }
        break;

        case WIFI_SMARTCONFIG:
        {
            os_trace("m_wifi: Wifi Smartconfig\n");
            m_led_stt_get_instance()._set_led_stt_smart_config_cb();
            s_wifi_obj.wifi_state = WIFI_WAIT_SMARTCONFIG_DONE;
        }
        break;

        case WIFI_WAIT_SMARTCONFIG_DONE:
        {
            // if (start_smartconfig() == SMARTCONFIG_DONE)
            // {
            //     os_trace("\nsmart config done\n");
            //     s_wifi_obj.wifi_state = WIFI_CONFIG_DONE;
            // }
        }
        break;

        case WIFI_APCONFIG:
        {
            m_wifi_ap_start();
            // m_wifi_sta_start();
            // reset_ap_config();
            s_wifi_obj.wifi_state = WIFI_WAIT_APCONFIG_DONE;
        }
        break;

        case WIFI_WAIT_APCONFIG_DONE:
        {
            if (is_ap_config_done())
                s_wifi_obj.wifi_state = WIFI_APCONFIG_DONE;
        }
        break;

        case WIFI_APCONFIG_DONE:
        {
            os_trace("Ap config done\n");
            s_wifi_obj.wifi_state = WIFI_CONFIG_DONE;
        }
        break;

        case WIFI_CONFIG_DONE:
        {
            os_trace("m_wifi: Wifi config done\n");
            vTaskDelay(pdMS_TO_TICKS(2000));
            bl_sys_reset_por();
            // s_wifi_obj.wifi_state = WIFI_DEV_ACTIVE;
        }
        break;

        case WIFI_DISCONNECT:
        {
            m_led_stt_get_instance()._set_led_stt_not_config_cb();
            m_wifi_retry_connect();
        }
        break;

        case WIFI_CONNECTED:
        {
            os_trace("m_wifi: Wifi connected\n");
            /* mqtt start */
            m_mqtt_get_instance()._mqtt_stop();
            m_mqtt_get_instance()._mqtt_start();
            s_wifi_obj.wifi_state = WIFI_DEV_ACTIVE;
        }
        break;

        case WIFI_DEV_ACTIVE:
        {
            m_wifi_sync_rssi();
            m_mqtt_get_instance()._mqtt_loop();
        }
        break;

        case WIFI_WAIT_EVENT:
        {
        }
        break;

        default:
            break;
        }

        static uint32_t old = 0;
        old += 100;
        if (old > 2000)
        {
            // ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
            old = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); /* should be 500ms to make it's priority low */
    }
}

void m_wifi_init(void)
{
    memset(&s_wifi_obj, 0, sizeof(s_wifi_obj));

    s_wifi_obj.wifi_state = WIFI_START;
    s_wifi_obj.max_retry_count = ESP_MAXIMUM_RETRY;
    s_wifi_obj.retry_count = 0;
    s_wifi_obj.event_source = WIFI_NORM_EVENT;

    s_wifi_obj.start_smart_config_cb = m_wifi_start_smartconfig;
    s_wifi_obj.start_ap_config_cb = m_wifi_start_apconfig;
    s_wifi_obj.wifi_set_state_cb = m_wifi_set_wifi_state;
    s_wifi_obj.wifi_get_state_cb = m_wifi_get_wifi_state;
    s_wifi_obj.wifi_set_ssid_cb = m_wifi_set_ssid;
    s_wifi_obj.wifi_set_passwd_cb = m_wifi_set_passwd;
    s_wifi_obj.wifi_change_wifi_cb = m_wifi_change_wifi;
    s_wifi_obj.wifi_save_ip = m_wifi_save_ip;
    s_wifi_obj.wifi_set_source_event_cb = m_wifi_set_evt_source;

    wifi_initialize();
    wifi_task(NULL);
}

m_wifi_t m_wifi_get_instance(void)
{
    return s_wifi_obj;
}

m_wifi_info_t* m_wifi_get_info_reference(void)
{
    return &s_wifi_obj._wifi_info;
}