#include "event_driven.h"

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"
#include "IRrecv.h"

// extern uint32_t app_btn_on_hold_time_fire_event_ms;
extern uint8_t use_server_prev;
extern void ble_paired_need_connect(void);

// {M_WIFI_NOT_CFG, SIG_AP_CFG, M_WIFI_APCONFIG},
void wifi_not_cfg_to_ap_cfg_handler(void *arg)
{
    m_wifi_t wf = m_wifi_get_instance();
    clear_timeout_reset();
    m_led_stt_get_instance()._set_led_stt_ap_config_cb();
    m_ir_deinit();

    /* ble rename */
    ble_set_name("Dungnt98", 1);

    // stop mqtt
    m_mqtt_get_instance()._mqtt_stop();

    // stop wifi sta
    wf.wifi_sta_stop_cb();

    // scan wifi
    wf.wifi_scan_cb();

    // start ap
    wf.wifi_ap_start_cb();

    // start http server
    m_http_server_start();

    // app_btn_on_hold_time_fire_event_ms = APP_BTN_HOLD_EVENT_5S;
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

m_state_transmit_t get_mqtt_info_to_ap_config = {
    .context = M_WIFI_CONN_MQTT,
    .signal = SIG_AP_CFG,
    .next_state = M_WIFI_APCONFIG,
    .signal_handler_cb = wifi_not_cfg_to_ap_cfg_handler
};

void ap_config_done_handler(void *arg)
{
    m_wifi_t wf = m_wifi_get_instance();
    // stop http server
    m_http_stop();

    // stop wifi ap
    m_wifi_get_instance().wifi_ap_stop_cb();

    // set new led states
    m_led_stt_get_instance()._set_led_stt_wifi_not_connect_cb();

    // wifi sta start
    char *ssid = wf._wifi_info._ssid;
    char *pass = wf._wifi_info._passwd;
    if (m_db_get_instance()->_get_dev_pair_state()) {
        use_server_prev = 1;
        wf.wifi_sta_start_cb(ssid, pass);
        ble_paired_need_connect();
    }
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

    m_ir_deinit();
    
    m_led_stt_get_instance()._set_led_stt_ap_config_cb();

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