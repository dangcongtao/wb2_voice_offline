#include "event_driven.h"

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"
#include "m_http_client.h"
#include "m_mqtt.h"
#include "IRrecv.h"

// extern uint32_t app_btn_on_hold_time_fire_event_ms;

extern char s_device_id[64];
extern void signal_post(m_signal_t sig);
uint8_t use_server_prev = 1;

void get_mqtt_info_handler(void *arg)
{
    // http client send GET request
    os_trace("get_mqtt_info_handler\r\n");

    m_led_stt_get_instance()._set_led_stt_mqtt_disconnect_cb();

    m_mqtt_server_t *server_inf = m_mqtt_get_instance().get_server_info();

    if (use_server_prev && server_inf != NULL && server_inf->port != 0 && strlen(server_inf->server) > 0 && strlen(server_inf->username) > 0 && strlen(server_inf->passwd) > 0 && server_inf->type == 0)
    {
        use_server_prev = 0;
        os_trace("Device had been actived ago\r\n");
    }
    else
    {
        use_server_prev = 0;
        if (!get_mqtt_info())
        {
            if (m_mqtt_get_instance().get_num_mqtt_broker() > 0)
            {
                signal_post(SIG_MQTT_GOT_INFO);
                return;
            }
            set_timeout_reset();
            vTaskDelay(pdMS_TO_TICKS(1000));
            return;
        }
    }

    signal_post(SIG_MQTT_GOT_INFO);
}
m_state_transmit_t dev_get_mqtt = {
    .context = M_WIFI_CONNECTING,
    .signal = SIG_WIFI_CONNECTED,
    .next_state = M_WIFI_CONN_MQTT,
    .signal_handler_cb = get_mqtt_info_handler};

void get_mqtt_broker_handler(void)
{
    m_mqtt_t mqtt = m_mqtt_get_instance();
    m_mqtt_server_t *server_inf = m_mqtt_get_instance().get_server_info();

    if (server_inf != NULL && server_inf->port != 0 && strlen(server_inf->server) > 0 && strlen(server_inf->username) > 0 && strlen(server_inf->passwd) > 0 && server_inf->type == 0)
    {
        mqtt._mqtt_stop();
        vTaskDelay(pdMS_TO_TICKS(3000));
        if (mqtt.update_mqtt_broker())
        {
            mqtt._mqtt_start();
        }
        else
        {
            if (get_mqtt_info())
            {
                mqtt._mqtt_start();
            }
            else if (m_mqtt_get_instance().get_num_mqtt_broker() > 0)
            {
                mqtt._mqtt_start();
            }
            else
            {
                os_trace("%s update mqtt broker fail\r\n", __func__);
            }
        }
    }
    else
    {
        os_trace("%s get new mqtt info\r\n", __func__);
        mqtt._mqtt_stop();
        if (!get_mqtt_info())
        {
            os_trace("Get MQTT Info Fail\r\n");
            if (m_mqtt_get_instance().get_num_mqtt_broker() > 0)
            {
                vTaskDelay(pdMS_TO_TICKS(3000));
                mqtt._mqtt_start();
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(3000));
            os_trace("%s get new mqtt info success\r\n", __func__);
            mqtt._mqtt_start();
        }
    }
}

void connect_mqtt_broker_handler(void *arg)
{
    os_trace("connect_mqtt_broker_handler\r\n");

    /* stop ble */
    // ble_deinit();

    /* mqtt start */
    m_mqtt_get_instance()._mqtt_start();
}
m_state_transmit_t dev_conn_mqtt = {
    .context = M_WIFI_CONN_MQTT,
    .signal = SIG_MQTT_GOT_INFO,
    .next_state = M_WIFI_MQTT_CONNECTING,
    .signal_handler_cb = connect_mqtt_broker_handler};

void device_active_handler(void *arg)
{
    os_trace("device_active_handler\r\n");
    /* ble rename */
    ble_set_name("Dungnt98", 1);
    // ble_init();

    // app_btn_on_hold_time_fire_event_ms = APP_BTN_HOLD_EVENT_15S;
    m_ir_init();
}
m_state_transmit_t dev_actived = {
    .context = M_WIFI_MQTT_CONNECTING,
    .signal = SIG_MQTT_CONNECTED,
    .next_state = M_WIFI_DEV_ACTIVE,
    .signal_handler_cb = device_active_handler};

void device_disconnect_mqtt_handler(void *arg)
{
    os_trace("device_disconnect_mqtt_handler\r\n");
    m_led_stt_get_instance()._set_led_stt_mqtt_disconnect_cb();
    // app_btn_on_hold_time_fire_event_ms = APP_BTN_HOLD_EVENT_5S;
}
m_state_transmit_t dev_actived_disconnect_mqtt = {
    .context = M_WIFI_DEV_ACTIVE,
    .signal = SIG_MQTT_DISCONNECTED,
    .next_state = M_WIFI_MQTT_CONNECTING,
    .signal_handler_cb = device_disconnect_mqtt_handler};

void device_disconnect_wifi_handler(void *arg)
{
    os_trace("device_disconnect_wifi_handler\r\n");
    m_led_stt_get_instance()._set_led_stt_wifi_not_connect_cb();
    // app_btn_on_hold_time_fire_event_ms = APP_BTN_HOLD_EVENT_5S;
    m_mqtt_get_instance()._mqtt_stop();
}
m_state_transmit_t dev_actived_disconnect_wifi = {
    .context = M_WIFI_DEV_ACTIVE,
    .signal = SIG_WIFI_DISCONNECT,
    .next_state = M_WIFI_CONNECTING,
    .signal_handler_cb = device_disconnect_wifi_handler};
m_state_transmit_t dev_mqtt_connecting_to_disconnect_wifi = {
    .context = M_WIFI_MQTT_CONNECTING,
    .signal = SIG_WIFI_DISCONNECT,
    .next_state = M_WIFI_CONNECTING,
    .signal_handler_cb = device_disconnect_wifi_handler};
m_state_transmit_t dev_get_mqtt_info_to_disconnect_wifi = {
    .context = M_WIFI_CONN_MQTT,
    .signal = SIG_WIFI_DISCONNECT,
    .next_state = M_WIFI_CONNECTING,
    .signal_handler_cb = device_disconnect_wifi_handler};

/* device actived -> smart config */
void dev_actived_to_smart_cfg_handler(void *arg)
{
    os_trace("dev_actived_to_smart_cfg_handler\r\n");

    // set led
    m_led_stt_get_instance()._set_led_stt_smart_config_cb();
    // app_btn_on_hold_time_fire_event_ms = APP_BTN_HOLD_EVENT_5S;

    // stop mqtt
    m_mqtt_get_instance()._mqtt_stop();

    // stop sta
    m_wifi_get_instance().wifi_sta_stop_cb();

    // set led
    m_led_stt_get_instance()._set_led_stt_smart_config_cb();

    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();

    // re-enable ble
    // ble_deinit();
    ble_set_name(s_device_id, 1);
    // ble_init();
}
m_state_transmit_t dev_actived_to_smart_cfg = {
    .context = M_WIFI_DEV_ACTIVE,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = dev_actived_to_smart_cfg_handler};

/* device actived -> AP config */
void dev_actived_to_ap_cfg_handler(void *arg)
{
    m_wifi_t wf = m_wifi_get_instance();
    os_trace("dev_actived_to_ap_cfg_handler\r\n");

    // set led
    m_led_stt_get_instance()._set_led_stt_ap_config_cb();
    // app_btn_on_hold_time_fire_event_ms = APP_BTN_HOLD_EVENT_5S;

    m_ir_deinit();

    // stop mqtt
    m_mqtt_get_instance()._mqtt_stop();

    // stop sta
    wf.wifi_sta_stop_cb();

    // scan wifi
    wf.wifi_scan_cb();

    // start ap
    wf.wifi_ap_start_cb();

    // start http server
    m_http_server_start();
}
m_state_transmit_t dev_actived_to_ap_cfg = {
    .context = M_WIFI_DEV_ACTIVE,
    .signal = SIG_AP_CFG,
    .next_state = M_WIFI_APCONFIG,
    .signal_handler_cb = dev_actived_to_ap_cfg_handler};

m_state_transmit_t wifi_connecting_to_smart_config = {
    .context = M_WIFI_CONNECTING,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = dev_actived_to_smart_cfg_handler};
m_state_transmit_t wifi_connecting_to_ap_config = {
    .context = M_WIFI_CONNECTING,
    .signal = SIG_AP_CFG,
    .next_state = M_WIFI_APCONFIG,
    .signal_handler_cb = dev_actived_to_ap_cfg_handler};