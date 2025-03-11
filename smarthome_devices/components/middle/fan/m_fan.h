#ifndef _M_FAN_H_
#define _M_FAN_H_
#include <stdint.h>

#define TIME_STEP_FAN_NATURAL_MODE_DC_MS  1000
#define TIME_STEP_FAN_NATURAL_MODE_AC_MS  4000

#define M_FAN_BUTTON_DECREASE_INDEX 0
#define M_FAN_BUTTON_INCREASE_INDEX 1
#define M_FAN_BUTTON_SWING_INDEX    2
#define M_FAN_BUTTON_TIMER_INDEX    3
#define M_FAN_BUTTON_NATURAL_INDEX  4
#define M_FAN_BUTTON_LIGHT_INDEX    5
#define M_FAN_BUTTON_POWER_INDEX    6

#define TIMEOUT_DISPLAY_TIME_OFF_MS  3000

#define FAN_SPEED_LEVEL_1   50
#define FAN_SPEED_LEVEL_2   75
#define FAN_SPEED_LEVEL_3   100


#define FAN_LIGHT_LEVEL_OFF 0
#define FAN_LIGHT_LEVEL_1   1
#define FAN_LIGHT_LEVEL_2   2
#define FAN_LIGHT_LEVEL_3   3

#define REMOTE_IR_DATA_DECREASE    0x1FE807F
#define REMOTE_IR_DATA_INCREASE    0x1FEC03F
#define REMOTE_IR_DATA_POWEROFF    0x1FE58A7
#define REMOTE_IR_DATA_SWING       0x1FE20DF
#define REMOTE_IR_DATA_TIMER       0x1FE609F
#define REMOTE_IR_DATA_NATURAL     0x1FEE01F
#define REMOTE_IR_DATA_LIGHT       0x1FE906F

typedef struct
{
    uint8_t speed_current;
    uint32_t time_current;
    uint8_t limit_low_speed;
    uint8_t limit_high_speed;
} m_fan_natural_mode_t;

typedef struct
{
    uint32_t time_set;
    uint32_t time_remain;
    uint8_t need_send_hc595;
    uint8_t led_7_seg_data;
    uint8_t led_stt_data;
    uint8_t switch_data_output;
    uint32_t last_tick_timer_toggle;
    uint32_t last_tick_blink_led;

    m_fan_natural_mode_t natural_mode;

} m_fan_runtime_t;

typedef struct
{
    uint8_t mode;
    uint8_t speed;
    uint8_t pcn;
    uint8_t swing;
    uint8_t buzz;
    uint8_t light;
} m_fan_info_t;

void m_fan_set_swing(uint8_t swing);
void m_fan_toogle_swing(void);

void m_fan_set_speed(uint8_t mode_set, uint8_t speed_set);
void m_fan_set_pcn(uint8_t pcn_set);
void m_fan_increase_speed(void);

void m_fan_set_light(uint8_t light);
void m_fan_toggle_light(void);

void m_fan_set_buzz_enable(uint8_t enable_buzz);

void m_fan_off(uint8_t src);

void m_fan_control(uint8_t mode_set, uint8_t speed_set, uint8_t pcn_set, uint8_t swing_set, uint32_t uid, uint8_t act_id, uint8_t respond);
void m_fan_set_time_off(uint32_t time_set);
void m_fan_toggle_natural_mode(void);
void hw_set_buzz(uint8_t buzz_set);

uint8_t m_fan_get_led_stt(void);
uint32_t m_fan_get_time_remain(void);
uint32_t m_fan_get_time_set(void);
uint8_t m_fan_get_current_speed_natural(void);

void m_fan_process_button_event(uint8_t button_pin);

void m_led_7_seg_loop(uint32_t delay_ms);
void m_fan_init(void);
m_fan_info_t *m_fan_get_instance(void);

#endif /* _M_FAN_H_ */