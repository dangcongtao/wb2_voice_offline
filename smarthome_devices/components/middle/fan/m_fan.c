#include "m_fan.h"
#include "hardware.h"

static m_fan_runtime_t s_fan_runtime;

void m_fan_set_speed(uint8_t speed)
{
    s_fan_runtime.speed = 0;
}

void m_fan_toggle_speed(void)
{
    s_fan_runtime.speed = s_fan_runtime.speed + 1;
    if (s_fan_runtime.speed > 3)
    {
        s_fan_runtime.speed = 1;
    }
    os_trace("%s speed: %u\r\n", __func__, s_fan_runtime.speed);
    hw_fan_set_speed(s_fan_runtime.speed);
}

void m_fan_set_swing(uint8_t swing)
{
    hw_fan_set_swing(swing);
}

void m_fan_toogle_swing(void)
{
    s_fan_runtime.swing = !s_fan_runtime.swing;
    os_trace("%s swing: %u\r\n", __func__, s_fan_runtime.swing);
    hw_fan_set_swing(s_fan_runtime.swing);
}

void m_led_7_seg_loop(void)
{

}

void m_fan_loop(void)
{

}

void m_fan_init(void)
{

}