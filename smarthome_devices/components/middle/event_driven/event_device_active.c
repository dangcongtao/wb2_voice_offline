#include "event_driven.h"

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"
#include "m_http_client.h"

void get_mqtt_info_handler(void *arg)
{
    // http client send GET request
    os_trace("get_mqtt_info_handler\r\n");
    get_mqtt_info();
    *((m_signal_t *)arg) = SIG_MQTT_GOT_INFO;
}
m_state_transmit_t dev_get_mqtt = {
    .context = M_WIFI_GET_MQTT,
    .signal = SIG_WIFI_CONNECTED,
    .next_state = M_WIFI_CONN_MQTT,
    .signal_handler_cb = get_mqtt_info_handler
};

void connect_mqtt_broker_handler(void *arg)
{
    os_trace("connect_mqtt_broker_handler\r\n");

    /* stop ble */
    ble_deinit();

    /* mqtt start */
    m_mqtt_get_instance()._mqtt_start();
}
m_state_transmit_t dev_conn_mqtt = {
    .context = M_WIFI_CONN_MQTT,
    .signal = SIG_MQTT_GOT_INFO,
    .next_state = M_WIFI_WAIT_MQTT_CONNECTED,
    .signal_handler_cb = connect_mqtt_broker_handler
};

m_state_transmit_t dev_conn_mqtt_on_init = {
    .context = M_WIFI_NOT_CFG,
    .signal = SIG_WIFI_CONNECTED,
    .next_state = M_WIFI_WAIT_MQTT_CONNECTED,
    .signal_handler_cb = connect_mqtt_broker_handler
};

void device_active_handler(void *arg)
{
    os_trace("device_active_handler\r\n");
    // ble init
    ble_init();
}
m_state_transmit_t dev_actived = {
    .context = M_WIFI_WAIT_MQTT_CONNECTED,
    .signal = SIG_MQTT_CONNECTED,
    .next_state = M_WIFI_DEV_ACTIVE,
    .signal_handler_cb = device_active_handler
};

/* device was actived */
void dev_actived_to_smart_cfg_handler(void *arg)
{
    // stop sta
    m_wifi_get_instance().wifi_ap_stop_cb();

    // stop mqtt
    m_mqtt_get_instance()._mqtt_stop();

    // enable ble
    ble_init();
    
    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();
}
m_state_transmit_t dev_actived_to_smart_cfg = {
    .context = M_WIFI_DEV_ACTIVE,
    .signal = SIG_SMART_CFG,
    .next_state = M_WIFI_SMARTCONFIG,
    .signal_handler_cb = dev_actived_to_smart_cfg_handler
};
