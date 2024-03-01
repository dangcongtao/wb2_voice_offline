#include "m_switch.h"
#include "hardware.h"

#include <stdint.h>

#include "m_button.h"
#include "m_mqtt.h"
#include "system_typedef.h"
#include "m_database.h"
#include "event_driven.h"

static m_switch_t s_switch_obj;
static char respond_msg[256];
uint8_t last_reason_reset = 0;

m_switch_state_t m_control_get_status_switch(uint8_t index)
{
    uint8_t status_pin = s_switch_obj._read_switch_cb(index);

    if (index == 0 && status_pin)
    {
        return CH1_ON;
    }
    else if (index == 0 && !status_pin)
    {
        return CH1_OFF;
    }
    else if (index == 1 && status_pin)
    {
        return CH2_ON;
    }
    else if (index == 1 && !status_pin)
    {
        return CH2_OFF;
    }
    else if (index == 2 && status_pin)
    {
        return CH3_ON;
    }
    else if (index == 2 && !status_pin)
    {
        return CH3_OFF;
    }
    else if (index == 3 && status_pin)
    {
        return CH4_ON;
    }
    else if (index == 3 && !status_pin)
    {
        return CH4_OFF;
    }

    return 0;
}

/**
 * Public API. Call this function to control switch
 */
static uint8_t m_control_switch(uint32_t action, uint32_t act_id, uint32_t uid, bool respond, bool set_even_release, m_ble_send_t send_ble)
{
    uint8_t channel_index = (action - 1) / 2;
    state_t state = (action % 2) ? HIGH : LOW;
    m_mqtt_t mqtt_obj = m_mqtt_get_instance();
    m_database_t *db_instance = m_db_get_instance();

    // os_trace("m_control_switch: %d, %d\r\n", channel_index, state);

    if (channel_index > max_number_channel)
    {
        os_trace("channel out of index\r\n");
        return 0;
    }

    s_switch_obj._write_switch_cb(channel_index, state);
    set_user_trigger_press_event(channel_index, set_even_release, action);
    db_instance->_set_current_state_cb(channel_index, action);
    /* save current state if needed */
    uint8_t def_state = s_switch_obj._switch_info.default_state[channel_index] % 3;
    if (def_state == SWITCH_ONE_KEEP_OLD_STATE)
    {
        db_instance->_request_sync_db(BIT_FLAG_SYNC_SWITCH);
    }
#if PROJECT_SHOCK
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED && is_shock_device(device_type))
    {
        if (is_leakage_current())
        {
            memset(respond_msg, 0, sizeof(respond_msg));
            sprintf(respond_msg, "{\"%s\":%d, \"%s\":%d}", gs_dev_type[device_type], 70, CJSON_KEY_LEAKAGE, 0);
            mqtt_obj._mqtt_publish(respond_msg, strlen(respond_msg));
        }
    }
#endif
    /* respond server here */
    if (uid == 0)
    {
        if (mqtt_obj._mqtt_state >= MQTT_INTIALIZED)
        {
            memset(respond_msg, 0, sizeof(respond_msg));
            sprintf(respond_msg, "{\"%s\":%d,\"action\":%ld,"
                                 "\"u\":%ld,\"act_id\":%d}",
                    gs_dev_type[device_type], 0, action, uid, act_id);
            mqtt_obj._mqtt_publish(respond_msg, strlen(respond_msg));
        }
        else
        {
            os_trace("Device is not connect mqtt\r\n");
        }
    }

    if (send_ble.send_self) {
        m_ble_info_t *obj_ble;
        obj_ble = m_ble_get_info_reference();
        
        for (uint8_t i = 0; i < obj_ble->num_devices_link; i++) {
            if (obj_ble->list_child[i].type == DEVICE_SELF) {
                m_ble_send_t send_self;
                send_self.send_self = false;
                send_self.send_ble = send_ble.send_ble;
                send_self.need_create_new_id = send_ble.need_create_new_id;

                if (((obj_ble->list_child[i].my_act - 1) / 2) == channel_index) {
                    uint8_t next_channel_index = (obj_ble->list_child[i].opp_act - 1) / 2;
                    uint32_t next_action = (next_channel_index + 1) * 2 - state;
                    m_control_switch(next_action, SRC_ACTION_LINK_STAIR, 0, true, true, send_self);
                } else if ( ((obj_ble->list_child[i].opp_act - 1) / 2) == channel_index) {
                    uint8_t next_channel_index = (obj_ble->list_child[i].my_act - 1) / 2;
                    uint32_t next_action = (next_channel_index + 1) * 2 - state;
                    m_control_switch(next_action, SRC_ACTION_LINK_STAIR, 0, true, true, send_self);
                }
            }
        }
    }

    if (is_support_link_stair(device_type) && send_ble.send_ble){
        m_ble_set_state_change(channel_index, state, send_ble);
        // os_trace("%s: %02X:%02X\r\n", __func__, send_ble.mac_source[0], send_ble.mac_source[1]);
    }

    return 1;
}

static uint8_t num_click_go_cfg_0 = 0;
static uint8_t num_click_go_cfg_1 = 0;
static uint8_t step_go_cfg_0 = 0;
static uint8_t step_go_cfg_1 = 0;
static uint32_t tick_go_cfg_0 = 0;
static uint32_t tick_go_cfg_1 = 0;
#define DEFAULT_TICK 60U
#define TICK_INVALID 0

static uint8_t m_get_num_click_go_cfg(int id)
{
    if (id == 0)
    {
        return num_click_go_cfg_0;
    }
    else
    {
        return num_click_go_cfg_1;
    }
}

static void m_set_num_click_go_cfg(int id, uint8_t val)
{
    if (id == 0)
    {
        num_click_go_cfg_0 = val;
    }
    else
    {
        num_click_go_cfg_1 = val;
    }
}

static uint8_t m_get_step_go_cfg(int id)
{
    if (id == 0)
    {
        return step_go_cfg_0;
    }
    else
    {
        return step_go_cfg_1;
    }
}

static void m_set_step_go_cfg(int id, uint8_t val)
{
    if (id == 0)
    {
        step_go_cfg_0 = val;
    }
    else
    {
        step_go_cfg_1 = val;
    }
}

static uint32_t m_get_tick_go_cfg(int id)
{
    if (id == 0)
    {
        return tick_go_cfg_0;
    }
    else
    {
        return tick_go_cfg_1;
    }
}

static void m_set_tick_go_cfg(int id, uint32_t val)
{
    if (id == 0)
    {
        tick_go_cfg_0 = val;
    }
    else
    {
        tick_go_cfg_1 = val;
    }
}

static void reset_data_blink_config(int index)
{
    if (device_type != SWMINI || index > 1)
    {
        return;
    }
    m_set_num_click_go_cfg(index, 0);
    m_set_step_go_cfg(index, 0);
    m_set_tick_go_cfg(index, TICK_INVALID);
}

void blink_switch_config_loop(void)
{
    if (device_type != SWMINI)
    {
        return;
    }

    for (int i = 0; i < 2; i++)
    {
        if (i > 1)
        {
            return;
        }
        if (m_get_tick_go_cfg(i) == TICK_INVALID)
        {
            continue;
        }
        m_set_tick_go_cfg(i, m_get_tick_go_cfg(i) - 1);
        if (m_get_tick_go_cfg(i) > 0)
        {
            continue;
        }

        os_trace("%d num_click_go_cfg: %u, step_go_cfg: %u, tick_go_cfg: %lu\r\n", i, m_get_num_click_go_cfg(i), m_get_step_go_cfg(i), m_get_num_click_go_cfg(i));

        if (1 == m_get_step_go_cfg(i) || 2 == m_get_step_go_cfg(i))
        {
            if (0 == m_get_num_click_go_cfg(i))
            {
                m_set_tick_go_cfg(i, DEFAULT_TICK);

                os_trace("go step %u\r\n", m_get_step_go_cfg(i));
                m_ble_send_t ble_send;
                ble_send.send_ble = false;
                ble_send.need_create_new_id = false;
                ble_send.send_self = true;
                m_control_switch(m_get_step_go_cfg(i) + i * 2, 0, 0, 0, 0, ble_send);
                m_set_step_go_cfg(i, m_get_step_go_cfg(i) + 1);
                return;
            }
            else
            {
                reset_data_blink_config(i);
            }
        }
        else if (3 == m_get_step_go_cfg(i))
        {
            // if (2 == m_get_num_click_go_cfg(i))
            // {
            //     reset_data_blink_config(i);
            //     signal_post(SIG_SMART_CFG);
            //     return;
            // }
            reset_data_blink_config(i);
        }
        else if (0 == m_get_step_go_cfg(i))
        {
            if (6 == m_get_num_click_go_cfg(i))
            {
                os_trace("go config\r\n");
                m_set_tick_go_cfg(i, TICK_INVALID + 1);
                m_set_num_click_go_cfg(i, 0);
                m_set_step_go_cfg(i, m_get_step_go_cfg(i) + 1);
                return;
            }
            else
            {
                reset_data_blink_config(i);
            }
        }
    }
}

void increate_step(int index)
{
    if (index > 1)
    {
        return;
    }
    m_set_tick_go_cfg(index, DEFAULT_TICK);
    m_set_num_click_go_cfg(index, m_get_num_click_go_cfg(index) + 1);

    if (3 == m_get_step_go_cfg(index) && m_get_tick_go_cfg(index) > 0) {
        reset_data_blink_config(index);
        signal_post(SIG_SMART_CFG);
    }
}

/* use for Button or Press-Release action */
static uint8_t m_toggle_switch(uint32_t index, uint32_t act_id, bool respond, bool set_even_release)
{
    uint8_t pin_value = s_switch_obj._read_switch_cb(index);

    // os_trace("m_toggle_switch %d: %d\r\n", index, pin_value);

    m_ble_send_t ble_send;
    ble_send.send_ble = true;
    ble_send.need_create_new_id = true;
    ble_send.send_self = true;

    /* control switch one */
    if (index == APP_SWITCH_ONE)
    {
        if (pin_value)
        {
            if (m_get_num_click_go_cfg(0) % 2 != 0)
            {
                increate_step(0);
            }
            else
            {
                reset_data_blink_config(0);
            }
            return m_control_switch(CH1_OFF, act_id, 0, respond, set_even_release, ble_send);
        }
        else
        {
            if (m_get_num_click_go_cfg(0) % 2 == 0)
            {
                increate_step(0);
            }
            else
            {
                reset_data_blink_config(0);
            }
            return m_control_switch(CH1_ON, act_id, 0, respond, set_even_release, ble_send);
        }
    }

    /* control switch two */
    else if (index == APP_SWITCH_TWO)
    {
        if (pin_value)
        {
            if (m_get_num_click_go_cfg(1) % 2 != 0)
            {
                increate_step(1);
            }
            else
            {
                reset_data_blink_config(1);
            }
            return m_control_switch(CH2_OFF, act_id, 0, respond, set_even_release, ble_send);
        }
        else
        {
            if (m_get_num_click_go_cfg(1) % 2 == 0)
            {
                increate_step(1);
            }
            else
            {
                reset_data_blink_config(1);
            }
            return m_control_switch(CH2_ON, act_id, 0, respond, set_even_release, ble_send);
        }
    }

    /* control switch three */
    else if (index == APP_SWITCH_THREE)
    {
        if (pin_value)
        {
            return m_control_switch(CH3_OFF, act_id, 0, respond, set_even_release, ble_send);
        }
        else
        {
            return m_control_switch(CH3_ON, act_id, 0, respond, set_even_release, ble_send);
        }
    }

    /* control switch four three */
    else if (index == APP_SWITCH_FOUR)
    {
        if (pin_value)
        {
            return m_control_switch(CH4_OFF, act_id, 0, respond, set_even_release, ble_send);
        }
        else
        {
            return m_control_switch(CH4_ON, act_id, 0, respond, set_even_release, ble_send);
        }
    }

    return 0;
}

static void control_switch_default_state(void)
{
    uint8_t mode;
    uint8_t state = LED_STT_OFF;
    uint8_t channel;
    m_database_t *db_instance = m_db_get_instance();

    uint8_t reason_reset = flash_read_rs_reason();
    os_trace("reason_reset: %u\r\n", reason_reset);
    if (reason_reset)
    {
        for (channel = 0; channel < m_db_get_number_channel(); channel++)
        {
            state = s_switch_obj._switch_info.current_state[channel] % 2;
            os_trace("current_state - %d : %s\r\n", channel, state ? "ON" : "OFF");
            if (s_switch_obj._write_switch_cb != NULL)
            {
                s_switch_obj._write_switch_cb(channel, state);
            }
        }
        last_reason_reset = reason_reset;
        flash_set_rs_reason(0);
        return;
    }

    for (channel = 0; channel < max_number_channel; channel++)
    {
        mode = s_switch_obj._switch_info.default_state[channel];

        if (mode >= 3)
            mode %= 3;

        switch (mode)
        {
        case SWITCH_ONE_KEEP_OLD_STATE:
        {
            state = s_switch_obj._switch_info.current_state[channel] % 2;
        }
        break;

        case SWITCH_ONE_DEFAULT_ON:
        {
            state = LED_STT_OFF;
        }
        break;

        case SWITCH_ONE_DEFAULT_OFF:
        {
            state = LED_STT_ON;
        }
        break;

        default:
        {
            os_trace("control_switch_default_state default case\r\n");
        }
        break;
        }

        if (s_switch_obj._write_switch_cb != NULL)
        {
            os_trace("control_switch_default_state - %d : %s\r\n", channel, state ? "ON" : "OFF");
            s_switch_obj._write_switch_cb(channel, state);

            /* save state to database */
            state = m_control_get_status_switch(channel);
            db_instance->_set_current_state_cb(channel, state);
        }
        else
        {
            os_trace("s_switch_obj._write_switch_cb is NULL\r\n");
        }
    }
}

/************************* public api ****************************/
void m_switch_init(void)
{
    os_trace("m_switch_init: %ld\r\n", max_number_channel);

    s_switch_obj.max_number_channel = max_number_channel;
    s_switch_obj._switch_cfg_cb = hw_switch_cfg;
    s_switch_obj._read_switch_cb = hw_read_switch_pin;
    s_switch_obj._write_switch_cb = hw_write_switch_pin;
    s_switch_obj._toggle_switch_cb = m_toggle_switch;
    s_switch_obj._control_switch_cb = m_control_switch;

    hw_switch_change_pin_type();

    uint32_t i = 0;
    for (i = 0; i < max_number_channel; i++)
        s_switch_obj._switch_cfg_cb(i);

    if (device_type == WSWITCH || device_type == WSDATIC || device_type == LHSWITCH)
    {
        switch (max_number_channel)
        {
        case 1:
            s_switch_obj._switch_cfg_cb(1);
            break;

        case 2:
            s_switch_obj._switch_cfg_cb(2);
            break;
        default:
            break;
        }
    }

    control_switch_default_state();
    tick_go_cfg_0 = TICK_INVALID;
    tick_go_cfg_1 = TICK_INVALID;
}

m_switch_t m_switch_get_instance(void)
{
    return s_switch_obj;
}

m_switch_info_t *m_switch_get_info_reference(void)
{
    return &s_switch_obj._switch_info;
}
