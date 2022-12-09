#include "m_wifi.h"

// #include "esp_log.h"
// #include "esp_system.h"
// #include "nvs_flash.h"

// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_smartconfig.h"
// #include "smartconfig_ack.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "freertos/timers.h"
// #include "freertos/event_groups.h"

// #include <string.h>

// #include "m_smartconfig.h"
// #include "m_ap_config.h"
// #include "m_database.h"
// #include "m_led_stt.h"
// #include "m_mqtt.h"
// #include "m_http_server.h"
// #include "app_config.h"
// #include "m_ota.h"

// #define ESP_MAXIMUM_RETRY       5

// #define TAG                 "m_wifi"

// static m_wifi_t s_wifi_obj;

// static void m_wifi_set_wifi_state(m_wifi_state_t newstate)
// {
//     s_wifi_obj.wifi_state = newstate;
// }

// m_wifi_state_t m_wifi_get_wifi_state(void)
// {
//     return s_wifi_obj.wifi_state;
// }

// static void m_wifi_start_smartconfig()
// {
//     os_trace("m_wifi_start_smartconfig \n");
//     s_wifi_obj.wifi_state = WIFI_SMARTCONFIG;
// }

// static void m_wifi_start_apconfig()
// {
//     s_wifi_obj.wifi_state = WIFI_APCONFIG;
// }

// static void m_wifi_set_ssid(char* ssid, size_t len)
// {
//     if (len <= sizeof(s_wifi_obj.wifi_info->_ssid)) {
//         memset(s_wifi_obj.wifi_info->_ssid, 0, sizeof(s_wifi_obj.wifi_info->_ssid));
//         memcpy(s_wifi_obj.wifi_info->_ssid, ssid, len);
//     }
// }

// static void m_wifi_set_passwd(char* passwd, size_t len)
// {
//     if (len <= sizeof(s_wifi_obj.wifi_info->_passwd)) {
//         memset(s_wifi_obj.wifi_info->_passwd, 0, sizeof(s_wifi_obj.wifi_info->_passwd));
//         memcpy(s_wifi_obj.wifi_info->_passwd, passwd, len);
//     }
// }

// static void m_wifi_change_wifi(char* ssid, char* passwd)
// {
//     m_wifi_get_instance().wifi_set_ssid_cb(ssid, strlen(ssid));
//     m_wifi_get_instance().wifi_set_passwd_cb(passwd, strlen(passwd));
//     m_wifi_get_instance().wifi_set_state_cb(WIFI_DISCONNECT);
// }

// static void m_wifi_save_ip(char* ip, size_t len)
// {
//     if (len <= sizeof(s_wifi_obj.wifi_info->_ip)) {
//         memset(s_wifi_obj.wifi_info->_ip, 0, sizeof(s_wifi_obj.wifi_info->_ip));
//         memcpy(s_wifi_obj.wifi_info->_ip, ip, len);
//     }
// }

// static void m_wifi_set_evt_source(m_wifi_event_source_t new_event)
// {
//     s_wifi_obj.event_source = new_event;
// }

// static void wifi_initialize()
// {
// }

// static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
// {

// }

// void m_wifi_connect(char* ssid, char* password)
// {

// }

// void m_wifi_retry_connect(void)
// {
//     static uint32_t time_reconnect;

//     time_reconnect += 50;
//     if (time_reconnect < 5000) {
//         return;
//     }
//     time_reconnect = 0;

// }

// static void m_wifi_sync_rssi(void)
// {
//     wifi_ap_record_t wifidata;
//     static uint32_t time_check_rssi = 0;

//     time_check_rssi += 100;
//     if (time_check_rssi > 1000) {
//         time_check_rssi = 0;
//     }
// }

// void wifi_task(void *param)
// {
//     /* load database */
//     while (true)
//     {
//         vTaskDelay(pdMS_TO_TICKS(100)); /* should be 500ms to make it's priority low */
//     }
// }

// void m_wifi_init(void)
// {
//     memset(&s_wifi_obj, 0, sizeof(s_wifi_obj));

//     s_wifi_obj.wifi_state = WIFI_START;
//     s_wifi_obj.max_retry_count = ESP_MAXIMUM_RETRY;
//     s_wifi_obj.retry_count = 0;
//     s_wifi_obj.event_source = WIFI_NORM_EVENT;

//     s_wifi_obj.wifi_info = m_db_get_wifi_reference();
//     s_wifi_obj.start_smart_config_cb = m_wifi_start_smartconfig;
//     s_wifi_obj.start_ap_config_cb = m_wifi_start_apconfig;
//     s_wifi_obj.wifi_set_state_cb = m_wifi_set_wifi_state;
//     s_wifi_obj.wifi_get_state_cb = m_wifi_get_wifi_state;
//     s_wifi_obj.wifi_set_ssid_cb = m_wifi_set_ssid;
//     s_wifi_obj.wifi_set_passwd_cb = m_wifi_set_passwd;
//     s_wifi_obj.wifi_change_wifi_cb = m_wifi_change_wifi;
//     s_wifi_obj.wifi_save_ip = m_wifi_save_ip;
//     s_wifi_obj.wifi_set_source_event_cb = m_wifi_set_evt_source;

//     wifi_initialize();
//     wifi_task(NULL);
// }

// m_wifi_t m_wifi_get_instance(void)
// {
//     return s_wifi_obj;
// }