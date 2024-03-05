#include "event_driven.h"

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"

extern char s_device_id[64];
extern uint32_t app_btn_on_hold_time_fire_event_ms;

void wifi_not_cfg_to_smart_cfg_handler(void *arg)
{
    os_trace("wifi_not_cfg_to_smart_cfg_handler\r\n");
    clear_timeout_reset();
    m_led_stt_get_instance()._set_led_stt_smart_config_cb();

    m_mqtt_get_instance()._mqtt_stop();

    // stop wifi sta
    m_wifi_get_instance().wifi_sta_stop_cb();

    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();

    // enable ble
    // ble_deinit();
    ble_set_name(s_device_id, 1);
    // ble_init();

    app_btn_on_hold_time_fire_event_ms = APP_BTN_HOLD_EVENT_5S;
}
m_state_transmit_t not_cfg_to_smart_cfg = {
    .context = M_WIFI_NOT_CFG,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = wifi_not_cfg_to_smart_cfg_handler
};

m_state_transmit_t mqtt_connecting_to_smartcfg = {
    .context = M_WIFI_MQTT_CONNECTING,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = wifi_not_cfg_to_smart_cfg_handler
};

m_state_transmit_t get_mqtt_info_to_smartcfg = {
    .context = M_WIFI_CONN_MQTT,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = wifi_not_cfg_to_smart_cfg_handler
};

void smartconfig_done_handler(void *arg)
{
    os_trace("smartconfig_done_handler\r\n");
    
    vTaskDelay(pdMS_TO_TICKS(500));

    /* ble rename */
    ble_set_name("Dungnt98", 1);

    // set new led states
    m_led_stt_get_instance()._set_led_stt_wifi_not_connect_cb();

    // stop wifi sta
    m_wifi_get_instance().wifi_sta_stop_cb();

    // wifi sta start
    char *ssid = m_wifi_get_instance()._wifi_info._ssid;
    char *pass = m_wifi_get_instance()._wifi_info._passwd;
    if (m_db_get_instance()->_get_dev_pair_state()) {
        m_wifi_get_instance().wifi_sta_start_cb(ssid, pass);
    }
}
m_state_transmit_t smartconfig_done = {
    .context = M_WIFI_SMARTCONFIG,
    .signal = SIG_SMART_CFG_DONE,
    .next_state = M_WIFI_CONNECTING,
    .signal_handler_cb = smartconfig_done_handler
};

/* AP-config -> Smart Config */
extern char s_device_id[64];
void apconfig_to_smartconfig_handler(void *arg)
{
    os_trace("apconfig_to_smartconfig_handler\r\n");
    m_led_stt_get_instance()._set_led_stt_smart_config_cb();

    /* ble rename */
    ble_set_name(s_device_id, 1);

    // stop http server
    m_http_stop();

    // stop wifi ap
    m_wifi_get_instance().wifi_ap_stop_cb();
}
m_state_transmit_t apconfig_to_smartconfig = {
    .context = M_WIFI_APCONFIG,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = apconfig_to_smartconfig_handler
};