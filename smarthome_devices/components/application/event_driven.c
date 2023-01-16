#include "event_driven.h"

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"

#define MAX_STATE_MACHINE      8
static uint8_t num_state_in_used = 0;

static m_signal_t signal;
static m_state_t current_state;
m_state_transmit_t state_array[MAX_STATE_MACHINE] = {0};

// /* device not config */
// {M_WIFI_NOT_CFG, SIG_SMART_CFG, M_WIFI_SMARTCONFIG},
void wifi_not_cfg_to_smart_cfg_handler(void *arg)
{
    // enable ble

    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();

    // change state to wait smartconfig done
    current_state = M_WIFI_WAIT_SMARTCONFIG_DONE;
}
m_state_transmit_t not_cfg_to_smart_cfg = {
    .context = M_WIFI_NOT_CFG, 
    .signal = SIG_SMART_CFG, 
    .next_state = M_WIFI_SMARTCONFIG, 
    .signal_handler_cb = wifi_not_cfg_to_smart_cfg_handler
};

/* device was actived */
// {M_WIFI_DEV_ACTIVE, SIG_SMART_CFG, M_WIFI_SMARTCONFIG},
void dev_actived_to_smart_cfg_handler(void *arg)
{
    // stop sta
    m_wifi_get_instance().wifi_ap_stop_cb();

    // stop mqtt
    m_mqtt_get_instance()._mqtt_stop();
    // enable ble

    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();

    // change state to wait smartconfig done
    current_state = M_WIFI_WAIT_SMARTCONFIG_DONE;
}
m_state_transmit_t dev_actived_to_smart_cfg = {
    .context = M_WIFI_DEV_ACTIVE, 
    .signal = SIG_SMART_CFG, 
    .next_state = M_WIFI_SMARTCONFIG, 
    .signal_handler_cb = dev_actived_to_smart_cfg_handler
};

/* get mqtt info -> connect mqtt info -> device active*/
// {M_WIFI_SMARTCONFIG, SIG_SMART_CFG_DONE, M_WIFI_GET_MQTT},
void smartconfig_done_handler(void *arg)
{
    // wifi sta start
    
    // get mqtt info
}
m_state_transmit_t smartconfig_done = {
    .context = M_WIFI_WAIT_SMARTCONFIG_DONE, 
    .signal = SIG_SMART_CFG_DONE, 
    .next_state = M_WIFI_GET_MQTT, 
    .signal_handler_cb = smartconfig_done_handler
};

// {M_WIFI_NOT_CFG, SIG_AP_CFG, M_WIFI_APCONFIG},
void wifi_not_cfg_to_ap_cfg_handler(void *arg)
{
    // scan wifi
    m_wifi_get_instance().wifi_scan_cb();

    // start http server
    m_http_server_start();

    // start ap
    m_wifi_get_instance().wifi_ap_start_cb();
}
m_state_transmit_t not_cfg_to_ap_cfg = {
    .context = M_WIFI_NOT_CFG, 
    .signal = SIG_AP_CFG, 
    .next_state = M_WIFI_APCONFIG, 
    .signal_handler_cb = wifi_not_cfg_to_ap_cfg_handler
};

// {M_WIFI_DEV_ACTIVE, SIG_AP_CFG, M_WIFI_APCONFIG},
void wifi_actived_to_ap_cfg_handler(void *arg)
{
    // stop sta
    // stop mqtt
    // stop ble
    
    // start http server
    // scan wifi
    // start ap
}
m_state_transmit_t dev_actived_to_ap_cfg = {
    .context = M_WIFI_DEV_ACTIVE, 
    .signal = SIG_AP_CFG, 
    .next_state = M_WIFI_APCONFIG, 
    .signal_handler_cb = wifi_actived_to_ap_cfg_handler 
};

// {M_WIFI_APCONFIG, SIG_AP_CFG_DONE, M_WIFI_GET_MQTT},
void ap_config_done_handler(void *arg)
{
    // get mqtt info
}
m_state_transmit_t ap_config_done = {
    .context = M_WIFI_APCONFIG, 
    .signal = SIG_AP_CFG_DONE, 
    .next_state = M_WIFI_GET_MQTT, 
    .signal_handler_cb = ap_config_done_handler
};

// {M_WIFI_GET_MQTT, SIG_MQTT_GOT_INFO, M_WIFI_CONN_MQTT},
void connect_mqtt_broker_handler(void *arg)
{
}
m_state_transmit_t dev_conn_mqtt = {
    .context = M_WIFI_GET_MQTT, 
    .signal = SIG_MQTT_GOT_INFO, 
    .next_state = M_WIFI_CONN_MQTT, 
    .signal_handler_cb = connect_mqtt_broker_handler
};

// {M_WIFI_CONN_MQTT, SIG_MQTT_CONNECTED, M_WIFI_DEV_ACTIVE},
void device_active_handler(void *arg)
{
    // ble init
    // mqtt
    // wifi sta
}
m_state_transmit_t dev_actived = {
    .context = M_WIFI_CONN_MQTT, 
    .signal = SIG_MQTT_CONNECTED, 
    .next_state = M_WIFI_DEV_ACTIVE, 
    .signal_handler_cb = device_active_handler
};

void signal_post(m_signal_t sig)
{
    signal = sig;
}

void signal_handler_register(m_state_transmit_t new_state)
{
    if (num_state_in_used >= MAX_STATE_MACHINE) {
        return;
    }

    state_array[num_state_in_used] = new_state;
    num_state_in_used ++;
}

#define STATE_TRANS_CNT  (sizeof(state_array) / sizeof(state_array[0]))

void m_event_loop(void)
{
    for (uint8_t i = 0; i < STATE_TRANS_CNT; i++)
    {
        if ((signal == state_array[i].signal) && (current_state == state_array[i].context))
        {
            state_array[i].signal_handler_cb(NULL);
            current_state = state_array[i].next_state;
            break;
        }
        else if (i == (STATE_TRANS_CNT - 1))
        {
            /* get signal but invalid context to handle */
        }
    }
}