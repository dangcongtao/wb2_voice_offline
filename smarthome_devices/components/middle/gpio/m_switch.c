#include "m_switch.h"
#include "hardware.h"

#include <stdint.h>

#include "m_button.h"
#include "m_mqtt.h"
#include "system_typedef.h"
#include "m_database.h"

static m_switch_t s_switch_obj;
char respond_msg[256];

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
static uint8_t m_control_switch(uint32_t action, uint32_t act_id, uint32_t uid, bool respond, bool set_even_release)
{
    uint8_t channel_index = (action - 1) / 2;
    state_t state = (action % 2) ? HIGH : LOW;
    // m_mqtt_t mqtt_obj = m_mqtt_get_instance();
    m_database_t *db_instance = m_db_get_instance();

    os_trace("m_control_switch: %d, %d\n", channel_index, state);
    
    if (channel_index > MAX_NUM_SWITCH)
    {
        os_trace("channel out of index\n");
        return 0;
    }

    s_switch_obj._write_switch_cb(channel_index, state);
    set_user_trigger_press_event(channel_index, set_even_release, action);
    db_instance->_set_current_state_cb(channel_index, state);
    /* save current state if needed */
    uint8_t def_state = db_instance->_m_device._switch_info.default_state[channel_index] % 3;
    if (def_state == SWITCH_ONE_KEEP_OLD_STATE)
    {
        db_instance->_request_sync_db();
    }

    // /* respond server here */
    // if (uid == 0)
    // {
    //     if (mqtt_obj._mqtt_state >= MQTT_INTIALIZED)
    //     {
    //         memset(respond_msg, 0, sizeof(respond_msg));
    //         sprintf(respond_msg, "{\"%s\":%d, \"action\":%ldu,"
    //                              "\"u\":%ld, \"act_id\": %d}",
    //                              gs_dev_type[device_type], 0, action, uid, 1);
    //         mqtt_obj._mqtt_publish(mqtt_obj.mqtt_info->topic_pub, respond_msg, strlen(respond_msg));
    //     }
    //     else
    //     {
    //         os_trace("Device is not connect mqtt\n");
    //     }
    // }

    return 1;
}

/* use for Button or Press-Release action */
static uint8_t m_toggle_switch(uint32_t index, uint32_t act_id, bool respond, bool set_even_release)
{
    uint8_t pin_value = s_switch_obj._read_switch_cb(index);

    // os_trace("m_toggle_switch %d: %d\n", index, pin_value);

    /* control switch one */
    if (index == APP_SWITCH_ONE)
    {
        if (pin_value)
        {
            return m_control_switch(CH1_OFF, act_id, 0, respond, set_even_release);
        }
        else
        {
            return m_control_switch(CH1_ON, act_id, 0, respond, set_even_release);
        }
    }

    /* control switch two */
    else if (index == APP_SWITCH_TWO)
    {
        if (pin_value)
        {
            return m_control_switch(CH2_OFF, act_id, 0, respond, set_even_release);
        }
        else
        {
            return m_control_switch(CH2_ON, act_id, 0, respond, set_even_release);
        }
    }

    /* control switch three */
    else if (index == APP_SWITCH_THREE)
    {
        if (pin_value)
        {
            return m_control_switch(CH3_OFF, act_id, 0, respond, set_even_release);
        }
        else
        {
            return m_control_switch(CH3_ON, act_id, 0, respond, set_even_release);
        }
    }

    /* control switch four three */
    else if (index == APP_SWITCH_FOUR)
    {
        if (pin_value)
        {
            return m_control_switch(CH4_OFF, act_id, 0, respond, set_even_release);
        }
        else
        {
            return m_control_switch(CH4_ON, act_id, 0, respond, set_even_release);
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

    for (channel = 0; channel < max_number_channel; channel++)
    {
        mode = db_instance->_m_device._switch_info.default_state[channel];

        if (mode >= 3)
            mode %= 3;

        switch (mode)
        {
        case SWITCH_ONE_KEEP_OLD_STATE:
        {
            state = db_instance->_m_device._switch_info.current_state[channel] % 2;
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
            os_trace("control_switch_default_state default case\n");
        }
        break;
        }

        if (s_switch_obj._write_switch_cb != NULL)
        {
            os_trace("control_switch_default_state - %d : %s\n", channel, state ? "ON" : "OFF");
            s_switch_obj._write_switch_cb(channel, state);
            
            /* save state to database */
            state = m_control_get_status_switch(channel);
            db_instance->_set_current_state_cb(channel, state);
        }
        else
        {
            os_trace("s_switch_obj._write_switch_cb is NULL\n");
        }
    }
}

/************************* public api ****************************/
void m_switch_init(void)
{
    os_trace("m_switch_init: %d\n", max_number_channel);

    s_switch_obj.max_number_channel = max_number_channel;
    s_switch_obj._switch_cfg_cb = hw_switch_cfg;
    s_switch_obj._read_switch_cb = hw_read_switch_pin;
    s_switch_obj._write_switch_cb = hw_write_switch_pin;
    s_switch_obj._toggle_switch_cb = m_toggle_switch;
    s_switch_obj._control_switch_cb = m_control_switch;

    uint32_t i = 0;
    for (i = 0; i < max_number_channel; i++)
        s_switch_obj._switch_cfg_cb(i);
    
    control_switch_default_state();
}

m_switch_t m_switch_get_instance(void)
{
    return s_switch_obj;
}
