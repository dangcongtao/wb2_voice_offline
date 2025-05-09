#include "m_fan_control.h"
#include "hardware.h"
#include "m_ft62f28.h"
#include "m_fan.h"

static m_fan_runtime_t s_fan_runtime;

uint8_t buzz_num;
uint8_t prevent_buzz = 0;

void m_fan_need_buzz(uint8_t num_buzz)
{
    num_buzz++;
    if (m_fan_get_instance()->disable_buzz != 1 && !prevent_buzz)
    {
        if (num_buzz > buzz_num)
        {
            buzz_num = num_buzz;
        }
        hw_buzz_set(SWITCH_ACTIVE_LEVEL);
    }
}

void m_buzz_play(uint8_t num_buzz)
{
    num_buzz++;
    if (m_fan_get_instance()->disable_buzz != 1)
    {
        buzz_num = num_buzz;
        hw_buzz_set(SWITCH_ACTIVE_LEVEL);
    }
}

uint8_t fan_get_func_from_index(uint8_t my_func)
{
    if (device_type == FANDC)
    {
        return my_func;
    }
    else if (device_type == FANACIR || device_type == FANAC)
    {
        switch (my_func)
        {
        case 3:
            return M_FAN_BUTTON_TIMER_INDEX;
        case 2:
            return M_FAN_BUTTON_NATURAL_INDEX;
        case 1:
            return M_FAN_BUTTON_SWING_INDEX;
        case 0:
            return M_FAN_BUTTON_INCREASE_INDEX;

        default:
            return M_FAN_BUTTON_INVALID;
            break;
        }
    }
    // else if (device_type == FANAC)
    // {
    //     switch (my_func)
    //     {
    //     case 3:
    //         return M_FAN_BUTTON_NATURAL_INDEX;
    //     case 2:
    //         return M_FAN_BUTTON_TIMER_INDEX;
    //     case 1:
    //         return M_FAN_BUTTON_SWING_INDEX;
    //     case 0:
    //         return M_FAN_BUTTON_INCREASE_INDEX;

    //     default:
    //         return M_FAN_BUTTON_INVALID;
    //         break;
    //     }
    // }

    return my_func;
}

uint8_t fan_get_index_from_func(uint8_t my_func)
{
    if (device_type == FANDC)
    {
        return my_func;
    }
    else if (device_type == FANACIR)
    {
        switch (my_func)
        {
        case M_FAN_BUTTON_TIMER_INDEX:
            return 3;
            break;
        case M_FAN_BUTTON_NATURAL_INDEX:
            return 2;
            break;
        case M_FAN_BUTTON_SWING_INDEX:
            return 1;
            break;
        case M_FAN_BUTTON_INCREASE_INDEX:
            return 0;
            break;

        default:
            return M_FAN_BUTTON_INVALID;
            break;
        }
    }
    else if (device_type == FANAC)
    {
        switch (my_func)
        {
        case M_FAN_BUTTON_NATURAL_INDEX:
            return 3;
            break;
        case 0:
        case 1:
        case 2:
            return my_func;
            break;
        default:
            return M_FAN_BUTTON_INVALID;
            break;
        }
    }

    return my_func;
}

void hw_led_stt_set_stt(uint8_t index, uint8_t val)
{
    index = fan_get_index_from_func(index);
    if (index == M_FAN_BUTTON_INVALID)
    {
        return;
    }
    uint8_t old_val_stt = s_fan_runtime.led_stt_data;
    s_fan_runtime.led_stt_data &= ~(1 << index);
    s_fan_runtime.led_stt_data |= (val << index);
    if (old_val_stt != s_fan_runtime.led_stt_data)
    {
        m_ft62f28_send_led();
    }
}

void hw_led_stt_toggle_stt(uint8_t index)
{
    uint8_t current_val = (s_fan_runtime.led_stt_data >> index) & 1;
    hw_led_stt_set_stt(index, !current_val);
}

uint8_t m_fan_get_led_stt(void)
{
    return s_fan_runtime.led_stt_data;
}

void hw_fan_set_speed_pcn(uint8_t speed)
{
    if (speed <= 100)
    {
        m_ft62f28_send_speed();
    }
}

void hw_fan_set_speed_level(uint8_t speed)
{
    os_trace("hw_fan_set_speed_level speed %u\r\n", speed);
    switch (speed)
    {
    case M_FAN_SPEED_OFF:
        m_ft62f28_send_speed();
        break;
    case M_FAN_SPEED_1:
        m_ft62f28_send_speed();
        break;
    case M_FAN_SPEED_2:
        m_ft62f28_send_speed();
        break;
    case M_FAN_SPEED_3:
        m_ft62f28_send_speed();
        break;
    default:
        break;
    }
}

void hw_fan_set_swing(uint8_t swing)
{
    s_fan_runtime.swing_mode = swing;
    if (swing)
    {
        hw_led_stt_set_stt(M_FAN_BUTTON_SWING_INDEX, LED_INDICATE_ON);
    }
    else
    {
        hw_led_stt_set_stt(M_FAN_BUTTON_SWING_INDEX, LED_INDICATE_OFF);
    }
    m_ft62f28_send_swing(swing);
}

void buzz_loop(void)
{
    if (buzz_num)
    {
        buzz_num--;
        if (buzz_num == 0)
        {
            hw_buzz_set(SWITCH_IDLE_LEVEL);
        }
    }
}