#include "m_fan.h"
#include "hardware.h"
#include "m_mqtt.h"
#include "app_fan.h"
#include "m_ft62f28.h"
#include "m_database.h"
#include "m_fan_control.h"
#include "m_fan_event.h"

static m_fan_info_t s_fan_info;
static uint32_t last_tick_blink_led;
extern char respond[512];
extern uint8_t prevent_buzz;
extern uint32_t last_tick_timer_toggle;
extern m_fan_runtime_t s_fan_runtime;

void m_fan_set_auto_mode_settings(uint8_t _auto_enable, uint32_t thresh1, uint32_t thresh2, uint32_t thresh3)
{
    s_fan_info.enable_auto_mode = _auto_enable;
    s_fan_info.thresh_hold_temperature[0] = thresh1;
    s_fan_info.thresh_hold_temperature[1] = thresh2;
    s_fan_info.thresh_hold_temperature[2] = thresh3;
    m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_FAN);
}

void m_fan_set_powersaving(uint8_t is_saving)
{
    s_fan_info.power_save = is_saving;
    m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_FAN);
}

void m_fan_set_light(uint8_t light)
{
    s_fan_info.light = light;
    // hw_led_stt_set_stt(M_FAN_BUTTON_LIGHT_INDEX, light ? LED_INDICATE_ON : LED_INDICATE_OFF);
    m_ft62f28_send_light();
}

void m_fan_toggle_light(void)
{
    s_fan_info.light++;
    if (s_fan_info.light > 3)
    {
        s_fan_info.light = 0;
    }
    m_fan_set_light(s_fan_info.light);
}

void m_fan_set_buzz_disable(uint8_t disable_buzz)
{
    s_fan_info.disable_buzz = disable_buzz;
    m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_FAN);
}

void m_fan_set_pcn(uint8_t pcn_set)
{
    s_fan_info.mode = M_FAN_MODE_PCN;
    s_fan_info.pcn = pcn_set;
    hw_74hc595_set_number(LED_7_SEG_CHARACTER_C_INDEX);
    m_ft62f28_send_speed();
}

static void m_fan_auto_mode_start(void)
{
    s_fan_info.mode = M_FAN_MODE_AUTO;
    if (s_fan_info.speed == M_FAN_SPEED_OFF || s_fan_info.speed > M_FAN_SPEED_3)
    {
        s_fan_info.speed = M_FAN_SPEED_1;
    }
    os_trace("%s  to speed: %u\r\n", __func__, s_fan_info.speed);
    hw_74hc595_set_number(LED_7_SEG_CHARACTER_A_INDEX);
    hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
    if (device_type == FANAC)
    {
        hw_led_stt_set_stt(0, LED_INDICATE_OFF);
        hw_led_stt_set_stt(1, LED_INDICATE_OFF);
        hw_led_stt_set_stt(2, LED_INDICATE_OFF);
    }
    hw_fan_set_speed_level(s_fan_info.speed);
    m_ft62f28_send_speed();
#if FAN_KEEP_OLD_SWING_MODE
    hw_fan_set_swing(s_fan_info.swing);
#endif
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
        hw_74hc595_set_number(s_fan_info.speed);
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
        m_ft62f28_send_speed();
#if FAN_KEEP_OLD_SWING_MODE
        hw_fan_set_swing(s_fan_info.swing);
#endif
    }
    else if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        uint8_t restore_val = prevent_buzz;
        s_fan_info.speed = M_FAN_SPEED_OFF;
        prevent_buzz = 0;
        m_fan_need_buzz(10);
        prevent_buzz = restore_val;
#if FAN_KEEP_OLD_SWING_MODE
        hw_fan_set_swing(M_FAN_SWING_OFF);
#else
        m_fan_set_swing(M_FAN_MODE_OFF);
#endif
        m_fan_set_light(FAN_LIGHT_LEVEL_OFF);
        m_fan_set_time_off(0);
        hw_fan_set_speed_level(M_FAN_SPEED_OFF);
        hw_74hc595_set_number(0);
        last_tick_timer_toggle = sys_get_tick_ms();
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
        hw_led_stt_set_stt(M_FAN_BUTTON_DECREASE_INDEX, LED_INDICATE_OFF);
        hw_led_stt_set_stt(M_FAN_BUTTON_INCREASE_INDEX, LED_INDICATE_OFF);
        m_ft62f28_send_speed();
    }
    else if (s_fan_info.mode == M_FAN_MODE_NATURAL)
    {
        m_fan_start_natural_mode();
    }
    else if (s_fan_info.mode == M_FAN_MODE_AUTO)
    {
        m_fan_auto_mode_start();
    }
}

void m_fan_off(uint8_t src)
{
    m_fan_need_buzz(10);
    m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
    app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, src);
}

void m_fan_control(uint8_t mode_set, uint8_t speed_set, uint8_t pcn_set, uint8_t swing_set, uint8_t lamp_set, uint32_t uid, uint8_t act_id, uint8_t respond)
{
    uint8_t can_set_lamp = 1;
    m_fan_need_buzz(1);
    if (mode_set == M_FAN_MODE_PCN)
    {
        m_fan_set_pcn(pcn_set);
    }
    else if (mode_set == M_FAN_MODE_AUTO)
    {
        m_fan_auto_mode_start();
    }
    else
    {
        if (mode_set == M_FAN_MODE_OFF && s_fan_info.mode != M_FAN_MODE_OFF)
        {
            can_set_lamp = 0;
        }
        m_fan_set_speed(mode_set, speed_set);
    }
    if (can_set_lamp)
    {
        m_fan_set_light(lamp_set);
    }
    m_fan_set_swing(swing_set);
    if (respond)
    {
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, uid, act_id);
    }
}

void m_fan_decrease_speed(void)
{
    if (s_fan_info.mode == M_FAN_MODE_AUTO)
    {
        os_trace("%s from M_FAN_MODE_AUTO to speed 3\r\n", __func__);
        s_fan_info.mode = M_FAN_MODE_ON;
        s_fan_info.speed = M_FAN_SPEED_3;
        hw_fan_set_speed_level(s_fan_info.speed);
        hw_74hc595_set_number(s_fan_info.speed);
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
    }
    else if (s_fan_info.mode == M_FAN_MODE_ON)
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
                m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
                return;
            }
        }
        os_trace("%s M_FAN_MODE_ON speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed_level(s_fan_info.speed);
        hw_74hc595_set_number(s_fan_info.speed);
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
    }
    else if (s_fan_info.mode == M_FAN_MODE_OFF)
    {
        os_trace("%s from M_FAN_MODE_OFF to auto_mode\r\n", __func__);
        m_fan_auto_mode_start();
        hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, LED_INDICATE_OFF);
    }
    else if (s_fan_info.mode == M_FAN_MODE_NATURAL || s_fan_info.mode == M_FAN_MODE_PCN)
    {
        s_fan_info.mode = M_FAN_MODE_ON;
        if (s_fan_info.speed == M_FAN_SPEED_OFF)
        {
            s_fan_info.speed = M_FAN_SPEED_3;
        }
        os_trace("%s M_FAN_MODE_NATURAL speed: %u\r\n", __func__, s_fan_info.speed);
        hw_fan_set_speed_level(s_fan_info.speed);
        hw_74hc595_set_number(s_fan_info.speed);
    }
}

static void m_fan_increase_speed(uint8_t is_off_when_excceed_limit)
{
    if (s_fan_info.mode == M_FAN_MODE_ON)
    {
        s_fan_info.speed = s_fan_info.speed + 1;
        if (s_fan_info.speed > M_FAN_SPEED_3)
        {
            // s_fan_info.speed = M_FAN_SPEED_1;
            m_fan_auto_mode_start();
        }
        os_trace("%s M_FAN_MODE_ON speed: %u\r\n", __func__, s_fan_info.speed);
        m_fan_set_speed(s_fan_info.mode, s_fan_info.speed);
    }
    else if (s_fan_info.mode == M_FAN_MODE_AUTO)
    {
        if (is_off_when_excceed_limit)
        {
            m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
            return;
        }
        else
        {
            s_fan_info.mode = M_FAN_MODE_ON;
            s_fan_info.speed = M_FAN_SPEED_1;
            os_trace("%s from M_FAN_MODE_OFF to speed: %u\r\n", __func__, s_fan_info.speed);
            m_fan_set_speed(s_fan_info.mode, s_fan_info.speed);
        }
    }
    else if (s_fan_info.mode == M_FAN_MODE_OFF || s_fan_info.mode == M_FAN_MODE_NATURAL || s_fan_info.mode == M_FAN_MODE_PCN)
    {
        s_fan_info.mode = M_FAN_MODE_ON;
        if (s_fan_info.speed == M_FAN_SPEED_OFF)
        {
            s_fan_info.speed = M_FAN_SPEED_1;
        }
        os_trace("%s M_FAN_MODE_NATURAL speed: %u\r\n", __func__, s_fan_info.speed);
        m_fan_set_speed(s_fan_info.mode, s_fan_info.speed);
    }
}

void m_fan_set_swing(uint8_t swing)
{
    os_trace("%s swing: %u\r\n", __func__, swing);
    if (!swing || s_fan_info.mode != M_FAN_MODE_OFF)
    {
        m_fan_need_buzz(1);
        s_fan_info.swing = swing;
        hw_fan_set_swing(s_fan_info.swing);
        m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_FAN);
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

void m_fan_process_button_event(uint8_t button_pin)
{
    button_pin = fan_get_func_from_index(button_pin);
    prevent_buzz = 1;
    switch (button_pin)
    {
    case M_FAN_BUTTON_POWER_INDEX:
        os_trace("M_FAN_BUTTON_POWEROFF\r\n");
        m_fan_off(SRC_ACTION_DEVICE);
        break;
    case M_FAN_BUTTON_INCREASE_INDEX:
        os_trace("M_FAN_BUTTON_INCREASE_INDEX\r\n");
        m_fan_increase_speed(true);
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_DEVICE);
        break;

    case M_FAN_BUTTON_DECREASE_INDEX:
        os_trace("M_FAN_BUTTON_DECREASE_INDEX\r\n");
        m_fan_decrease_speed();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_DEVICE);
        break;

    case M_FAN_BUTTON_SWING_INDEX:
        os_trace("M_FAN_BUTTON_SWING_INDEX\r\n");
        m_fan_toogle_swing();
        m_fan_need_buzz(1);
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_DEVICE);
        break;

    case M_FAN_BUTTON_TIMER_INDEX:
        os_trace("M_FAN_BUTTON_TIMER_INDEX\r\n");
        m_fan_timer_toggle();
        break;

    case M_FAN_BUTTON_NATURAL_INDEX:
        os_trace("M_FAN_BUTTON_NATURAL_INDEX\r\n");
        m_fan_toggle_natural_mode();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_DEVICE);
        break;

    case M_FAN_BUTTON_LIGHT_INDEX:
        os_trace("M_FAN_BUTTON_LIGHT_INDEX\r\n");
        m_fan_toggle_light();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_DEVICE);
        // app_fan_respond_light(s_fan_info.light, SRC_ACTION_DEVICE);
        m_fan_need_buzz(1);
        break;

    default:
        break;
    }
    prevent_buzz = 0;
}

void m_fan_process_remote_event(uint32_t button_data)
{
    os_trace("%s %X\r\n", __func__, button_data);

    switch (button_data)
    {
    case REMOTE_IR_DATA_POWEROFF:
        os_trace("REMOTE_IR_DATA_POWEROFF\r\n");
        if (s_fan_info.mode == M_FAN_MODE_OFF)
        {
            m_fan_set_speed(M_FAN_MODE_ON, M_FAN_SPEED_1);
            m_fan_need_buzz(1);
        }
        else
        {
            m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
            m_fan_need_buzz(10);
        }
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_REMOTE);
        break;

    case REMOTE_IR_DATA_INCREASE:
        os_trace("REMOTE_IR_DATA_INCREASE\r\n");
        m_fan_increase_speed(false);
        m_fan_need_buzz(1);
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_REMOTE);
        break;

        // case REMOTE_IR_DATA_DECREASE:
        //     os_trace("REMOTE_IR_DATA_DECREASE\r\n");
        //     m_fan_decrease_speed();
        //     m_fan_need_buzz(1);
        //     app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_REMOTE);
        //     break;

    case REMOTE_IR_DATA_SWING:
        os_trace("REMOTE_IR_DATA_SWING\r\n");
        m_fan_toogle_swing();
        m_fan_need_buzz(1);
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_REMOTE);
        break;

    case REMOTE_IR_DATA_TIMER:
        os_trace("REMOTE_IR_DATA_TIMER\r\n");
        m_fan_timer_toggle();
        break;

    case REMOTE_IR_DATA_NATURAL:
        os_trace("REMOTE_IR_DATA_NATURAL\r\n");
        m_fan_toggle_natural_mode();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_REMOTE);
        break;

    case REMOTE_IR_DATA_LIGHT:
        os_trace("REMOTE_IR_DATA_LIGHT\r\n");
        m_fan_toggle_light();
        app_fan_respond_control(s_fan_info.mode, s_fan_info.speed, s_fan_info.pcn, s_fan_info.swing, s_fan_info.light, 0, SRC_ACTION_REMOTE);
        // app_fan_respond_light(s_fan_info.light, SRC_ACTION_REMOTE);
        m_fan_need_buzz(1);
        break;

    default:
        os_trace("INVALID REMOTE BUTTON\r\n");
        break;
    }
}

extern uint8_t bat_is_low(void);

void m_led_number_show(void)
{
    if (s_fan_info.mode == M_FAN_MODE_NATURAL)
    {
        hw_74hc595_set_number(LED_7_SEG_CHARACTER_E_INDEX);
    }
    else if (s_fan_info.mode == M_FAN_MODE_PCN)
    {
        hw_74hc595_set_number(LED_7_SEG_CHARACTER_C_INDEX);
    }
    else if (s_fan_info.mode == M_FAN_MODE_AUTO)
    {
        hw_74hc595_set_number(LED_7_SEG_CHARACTER_A_INDEX);
    }
    else if (s_fan_info.speed)
    {
        hw_74hc595_set_number(s_fan_info.speed);
    }
    else
    {
        hw_74hc595_set_number(LED_7_SEG_OFF_INDEX);
    }
}

static void m_fan_led_proccess_when_active(void)
{
    static uint8_t led_val = 0;

    led_val = !led_val;

    if (s_fan_info.mode == M_FAN_MODE_NATURAL)
    {
        if (hw_btn_read(M_FAN_BUTTON_NATURAL_INDEX) == BTN_IDLE_LEVEL)
        {
            hw_led_stt_set_stt(M_FAN_BUTTON_NATURAL_INDEX, led_val);
            if (device_type == FANAC)
            {
                hw_led_stt_set_stt(0, LED_INDICATE_OFF);
                hw_led_stt_set_stt(1, LED_INDICATE_OFF);
                hw_led_stt_set_stt(2, LED_INDICATE_OFF);
            }
        }
    }

    if (s_fan_info.swing && s_fan_runtime.swing_mode)
    {
        if (hw_btn_read(M_FAN_BUTTON_SWING_INDEX) == BTN_IDLE_LEVEL && device_type != FANAC)
        {
            hw_led_stt_set_stt(M_FAN_BUTTON_SWING_INDEX, led_val);
        }
    }

    if (s_fan_info.time_set && !last_tick_timer_toggle)
    {
        if (hw_btn_read(M_FAN_BUTTON_TIMER_INDEX) == BTN_IDLE_LEVEL || device_type == FANAC)
        {
            hw_led_stt_set_stt(M_FAN_BUTTON_TIMER_INDEX, led_val);
        }
    }

    if (device_type == FANAC)
    {
        if (s_fan_info.mode == M_FAN_MODE_ON)
        {
            if (s_fan_info.speed == M_FAN_SPEED_1)
            {
                hw_led_stt_set_stt(0, led_val);
                hw_led_stt_set_stt(1, LED_INDICATE_OFF);
                hw_led_stt_set_stt(2, LED_INDICATE_OFF);
            }
            else if (s_fan_info.speed == M_FAN_SPEED_2)
            {
                hw_led_stt_set_stt(1, led_val);
                hw_led_stt_set_stt(0, LED_INDICATE_OFF);
                hw_led_stt_set_stt(2, LED_INDICATE_OFF);
            }
            else if (s_fan_info.speed == M_FAN_SPEED_3)
            {
                hw_led_stt_set_stt(2, led_val);
                hw_led_stt_set_stt(0, LED_INDICATE_OFF);
                hw_led_stt_set_stt(1, LED_INDICATE_OFF);
            }
        }
    }

    if (device_type == FANDC)
    {
        if (bat_is_low() && !last_tick_timer_toggle)
        {
            if (led_val)
            {
                m_led_number_show();
            }
            else
            {
                hw_74hc595_set_number(LED_7_SEG_OFF_INDEX);
            }
        }
    }
}

static void m_fan_check_flag(void)
{
    hw_74hc595_loop();

    if (last_tick_timer_toggle && sys_get_tick_ms() - last_tick_timer_toggle >= TIMEOUT_DISPLAY_TIME_OFF_MS)
    {
        last_tick_timer_toggle = 0;

        m_led_number_show();
    }
}

void m_led_7_seg_loop(uint32_t delay_ms)
{
    if (sys_get_tick_ms() - last_tick_blink_led > 1000)
    {
        last_tick_blink_led = sys_get_tick_ms();
        m_fan_led_proccess_when_active();
        m_fan_mode_natural_process(1000);
        m_fan_mode_auto_handle(1000);
        m_fan_timer_loop(1000);
    }
    m_fan_check_flag();
}

uint8_t last_reason_reset;
void m_fan_init(void)
{
    hw_buzz_init();

    uint8_t reason_reset = flash_read_rs_reason();
    os_trace("reason_reset: %u\r\n", reason_reset);

    if (reason_reset)
    {
        last_reason_reset = reason_reset;
        flash_set_rs_reason(0);
    }
    else
    {
        s_fan_info.mode = M_FAN_MODE_OFF;
        s_fan_info.speed = M_FAN_SPEED_OFF;
        s_fan_info.pcn = M_FAN_SPEED_OFF;
        s_fan_info.light = FAN_LIGHT_LEVEL_OFF;
        s_fan_info.time_set = 0;
    }
    last_tick_timer_toggle = 1;
    // led_stt_data = 0b00000000;
}

m_fan_info_t *m_fan_get_instance(void)
{
    return &s_fan_info;
}