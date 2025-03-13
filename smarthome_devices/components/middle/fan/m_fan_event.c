#include "m_fan_event.h"
#include "m_fan_control.h"
#include "hardware.h"
#include "m_ft62f28.h"
#include "m_fan.h"

static m_fan_natural_mode_t s_natural_mode;
static uint32_t time_set;
static uint32_t time_remain;
uint32_t last_tick_timer_toggle;

uint8_t m_fan_get_current_speed_natural(void)
{
    return s_natural_mode.speed_current;
}

void m_fan_set_time_off(uint32_t _time_set)
{
    os_trace("%s time_set: %u\r\n", __func__, _time_set);
    time_remain = _time_set * 1000;
    time_set = _time_set * 1000;
    last_tick_timer_toggle = 0;
    if (time_remain > 0)
    {
        hw_led_stt_set_stt(M_FAN_BUTTON_TIMER_INDEX, LED_INDICATE_ON);
    }
    else
    {
        hw_led_stt_set_stt(M_FAN_BUTTON_TIMER_INDEX, LED_INDICATE_OFF);
    }
}

uint32_t m_fan_get_time_remain(void)
{
    return time_remain / 1000;
}

uint32_t m_fan_get_time_set(void)
{
    return time_set / 1000;
}

void m_fan_start_natural_mode(void)
{
    if (device_type == FANDC)
    {
        s_natural_mode.speed_current = 10; // 10%
    }
    else if (device_type == FANAC || device_type == FANACIR)
    {
        s_natural_mode.speed_current = M_FAN_SPEED_1;
    }
    m_ft62f28_send_speed();
    s_natural_mode.time_current = 0;
}

void m_fan_toggle_natural_mode(void)
{
    m_fan_info_t *my_fan = m_fan_get_instance();
    if (my_fan->mode == M_FAN_MODE_OFF)
    {
        my_fan->mode = M_FAN_MODE_NATURAL;
        m_fan_start_natural_mode();
        // my_fan->speed = M_FAN_SPEED_1;
        os_trace("%s mode_natural: %u\r\n", __func__, my_fan->mode);
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_ON);
        // hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_INDICATE_ON);
    }
    else
    {
        if (my_fan->mode == M_FAN_MODE_NATURAL)
        {
            my_fan->mode = M_FAN_MODE_ON;
            m_fan_set_speed(my_fan->mode, my_fan->speed);
            hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
        }
        else
        {
            my_fan->mode = M_FAN_MODE_NATURAL;
            m_fan_start_natural_mode();
            os_trace("%s mode_natural: %u\r\n", __func__, my_fan->mode);
            hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_ON);
            // hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_INDICATE_ON);
        }
    }
    m_fan_need_buzz();
}

void m_fan_mode_natural_process(uint32_t delay_ms)
{
    m_fan_info_t *my_fan = m_fan_get_instance();
    if (my_fan->mode == M_FAN_MODE_NATURAL)
    {
        s_natural_mode.time_current += delay_ms;
        if (device_type == FANDC)
        {
            if (s_natural_mode.time_current >= TIME_STEP_FAN_NATURAL_MODE_DC_MS)
            {
                s_natural_mode.time_current = 0;

                s_natural_mode.speed_current -= 10;
                if (s_natural_mode.speed_current == 0 || s_natural_mode.speed_current > 100)
                {
                    s_natural_mode.speed_current = 100;
                }
                hw_fan_set_speed_pcn(s_natural_mode.speed_current);
            }
            // os_trace("m_fan_mode_natural_process %u\r\n", s_natural_mode.speed_current);
        }
        else if (device_type == FANAC || device_type == FANACIR)
        {
            if (s_natural_mode.time_current >= TIME_STEP_FAN_NATURAL_MODE_AC_MS)
            {
                s_natural_mode.time_current = 0;
                if (s_natural_mode.speed_current == 0 || s_natural_mode.speed_current > M_FAN_SPEED_3)
                {
                    s_natural_mode.speed_current = M_FAN_SPEED_3;
                }
                else
                {
                    s_natural_mode.speed_current--;
                }
                hw_fan_set_speed_level(s_natural_mode.speed_current);
            }
        }
    }
}

void m_fan_timer_toggle(void)
{
    m_fan_info_t *my_fan = m_fan_get_instance();
    if (my_fan->mode == M_FAN_MODE_OFF)
    {
        os_trace("can't set time cos fan off\r\n");
        return;
    }
    m_fan_need_buzz();
    uint32_t time_hours = time_set / (60 * 60 * 1000);
    time_hours++;
    if (time_hours > 9)
    {
        time_hours = 0;
    }
    m_fan_set_time_off(time_hours * 60 * 60);
    last_tick_timer_toggle = sys_get_tick_ms();
    os_trace("%s hours: %u\r\n", __func__, time_hours);
    hw_74hc595_set_number(time_hours);
}

void m_fan_timer_loop(uint32_t delay_ms)
{
    if (time_remain > 0)
    {
        time_remain -= delay_ms;
        if (time_remain == 0)
        {
            hw_led_stt_set_stt(M_FAN_BUTTON_TIMER_INDEX, LED_INDICATE_OFF);
            m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
        }
    }
}