#include "m_fan.h"
#include "hardware.h"
#include "m_mqtt.h"
#include "app_fan.h"
#include "m_ft62f28.h"

static m_fan_runtime_t s_fan_runtime;
static m_fan_info_t s_fan_info;
extern char respond[512];
extern uint8_t need_buzz_off;

#define LED_7_SEG_OFF_INDEX 10

static const uint8_t arr_seg_7[] = {
    0b10000000, // 0 (0x40) - Các thanh 0,1,2,3,4,5 sáng (bit 0), thanh 6 tắt (bit 1)
    0b11110010, // 1 (0x79) - Thanh 1,2 sáng
    0b01001000, // 2 (0x24) - Thanh 0,1,6,4,3 sáng
    0b01100000, // 3 (0x30) - Thanh 0,1,6,2,3 sáng
    0b00110010, // 4 (0x19) - Thanh 1,2,5,6 sáng
    0b00100100, // 5 (0x12) - Thanh 0,6,2,3,5 sáng
    0b00000100, // 6 (0x02) - Thanh 0,6,2,3,4,5 sáng
    0b11110000, // 7 (0x78) - Thanh 0,1,2 sáng
    0b00000000, // 8 (0x00) - Tất cả sáng
    0b00100000, // 9 (0x10) - Thanh 0,1,2,3,5,6 sáng
    0b11111110, // 10 (0x00) - Tắt hết
};

void m_fan_set_number_led(uint8_t num)
{
    s_fan_runtime.led_7_seg_data &= 0x01;
    s_fan_runtime.led_7_seg_data |= arr_seg_7[num];
    s_fan_runtime.need_send_hc595 = 1;
}

void hw_led_stt_set_stt(uint8_t index, uint8_t val)
{
    m_ft62f28_send_led();
    s_fan_runtime.led_stt_data &= ~(1 << index);
    s_fan_runtime.led_stt_data |= (val << index);
}

void hw_led_stt_toggle_stt(uint8_t index)
{
    uint8_t current_val = (s_fan_runtime.led_stt_data >> index) & 1;
    hw_led_stt_set_stt(index, !current_val);
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
    switch (speed)
    {
    case M_FAN_SPEED_OFF:
        m_fan_set_number_led(LED_7_SEG_OFF_INDEX);
        hw_led_stt_set_stt(M_FAN_BUTTON_DECREASE_INDEX, LED_INDICATE_OFF);
        hw_led_stt_set_stt(M_FAN_BUTTON_INCREASE_INDEX, LED_INDICATE_OFF);
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
        m_ft62f28_send_speed();
        break;
    case M_FAN_SPEED_1:
        s_fan_runtime.switch_data_output &= 0b10010;
        s_fan_runtime.switch_data_output |= 0b10;
        // hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_INDICATE_ON);
        m_ft62f28_send_speed();
        break;
    case M_FAN_SPEED_2:
        s_fan_runtime.switch_data_output &= 0b10100;
        s_fan_runtime.switch_data_output |= 0b100;
        // hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_INDICATE_ON);
        m_ft62f28_send_speed();
        break;
    case M_FAN_SPEED_3:
        s_fan_runtime.switch_data_output &= 0b11000;
        s_fan_runtime.switch_data_output |= 0b1000;
        // hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_INDICATE_ON);
        m_ft62f28_send_speed();
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
        hw_led_stt_set_stt(M_FAN_BUTTON_SWING_INDEX, LED_INDICATE_ON);
    }
    else
    {
        s_fan_runtime.switch_data_output &= 0b01111;
        hw_led_stt_set_stt(M_FAN_BUTTON_SWING_INDEX, LED_INDICATE_OFF);
    }
    m_ft62f28_send_swing();
}

void hw_fan_set_light(uint8_t light)
{
    s_fan_info.light = light;
    hw_led_stt_set_stt(M_FAN_BUTTON_LIGHT_INDEX, light ? LED_INDICATE_ON : LED_INDICATE_OFF);
    m_ft62f28_send_light();
}

void hw_set_buzz(uint8_t buzz_set)
{
    s_fan_runtime.led_7_seg_data &= 0xFE;
    s_fan_runtime.led_7_seg_data |= (buzz_set ? 1 : 0);
    s_fan_runtime.need_send_hc595 = 1;
}

void m_fan_toggle_light(void)
{
    s_fan_info.light = !s_fan_info.light;
    hw_fan_set_light(s_fan_info.light);
}

void m_fan_set_time_off(uint32_t time_set)
{
    os_trace("%s time_set: %u\r\n", __func__, time_set);
    s_fan_runtime.time_remain = time_set * 1000;
    s_fan_runtime.time_set = time_set * 1000;
    if (s_fan_runtime.time_remain > 0)
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
    return s_fan_runtime.time_remain / 1000;
}

uint32_t m_fan_get_time_set(void)
{
    return s_fan_runtime.time_set / 1000;
}

static void m_fan_start_natural_mode(void)
{
    s_fan_runtime.natural_mode.speed_current = FAN_SPEED_LEVEL_1;
    m_ft62f28_send_speed();
    s_fan_runtime.natural_mode.time_current = 0;
}

void m_fan_set_buzz_enable(uint8_t enable_buzz)
{
    s_fan_info.buzz = enable_buzz;
}

void m_fan_need_buzz(void)
{
    if (s_fan_info.buzz)
    {
        need_buzz_off = 1;
    }
}

void m_fan_set_speed(uint8_t mode_set, uint8_t speed_set)
{
    os_trace("%s mode: %u, speed: %u\r\n", __func__, mode_set, speed_set);
    if (mode_set != 0xFF)
    {
        s_fan_info.mode = mode_set;
    }
    if (speed_set != 0xFF)
    {
        s_fan_info.speed = speed_set;
    }
    if (s_fan_info.mode == M_FAN_MODE_ON)
    {
        if (s_fan_info.speed == M_FAN_SPEED_OFF)
        {
            s_fan_info.speed = M_FAN_SPEED_1;
        }
        hw_fan_set_speed_level(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
        m_ft62f28_send_speed();
    }
    else if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        hw_fan_set_speed_level(M_FAN_SPEED_OFF);
        m_fan_set_time_off(0);
        hw_fan_set_swing(M_FAN_MODE_OFF);
        m_fan_set_number_led(LED_7_SEG_OFF_INDEX);
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
        hw_led_stt_set_stt(M_FAN_BUTTON_DECREASE_INDEX, LED_INDICATE_OFF);
        hw_led_stt_set_stt(M_FAN_BUTTON_INCREASE_INDEX, LED_INDICATE_OFF);
        hw_led_stt_set_stt(M_FAN_BUTTON_SWING_INDEX, LED_INDICATE_OFF);
        m_ft62f28_send_speed();
    }
    else if (s_fan_info.mode == M_FAN_MODE_NATURAL)
    {
        // if (s_fan_info.speed == M_FAN_MODE_OFF)
        // {
        //     s_fan_info.speed = M_FAN_SPEED_1;
        // }
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_ON);
        // hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_INDICATE_ON);
        m_fan_start_natural_mode();
        m_ft62f28_send_speed();
    }
}

void m_fan_off(uint8_t src)
{
    m_fan_need_buzz();
    m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
    app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_DEVICE);
}

void m_fan_control(uint8_t mode_set, uint8_t speed_set, uint8_t swing_set, uint32_t uid, uint8_t act_id, uint8_t respond)
{
    m_fan_need_buzz();
    hw_fan_set_swing(swing_set);
    m_fan_set_speed(mode_set, speed_set);
    if (respond)
    {
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, uid, act_id);
    }
}

void m_fan_decrease_speed(void)
{
    if (s_fan_info.mode == M_FAN_MODE_ON)
    {
        if (s_fan_info.speed == M_FAN_SPEED_OFF)
        {
            s_fan_info.speed = M_FAN_SPEED_3;
        }
        else
        {
            s_fan_info.speed = s_fan_info.speed - 1;
            if (s_fan_info.speed == M_FAN_SPEED_OFF)
            {
                s_fan_info.speed = M_FAN_SPEED_3;
            }
        }
        os_trace("%s M_FAN_MODE_ON speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed_level(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
    }
    else if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        s_fan_info.mode = M_FAN_MODE_ON;
        s_fan_info.speed = M_FAN_SPEED_3;
        os_trace("%s from M_FAN_MODE_OFF to speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed_level(s_fan_info.speed);
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
        hw_fan_set_speed_level(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
    }
}

void m_fan_increase_speed(void)
{
    if (s_fan_info.mode == M_FAN_MODE_ON)
    {
        s_fan_info.speed = s_fan_info.speed + 1;
        if (s_fan_info.speed > 3)
        {
            s_fan_info.speed = 1;
        }
        os_trace("%s M_FAN_MODE_ON speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed_level(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
    }
    else if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        s_fan_info.mode = M_FAN_MODE_ON;
        s_fan_info.speed = M_FAN_SPEED_1;
        os_trace("%s from M_FAN_MODE_OFF to speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed_level(s_fan_info.speed);
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
        hw_fan_set_speed_level(s_fan_info.speed);
        m_fan_set_number_led(s_fan_info.speed);
    }
}

void m_fan_set_swing(uint8_t swing)
{
    os_trace("%s swing: %u\r\n", __func__, swing);
    if (s_fan_info.mode != M_FAN_MODE_OFF)
    {
        m_fan_need_buzz();
        s_fan_info.swing = swing;
        hw_fan_set_swing(s_fan_info.swing);
    }
    else
    {
        os_trace("%s cant swing when fan is off\r\n", __func__);
    }
}

void m_fan_toogle_swing(void)
{
    uint8_t swing_set = !s_fan_info.swing;
    m_fan_set_swing(swing_set);
}

void m_fan_toggle_natural_mode(void)
{
    if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        s_fan_info.mode = M_FAN_MODE_NATURAL;
        m_fan_start_natural_mode();
        // s_fan_info.speed = M_FAN_SPEED_1;
        os_trace("%s mode_natural: %u\r\n", __func__, s_fan_info.mode);
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_ON);
        // hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_INDICATE_ON);
    }
    else
    {
        if (s_fan_info.mode == M_FAN_MODE_NATURAL)
        {
            s_fan_info.mode = M_FAN_MODE_ON;
            m_fan_set_speed(s_fan_info.mode, s_fan_info.speed);
            hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
        }
        else
        {
            s_fan_info.mode = M_FAN_MODE_NATURAL;
            m_fan_start_natural_mode();
            os_trace("%s mode_natural: %u\r\n", __func__, s_fan_info.mode);
            hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_ON);
            // hw_led_stt_set_stt(LED_SPEED_NORMAL_IDX, LED_INDICATE_ON);
        }
    }
    m_fan_need_buzz();
}

void m_fan_timer_toggle(void)
{
    if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        os_trace("can't set time cos fan off\r\n");
        return;
    }
    m_fan_need_buzz();
    uint32_t time_hours = s_fan_runtime.time_set / (60 * 60 * 1000);
    time_hours++;
    if (time_hours > 9)
    {
        time_hours = 0;
    }
    m_fan_set_time_off(time_hours * 60 * 60);
    s_fan_runtime.last_tick_timer_toggle = sys_get_tick_ms();
    os_trace("%s hours: %u\r\n", __func__, time_hours);
    m_fan_set_number_led(time_hours);
}

static void m_fan_timer_loop(uint32_t delay_ms)
{
    if (s_fan_runtime.time_remain > 0)
    {
        s_fan_runtime.time_remain -= delay_ms;
        if (s_fan_runtime.time_remain == 0)
        {
            hw_led_stt_set_stt(M_FAN_BUTTON_TIMER_INDEX, LED_INDICATE_OFF);
            m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
        }
    }
}

static void m_fan_set_data_hc595(uint8_t data)
{
    hw_74hc595_write_byte(data, 1);
}

static void m_fan_mode_natural_process(uint32_t delay_ms)
{
    if (s_fan_info.mode == M_FAN_MODE_NATURAL)
    {
        s_fan_runtime.natural_mode.time_current += delay_ms;
        if (s_fan_runtime.natural_mode.time_current % 1000 == 0)
        {
            if (hw_btn_read(M_FAN_BUTTON_NATURAL_INDEX) == BTN_IDLE_LEVEL)
            {
                hw_led_stt_toggle_stt(M_FAN_BUTTON_NATURAL_INDEX);
            }
        }
        if (s_fan_runtime.natural_mode.time_current >= TIME_STEP_FAN_NATURAL_MODE_MS)
        {
            s_fan_runtime.natural_mode.time_current = 0;
            s_fan_runtime.natural_mode.speed_current -= 10;
            if (s_fan_runtime.natural_mode.speed_current == 0 || s_fan_runtime.natural_mode.speed_current > 100)
            {
                s_fan_runtime.natural_mode.speed_current = 100;
            }
            /* if (s_fan_runtime.natural_mode.speed_current == M_FAN_SPEED_1)
            {
                s_fan_runtime.natural_mode.speed_current = M_FAN_SPEED_3;
            }
            else if (s_fan_runtime.natural_mode.speed_current == M_FAN_SPEED_2)
            {
                s_fan_runtime.natural_mode.speed_current = M_FAN_SPEED_1;
            }
            else
            {
                s_fan_runtime.natural_mode.speed_current = M_FAN_SPEED_2;
            } */
            hw_fan_set_speed_pcn(s_fan_runtime.natural_mode.speed_current);
            os_trace("m_fan_mode_natural_process %u\r\n", s_fan_runtime.natural_mode.speed_current);
        }
    }
}

void m_fan_process_button_event(uint8_t button_pin)
{
    switch (button_pin)
    {
    case M_FAN_BUTTON_INCREASE_INDEX:
        os_trace("M_FAN_BUTTON_INCREASE_INDEX\r\n");
        m_fan_increase_speed();
        m_fan_need_buzz();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_DEVICE);
        break;

    case M_FAN_BUTTON_DECREASE_INDEX:
        os_trace("M_FAN_BUTTON_DECREASE_INDEX\r\n");
        m_fan_decrease_speed();
        m_fan_need_buzz();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_DEVICE);
        break;

    case M_FAN_BUTTON_SWING_INDEX:
        os_trace("M_FAN_BUTTON_SWING_INDEX\r\n");
        m_fan_toogle_swing();
        m_fan_need_buzz();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_DEVICE);
        break;

    case M_FAN_BUTTON_TIMER_INDEX:
        os_trace("M_FAN_BUTTON_TIMER_INDEX\r\n");
        m_fan_timer_toggle();
        break;

    case M_FAN_BUTTON_NATURAL_INDEX:
        os_trace("M_FAN_BUTTON_NATURAL_INDEX\r\n");
        m_fan_toggle_natural_mode();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_DEVICE);
        break;

    case M_FAN_BUTTON_LIGHT_INDEX:
        os_trace("M_FAN_BUTTON_LIGHT_INDEX\r\n");
        m_fan_toggle_light();
        app_fan_respond_light(s_fan_info.light, SRC_ACTION_DEVICE);
        m_fan_need_buzz();
        break;

    default:
        break;
    }
}

#define BUTTON_DATA_DECREASE    0x1FE807F
#define BUTTON_DATA_INCREASE    0x1FEC03F
#define BUTTON_DATA_POWEROFF    0x1FE58A7
#define BUTTON_DATA_SWING       0x1FE20DF
#define BUTTON_DATA_TIMER       0x1FE609F
#define BUTTON_DATA_NATURAL     0x1FEE01F
#define BUTTON_DATA_LIGHT   0x1FE906F

void m_fan_process_remote_event(uint32_t button_data)
{
    os_trace("%s %X\r\n", __func__, button_data);
    switch (button_data)
    {
    case BUTTON_DATA_POWEROFF:
        os_trace("BUTTON_DATA_POWEROFF\r\n");
        m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
        m_fan_need_buzz();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_REMOTE);
        break;
        
    case BUTTON_DATA_INCREASE:
        os_trace("BUTTON_DATA_INCREASE\r\n");
        m_fan_increase_speed();
        m_fan_need_buzz();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_REMOTE);
        break;

    case BUTTON_DATA_DECREASE:
        os_trace("BUTTON_DATA_DECREASE\r\n");
        m_fan_decrease_speed();
        m_fan_need_buzz();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_REMOTE);
        break;

    case BUTTON_DATA_SWING:
        os_trace("BUTTON_DATA_SWING\r\n");
        m_fan_toogle_swing();
        m_fan_need_buzz();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_REMOTE);
        break;

    case BUTTON_DATA_TIMER:
        os_trace("BUTTON_DATA_TIMER\r\n");
        m_fan_timer_toggle();
        break;

    case BUTTON_DATA_NATURAL:
        os_trace("BUTTON_DATA_NATURAL\r\n");
        m_fan_toggle_natural_mode();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.swing, 0, SRC_ACTION_REMOTE);
        break;

    case BUTTON_DATA_LIGHT:
        os_trace("BUTTON_DATA_LIGHT\r\n");
        m_fan_toggle_light();
        app_fan_respond_light(s_fan_info.light, SRC_ACTION_REMOTE);
        m_fan_need_buzz();
        break;

    default:
        break;
    }
}

static void m_fan_check_flag(void)
{
    if (s_fan_runtime.need_send_hc595 == 1)
    {
        s_fan_runtime.need_send_hc595 = 0;
        m_fan_set_data_hc595(s_fan_runtime.led_7_seg_data);
    }
    if (s_fan_runtime.last_tick_timer_toggle && sys_get_tick_ms() - s_fan_runtime.last_tick_timer_toggle >= TIMEOUT_DISPLAY_TIME_OFF_MS)
    {
        s_fan_runtime.last_tick_timer_toggle = 0;
        m_fan_set_number_led(s_fan_info.speed);
    }
}

void m_led_7_seg_loop(uint32_t delay_ms)
{
    m_fan_mode_natural_process(delay_ms);
    m_fan_timer_loop(delay_ms);
    m_fan_check_flag();
}

void m_fan_init(void)
{
    hw_fan_init();
    s_fan_runtime.led_stt_data = 0b00000000;
    s_fan_runtime.led_7_seg_data = arr_seg_7[0];
}

uint8_t m_fan_get_led_stt(void)
{
    return s_fan_runtime.led_stt_data;
}

uint8_t m_fan_get_current_speed_natural(void)
{
    return s_fan_runtime.natural_mode.speed_current;
}

m_fan_info_t *m_fan_get_instance(void)
{
    return &s_fan_info;
}