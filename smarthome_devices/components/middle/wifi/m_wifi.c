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
#include "m_wf_ap_config.h"
#include "m_wf_ble_cfg.h"
#include "m_http_server.h"
#include "m_wf_scan.h"
#include "event_driven.h"

static wifi_conf_t conf = {
    .country_code = "VN",
};
m_wifi_t g_wifi_obj;
int g_wifi_mode = 0;

static void m_wifi_event_cb(input_event_t *event, void *private_data)
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

static void m_wifi_start_smartconfig()
{
    signal_post(SIG_SMART_CFG);
}

static void m_wifi_start_apconfig()
{
    signal_post(SIG_AP_CFG);
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
    m_wifi_t wf = m_wifi_get_instance();
    m_mqtt_get_instance()._mqtt_stop();
    vTaskDelay(pdMS_TO_TICKS(2000));
    wf.wifi_sta_stop_cb();
    vTaskDelay(pdMS_TO_TICKS(5000));
    wf.wifi_set_ssid_cb(ssid, strlen(ssid));
    wf.wifi_set_passwd_cb(passwd, strlen(passwd));
    wf.wifi_sta_start_cb(m_db_get_instance()->_m_device._wifi_info->_ssid, m_db_get_instance()->_m_device._wifi_info->_passwd);
}

static void m_wifi_save_ip(char *ip, size_t len)
{
    if (len <= sizeof(g_wifi_obj._wifi_info._ip))
    {
        memset(g_wifi_obj._wifi_info._ip, 0, sizeof(g_wifi_obj._wifi_info._ip));
        memcpy(g_wifi_obj._wifi_info._ip, ip, len);
    }
}

static void m_wifi_sync_rssi(void)
{
    wifi_mgmr_rssi_get(&(g_wifi_obj._wifi_info._rssi));
}

static void wifi_initialize(void)
{
   
    aos_register_event_filter(EV_WIFI, m_wifi_event_cb, NULL);
    hal_wifi_start_firmware_task();
    wifi_mgmr_start_background(&conf);
}

void m_wifi_init(void)
{
    memset(&g_wifi_obj, 0, sizeof(g_wifi_obj));

    g_wifi_obj.start_smart_config_cb = m_wifi_start_smartconfig;
    g_wifi_obj.start_ap_config_cb = m_wifi_start_apconfig;
    g_wifi_obj.wifi_set_ssid_cb = m_wifi_set_ssid;
    g_wifi_obj.wifi_set_passwd_cb = m_wifi_set_passwd;
    g_wifi_obj.wifi_change_wifi_cb = m_wifi_change_wifi;
    g_wifi_obj.wifi_save_ip = m_wifi_save_ip;
    g_wifi_obj.wifi_sync_rssi_cb = m_wifi_sync_rssi;

    g_wifi_obj.wifi_scan_cb = scan_ap_around;
    g_wifi_obj.wifi_ap_stop_cb = m_wifi_ap_stop;
    g_wifi_obj.wifi_ap_start_cb = m_wifi_ap_start;
    g_wifi_obj.wifi_sta_stop_cb = m_wifi_sta_stop;
    g_wifi_obj.wifi_sta_start_cb = m_wifi_sta_start;

    wifi_initialize();
}

m_wifi_t m_wifi_get_instance(void)
{
    return g_wifi_obj;
}

m_wifi_info_t* m_wifi_get_info_reference(void)
{
    return &g_wifi_obj._wifi_info;
}