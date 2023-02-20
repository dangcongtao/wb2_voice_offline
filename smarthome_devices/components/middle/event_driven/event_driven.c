#include "event_driven.h"

#include "m_ble.h"
#include "m_wifi.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"

#define MAX_STATE_MACHINE 20

static m_signal_t signal;
static uint8_t num_state_in_used = 0;
m_state_t current_state;
m_state_transmit_t state_array[MAX_STATE_MACHINE] = {0};

extern m_state_transmit_t not_cfg_to_ap_cfg;
extern m_state_transmit_t not_cfg_to_smart_cfg;

extern m_state_transmit_t dev_actived_to_ap_cfg;
extern m_state_transmit_t dev_actived_to_smart_cfg;

extern m_state_transmit_t ap_config_done;
extern m_state_transmit_t smartconfig_done;

extern m_state_transmit_t mqtt_connecting_to_smartcfg;
extern m_state_transmit_t mqtt_connecting_to_ap_config;

extern m_state_transmit_t wifi_connecting_to_ap_config;
extern m_state_transmit_t wifi_connecting_to_smart_config;

extern m_state_transmit_t smartconfig_to_apconfig;
extern m_state_transmit_t apconfig_to_smartconfig;

extern m_state_transmit_t dev_get_mqtt;
extern m_state_transmit_t dev_conn_mqtt;
extern m_state_transmit_t dev_actived;
extern m_state_transmit_t dev_actived_disconnect_mqtt;
extern m_state_transmit_t dev_actived_disconnect_wifi;

const char* state_machine[] = {
    "M_WIFI_START",
    "M_WIFI_DISCONNECT",
    "M_WIFI_CONNECTED",
    "M_WIFI_CONNECTING",
    "M_WIFI_SMARTCONFIG",
    "M_WIFI_WAIT_SMARTCONFIG_DONE",
    "M_WIFI_APCONFIG",
    "M_WIFI_WAIT_APCONFIG_DONE",
    "M_WIFI_APCONFIG_DONE",
    "M_WIFI_CONFIG_DONE",
    "M_WIFI_WAIT_EVENT",
    "M_WIFI_GET_MQTT",
    "M_WIFI_CONN_MQTT",
    "M_WIFI_MQTT_CONNECTING",
    "M_WIFI_DEV_ACTIVE",
    "M_WIFI_NOT_CFG",
    NULL
};

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
    printf("====>>> Set New Signal: %d, from context: %d\r\n", sig, current_state);
    signal = sig;
}

void m_event_init(void)
{
    signal = SIG_NONE;
    current_state = M_WIFI_NOT_CFG;
    memset(state_array, 0, sizeof(state_array));

    signal_handler_register(not_cfg_to_ap_cfg);
    signal_handler_register(not_cfg_to_smart_cfg);
    
    signal_handler_register(ap_config_done);
    signal_handler_register(smartconfig_done);

    signal_handler_register(wifi_connecting_to_ap_config);
    signal_handler_register(wifi_connecting_to_smart_config);

    signal_handler_register(smartconfig_to_apconfig);
    signal_handler_register(apconfig_to_smartconfig);

    signal_handler_register(dev_actived_to_smart_cfg);
    signal_handler_register(dev_actived_to_ap_cfg);

    signal_handler_register(mqtt_connecting_to_smartcfg);
    signal_handler_register(mqtt_connecting_to_ap_config);

    signal_handler_register(dev_get_mqtt);
    signal_handler_register(dev_conn_mqtt);
    signal_handler_register(dev_actived);
    signal_handler_register(dev_actived_disconnect_mqtt);
    signal_handler_register(dev_actived_disconnect_wifi);
    
    /* check pair state */
    if (m_db_get_instance()->_get_dev_pair_state())
    {
        char *ssid = m_wifi_get_instance()._wifi_info._ssid;
        char *pass = m_wifi_get_instance()._wifi_info._passwd;
        m_wifi_get_instance().wifi_sta_start_cb(ssid, pass);
        current_state = M_WIFI_CONNECTING;
    }
}

static m_signal_t old_signal;
static uint32_t time_wifi_wait_config = 0;
static const uint32_t TIMEOUT_WIFI_CONFIG_MS = 1000*60*15;
void m_event_loop(uint32_t time_schedual_ms)
{
    for (uint8_t i = 0; i < MAX_STATE_MACHINE; i++)
    {
        if ((signal == state_array[i].signal) && (current_state == state_array[i].context))
        {
            printf("================>>>>CHANGE FROM %d TO %d\r\n", current_state, state_array[i].next_state);

            if (state_array[i].signal_handler_cb) {
                signal = SIG_NONE;
                state_array[i].signal_handler_cb(&signal);
            }
            current_state = state_array[i].next_state;

            /* start count time wait connect */
            if (current_state == M_WIFI_SMARTCONFIG || current_state == M_WIFI_APCONFIG) {
                time_wifi_wait_config = 0;
            }

            break;
        }
        else if (i == (SIZEOF_ARRAY(state_array) - 1))
        {
            /* get signal but invalid context to handle */
            if (old_signal != signal) {
                printf("====>>> Signal: %d, Context: %d\r\n", signal, current_state);
                old_signal = signal;
            }
        }
    }
    
    if (current_state == M_WIFI_SMARTCONFIG)
    {
        /* count time wait wifi config */
        time_wifi_wait_config += time_schedual_ms;

        /* check timeout */
        if (time_wifi_wait_config >= TIMEOUT_WIFI_CONFIG_MS) {
            /* reset old configuration and connect to old wifi, mqtt */
            time_wifi_wait_config = 0;
            m_db_get_instance()->_backup_db_cb();
            signal_post(SIG_SMART_CFG_DONE);
        }
    }
    else if (current_state == M_WIFI_APCONFIG)
    {
        /* count time wait wifi config */
        time_wifi_wait_config += time_schedual_ms;

        /* check timeout */
        if (time_wifi_wait_config >= TIMEOUT_WIFI_CONFIG_MS) {
            /* reset old configuration and connect to old wifi, mqtt */
            time_wifi_wait_config = 0;
            m_db_get_instance()->_backup_db_cb();
            signal_post(SIG_AP_CFG_DONE);
        }
    }
}