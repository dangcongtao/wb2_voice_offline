#include "m_fan.h"
#include "hardware.h"
#include "m_mqtt.h"

static m_fan_runtime_t s_fan_runtime;
static m_fan_info_t s_fan_info;
extern char respond[512];

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

void hw_led_stt_set_stt(uint8_t index, uint8_t val)
{
    s_fan_runtime.led_stt_data &= ~(1 << index);
    s_fan_runtime.led_stt_data |= (val << index);
}

void hw_fan_set_speed(uint8_t speed)
{
    switch (speed)
    {
    case M_FAN_SPEED_OFF:
        s_fan_runtime.switch_data_output = 0b00000000;
        hw_led_stt_set_stt(LED_SPEED_NATURAL_IDX, LED_STT_OFF);
        hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_STT_OFF);
        hw_led_stt_set_stt(LED_SPEED_SWING_IDX, LED_STT_OFF);
        break;
    case M_FAN_SPEED_1:
        s_fan_runtime.switch_data_output &= 0b10010;
        s_fan_runtime.switch_data_output |= 0b10;
        hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_STT_ON);
        break;
    case M_FAN_SPEED_2:
        s_fan_runtime.switch_data_output &= 0b10100;
        s_fan_runtime.switch_data_output |= 0b100;
        hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_STT_ON);
        break;
    case M_FAN_SPEED_3:
        s_fan_runtime.switch_data_output &= 0b11000;
        s_fan_runtime.switch_data_output |= 0b1000;
        hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_STT_ON);
        break;
    default:
        break;
    }
}

void hw_fan_set_swing(uint8_t swing)
{
    if (swing)
    {
        s_fan_runtime.switch_data_output |= 0b10000;
        hw_led_stt_set_stt(LED_SPEED_SWING_IDX, LED_STT_ON);
    }
    else
    {
        s_fan_runtime.switch_data_output &= 0b01111;
        hw_led_stt_set_stt(LED_SPEED_SWING_IDX, LED_STT_OFF);
    }
}

void m_fan_respond_control(uint32_t uid, uint8_t act_id)
{
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld,\"mode\":%u,\"lv\":%u,\"swing\":%u,\"u\":%lu,\"act_id\":%lu}", gs_dev_type[device_type], 20, s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, uid, act_id);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

void m_fan_set_time_off(uint32_t time_set)
{
    os_trace("%s time_set: %u\r\n", __func__, time_set);
    s_fan_runtime.time_remain = time_set * 1000;
    s_fan_runtime.time_set = time_set * 1000;
}

uint32_t m_fan_get_time_remain(void)
{
    return s_fan_runtime.time_remain / 1000;
}

uint32_t m_fan_get_time_set(void)
{
    return s_fan_runtime.time_set / 1000;
}

static void m_fan_start_natural_mode(void)
{
}

void m_fan_set_buzz(uint8_t buzz_set)
{
    s_fan_info.buzz = buzz_set;
}

void m_fan_set_speed(uint8_t mode_set, uint8_t speed_set)
{
    os_trace("%s mode: %u, speed: %u\r\n", __func__, mode_set, speed_set);
    s_fan_info.mode = mode_set;
    s_fan_info.speed = speed_set;
    if (s_fan_info.mode == M_FAN_MODE_ON)
    {
        hw_fan_set_speed(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
        hw_led_stt_set_stt(LED_SPEED_NATURAL_IDX, LED_STT_OFF);
    }
    else if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        hw_fan_set_speed(M_FAN_SPEED_OFF);
        m_fan_set_number_led(M_FAN_SPEED_OFF);
        m_fan_set_swing(M_FAN_MODE_OFF);
        hw_led_stt_set_stt(LED_SPEED_NATURAL_IDX, LED_STT_OFF);
        hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_STT_OFF);
        hw_led_stt_set_stt(LED_SPEED_SWING_IDX, LED_STT_OFF);
    }
    else if (s_fan_info.mode == M_FAN_MODE_NATURAL)
    {
        if (s_fan_info.speed == M_FAN_MODE_OFF)
        {
            s_fan_info.speed = M_FAN_SPEED_1;
        }
        hw_led_stt_set_stt(LED_SPEED_NATURAL_IDX, LED_STT_ON);
        hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_STT_ON);
    }
}

void m_fan_control(uint8_t mode_set, uint8_t speed_set, uint8_t swing_set, uint32_t uid, uint8_t act_id, uint8_t respond)
{
    m_fan_set_swing(swing_set);
    m_fan_set_speed(mode_set, speed_set);
    if (respond)
    {
        m_fan_respond_control(uid, act_id);
    }
}

void m_fan_toggle_speed(void)
{
    if (s_fan_info.mode == M_FAN_MODE_ON)
    {
        s_fan_info.speed = s_fan_info.speed + 1;
        if (s_fan_info.speed > 3)
        {
            s_fan_info.speed = 1;
        }
        os_trace("%s M_FAN_MODE_ON speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
    }
    else if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        s_fan_info.mode = M_FAN_MODE_ON;
        s_fan_info.speed = M_FAN_SPEED_1;
        os_trace("%s from M_FAN_MODE_OFF to speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
    }
    else if (s_fan_info.mode == M_FAN_MODE_NATURAL)
    {
        s_fan_info.mode = M_FAN_MODE_ON;
        if (s_fan_info.speed == M_FAN_SPEED_OFF)
        {
            s_fan_info.speed = M_FAN_SPEED_1;
        }
        os_trace("%s M_FAN_MODE_NATURAL speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
    }
}

void m_fan_set_swing(uint8_t swing)
{
    hw_fan_set_swing(swing);
}

void m_fan_toogle_swing(void)
{
    if (s_fan_info.mode != M_FAN_MODE_OFF)
    {
        s_fan_info.swing = !s_fan_info.swing;
        os_trace("%s swing: %u\r\n", __func__, s_fan_info.swing);
        hw_fan_set_swing(s_fan_info.swing);
    }
    else
    {
        os_trace("%s cant swing when fan is off\r\n", __func__);
    }
}

void m_fan_toggle_natural_mode(void)
{
    if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        s_fan_info.mode = M_FAN_MODE_NATURAL;
        s_fan_info.speed = M_FAN_SPEED_1;
        os_trace("%s mode_natural: %u\r\n", __func__, s_fan_info.mode);
        hw_led_stt_set_stt(LED_SPEED_NATURAL_IDX, LED_STT_ON);
        hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_STT_ON);
    }
    else
    {
        if (s_fan_info.mode == M_FAN_MODE_NATURAL)
        {
            s_fan_info.mode = M_FAN_MODE_ON;
            m_fan_set_speed(s_fan_info.mode, s_fan_info.speed);
            hw_led_stt_set_stt(LED_SPEED_NATURAL_IDX, LED_STT_OFF);
        }
        else
        {
            s_fan_info.mode = M_FAN_MODE_NATURAL;
            os_trace("%s mode_natural: %u\r\n", __func__, s_fan_info.mode);
            hw_led_stt_set_stt(LED_SPEED_NATURAL_IDX, LED_STT_ON);
            hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_STT_ON);
        }
    }
}

static void m_fan_timer_loop(uint32_t delay_ms)
{
    if (s_fan_runtime.time_remain > 0)
    {
        s_fan_runtime.time_remain -= delay_ms;
        if (s_fan_runtime.time_remain == 0)
        {
            m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
        }
    }
}

static void m_fan_set_data_hc595(uint8_t data)
{
    // if (request_change_hc595_second)
    // {
    //     request_change_hc595_second = 0;
    //     hw_74hc595_write_byte((uint16_t)data | (0b00000001 << 8), 2);
    // }
    // else
    // {
    // hw_74hc595_write_byte(data, 1);
    hw_74hc595_write_byte((uint16_t)data | ((uint16_t)s_fan_runtime.switch_data_output << 8), 2);
    // }
}

void m_led_7_seg_loop(uint32_t delay_ms)
{
    static uint8_t tick_10ms = 0;
    static uint8_t num_curr = 0;

    m_fan_timer_loop(delay_ms);
    // return;

    if (s_fan_runtime.step == 0)
    {
        bl_gpio_output_set(LED_COM_PIN, 0);
        m_fan_set_data_hc595(s_fan_runtime.led_7_seg_data);
        s_fan_runtime.step = 1;
    }
    else if (s_fan_runtime.step == 1)
    {
        s_fan_runtime.step = 0;
        // thứ tự bị ngược, bit cuối cùng là bit com
        m_fan_set_data_hc595(s_fan_runtime.led_stt_data);
        bl_gpio_output_set(LED_COM_PIN, 1);
    }

    tick_10ms++;
    if (tick_10ms >= 100)
    {
        tick_10ms = 0;
        // s_fan_runtime.led_7_seg_data = arr_seg_7[num_curr % 10];
        // num_curr++;
        // s_fan_runtime.led_stt_data = (~(0b1 << (num_curr % 7 + 1))) & 0b11111110;
    }
}

void m_fan_loop(void)
{
}

void m_fan_init(void)
{
    s_fan_runtime.led_stt_data = 0b11111110;
    s_fan_runtime.led_7_seg_data = arr_seg_7[0];
}

m_fan_info_t *m_fan_get_instance(void)
{
    return &s_fan_info;
}