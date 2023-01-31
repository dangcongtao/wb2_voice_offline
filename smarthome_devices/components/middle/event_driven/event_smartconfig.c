#include "event_driven.h"

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"

void wifi_not_cfg_to_smart_cfg_handler(void *arg)
{
    os_trace("wifi_not_cfg_to_smart_cfg_handler\r\n");

    // enable ble
    ble_init();

    // stop wifi sta
    m_wifi_get_instance().wifi_sta_stop_cb();

    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();
}
m_state_transmit_t not_cfg_to_smart_cfg = {
    .context = M_WIFI_NOT_CFG,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = wifi_not_cfg_to_smart_cfg_handler
};

m_state_transmit_t wait_mqtt_connect_to_smartcfg = {
    .context = M_WIFI_WAIT_MQTT_CONNECTED,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = wifi_not_cfg_to_smart_cfg_handler
};

void smartconfig_done_handler(void *arg)
{
    os_trace("smartconfig_done_handler\r\n");
    
    vTaskDelay(pdMS_TO_TICKS(500));

    // stop ble
    ble_deinit();

    // stop wifi sta
    m_wifi_get_instance().wifi_sta_stop_cb();

    // wifi sta start
    char *ssid = m_wifi_get_instance()._wifi_info._ssid;
    char *pass = m_wifi_get_instance()._wifi_info._passwd;
    m_wifi_get_instance().wifi_sta_start_cb(ssid, pass);
}
m_state_transmit_t smartconfig_done = {
    .context = M_WIFI_SMARTCONFIG,
    .signal = SIG_SMART_CFG_DONE,
    .next_state = M_WIFI_GET_MQTT,
    .signal_handler_cb = smartconfig_done_handler
};