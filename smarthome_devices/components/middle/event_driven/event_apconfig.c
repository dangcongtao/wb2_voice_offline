#include "event_driven.h"

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"

extern uint32_t app_btn_on_hold_time_fire_event_ms;

// {M_WIFI_NOT_CFG, SIG_AP_CFG, M_WIFI_APCONFIG},
void wifi_not_cfg_to_ap_cfg_handler(void *arg)
{
    // stop wifi sta
    m_wifi_get_instance().wifi_sta_stop_cb();

    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();

    // start ap
    m_wifi_get_instance().wifi_ap_start_cb();

    // start http server
    m_http_server_start();

    app_btn_on_hold_time_fire_event_ms = APP_BTN_HOLD_EVENT_5S;
}
m_state_transmit_t not_cfg_to_ap_cfg = {
    .context = M_WIFI_NOT_CFG,
    .signal = SIG_AP_CFG,
    .next_state = M_WIFI_APCONFIG,
    .signal_handler_cb = wifi_not_cfg_to_ap_cfg_handler};

m_state_transmit_t mqtt_connecting_to_ap_config = {
    .context = M_WIFI_MQTT_CONNECTING,
    .signal = SIG_AP_CFG,
    .next_state = M_WIFI_APCONFIG,
    .signal_handler_cb = wifi_not_cfg_to_ap_cfg_handler
};

void ap_config_done_handler(void *arg)
{
    // stop http server
    m_http_stop();

    // stop wifi ap
    m_wifi_get_instance().wifi_ap_stop_cb();

    // set new led states
    m_led_stt_get_instance()._set_led_stt_wifi_not_connect_cb();

    // wifi sta start
    char *ssid = m_wifi_get_instance()._wifi_info._ssid;
    char *pass = m_wifi_get_instance()._wifi_info._passwd;
    m_wifi_get_instance().wifi_sta_start_cb(ssid, pass);
}
m_state_transmit_t ap_config_done = {
    .context = M_WIFI_APCONFIG,
    .signal = SIG_AP_CFG_DONE,
    .next_state = M_WIFI_CONNECTING,
    .signal_handler_cb = ap_config_done_handler
};

/* Smart Config -> AP-config */
void smartconfig_to_apconfig_handler(void *arg)
{
    os_trace("smartconfig_to_apconfig_handler\r\n");

    /* ble rename */
    ble_set_name("Dungnt98", 1);

    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();

    // start ap
    m_wifi_get_instance().wifi_ap_start_cb();

    // start http server
    m_http_server_start();
}
m_state_transmit_t smartconfig_to_apconfig = {
    .context = M_WIFI_SMARTCONFIG,
    .signal = SIG_AP_CFG,
    .next_state = M_WIFI_APCONFIG,
    .signal_handler_cb = smartconfig_to_apconfig_handler
};