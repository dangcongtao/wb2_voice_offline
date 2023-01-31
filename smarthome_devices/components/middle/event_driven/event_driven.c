#include "event_driven.h"

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"

#define MAX_STATE_MACHINE 10

static m_signal_t signal;
static uint8_t num_state_in_used = 0;
m_state_t current_state;
m_state_transmit_t state_array[MAX_STATE_MACHINE] = {0};

extern m_state_transmit_t not_cfg_to_smart_cfg;
extern m_state_transmit_t dev_actived_to_smart_cfg;
extern m_state_transmit_t wait_mqtt_connect_to_smartcfg;
extern m_state_transmit_t smartconfig_done;

extern m_state_transmit_t not_cfg_to_ap_cfg;
extern m_state_transmit_t dev_actived_to_ap_cfg;
extern m_state_transmit_t ap_config_done;

extern m_state_transmit_t dev_get_mqtt;
extern m_state_transmit_t dev_conn_mqtt;
extern m_state_transmit_t dev_conn_mqtt_on_init;
extern m_state_transmit_t dev_actived;

void signal_handler_register(m_state_transmit_t new_state)
{
    if (num_state_in_used >= MAX_STATE_MACHINE)
    {
        return;
    }

    state_array[num_state_in_used] = new_state;
    num_state_in_used++;
}

void signal_post(m_signal_t sig)
{
    signal = sig;
}

void m_event_init(void)
{
    signal = SIG_NONE;
    current_state = M_WIFI_NOT_CFG;
    memset(state_array, 0, sizeof(state_array));

    signal_handler_register(not_cfg_to_smart_cfg);
    signal_handler_register(dev_actived_to_smart_cfg);
    signal_handler_register(smartconfig_done);
    signal_handler_register(not_cfg_to_ap_cfg);
    signal_handler_register(ap_config_done);
    signal_handler_register(dev_get_mqtt);
    signal_handler_register(dev_conn_mqtt);
    signal_handler_register(dev_actived);
    signal_handler_register(dev_conn_mqtt_on_init);
    signal_handler_register(wait_mqtt_connect_to_smartcfg);
    
    /* check pair state */
    if (m_db_get_instance()->_get_dev_pair_state())
    {
        char *ssid = m_wifi_get_instance()._wifi_info._ssid;
        char *pass = m_wifi_get_instance()._wifi_info._passwd;
        m_wifi_get_instance().wifi_sta_start_cb(ssid, pass);
    }
}

void m_event_loop(void)
{
    for (uint8_t i = 0; i < SIZEOF_ARRAY(state_array); i++)
    {
        if ((signal == state_array[i].signal) && (current_state == state_array[i].context))
        {
            if (state_array[i].signal_handler_cb)
                state_array[i].signal_handler_cb(&signal);
            current_state = state_array[i].next_state;
            break;
        }
        else if (i == (SIZEOF_ARRAY(state_array) - 1))
        {
            /* get signal but invalid context to handle */
        }
    }
}