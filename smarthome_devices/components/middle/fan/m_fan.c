#include "m_fan.h"
#include "hardware.h"

static m_fan_runtime_t s_fan_runtime;
static m_fan_info_t s_fan_info;

static const uint8_t arr_seg_7[] = {
    0b10000001, // 0 (0x40) - Các thanh 0,1,2,3,4,5 sáng (bit 0), thanh 6 tắt (bit 1)
    0b11110011, // 1 (0x79) - Thanh 1,2 sáng
    0b01001001, // 2 (0x24) - Thanh 0,1,6,4,3 sáng
    0b01100001, // 3 (0x30) - Thanh 0,1,6,2,3 sáng
    0b00110011, // 4 (0x19) - Thanh 1,2,5,6 sáng
    0b00100101, // 5 (0x12) - Thanh 0,6,2,3,5 sáng
    0b00000101, // 6 (0x02) - Thanh 0,6,2,3,4,5 sáng
    0b11110001, // 7 (0x78) - Thanh 0,1,2 sáng
    0b00000001, // 8 (0x00) - Tất cả sáng
    0b00100001  // 9 (0x10) - Thanh 0,1,2,3,5,6 sáng
};

void m_fan_set_number_led(uint8_t num)
{
    s_fan_runtime.led_7_seg_data = arr_seg_7[num];
}

void m_fan_set_time_off(uint32_t time_set)
{
    s_fan_runtime.time_remain = time_set;
    s_fan_runtime.time_set = time_set;
}

uint32_t m_fan_get_time_remain(void)
{
    return s_fan_runtime.time_remain;
}

void m_fan_set_speed(uint8_t mode_set, uint8_t speed_set)
{
    s_fan_info.mode = mode_set;
    s_fan_info.speed = speed_set;
    if (s_fan_info.mode == M_FAN_MODE_ON)
    {
        hw_fan_set_speed(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
    }
}

void m_fan_toggle_speed(void)
{
    s_fan_info.speed = s_fan_info.speed + 1;
    if (s_fan_info.speed > 3)
    {
        s_fan_info.speed = 1;
    }
    os_trace("%s speed: %u\r\n", __func__, s_fan_info.speed);
    hw_fan_set_speed(s_fan_info.speed);
    m_fan_set_number_led(s_fan_info.speed);
}

void m_fan_set_swing(uint8_t swing)
{
    hw_fan_set_swing(swing);
}

void m_fan_toogle_swing(void)
{
    s_fan_info.swing = !s_fan_info.swing;
    os_trace("%s swing: %u\r\n", __func__, s_fan_info.swing);
    hw_fan_set_swing(s_fan_info.swing);
}

void m_led_7_seg_loop(void)
{
    static uint8_t tick_10ms = 0;
    static uint8_t num_curr = 0;
    if (s_fan_runtime.step == 0)
    {
        bl_gpio_output_set(LED_COM_PIN, 0);
        hw_74hc595_write_byte(s_fan_runtime.led_7_seg_data);
        s_fan_runtime.step = 1;
    }
    else if (s_fan_runtime.step == 1)
    {
        s_fan_runtime.step = 0;
        // thứ tự bị ngược, bit cuối cùng là bit com
        hw_74hc595_write_byte(s_fan_runtime.led_stt_data);
        bl_gpio_output_set(LED_COM_PIN, 1);
    }

    tick_10ms++;
    if (tick_10ms >= 100)
    {
        tick_10ms = 0;
        num_curr++;
        s_fan_runtime.led_7_seg_data = arr_seg_7[num_curr % 10];
        s_fan_runtime.led_stt_data = (~(0b1 << (num_curr % 7 + 1))) & 0b11111110;
    }
}

void m_fan_loop(void)
{
}

void m_fan_init(void)
{
}

m_fan_info_t *m_fan_get_instance(void)
{
    return &s_fan_info;
}