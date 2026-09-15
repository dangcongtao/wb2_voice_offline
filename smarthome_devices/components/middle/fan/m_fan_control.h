#ifndef _M_FAN_CONTROL_H_
#define _M_FAN_CONTROL_H_
#include <stdint.h>

#define TIME_STEP_FAN_NATURAL_MODE_DC_MS 1000
#define TIME_STEP_FAN_NATURAL_MODE_AC_MS 4000

#define M_FAN_BUTTON_DECREASE_INDEX 0
#define M_FAN_BUTTON_INCREASE_INDEX 1
#define M_FAN_BUTTON_SWING_INDEX 2
#define M_FAN_BUTTON_TIMER_INDEX 3
#define M_FAN_BUTTON_NATURAL_INDEX 4
#define M_FAN_BUTTON_LIGHT_INDEX 5
#define M_FAN_BUTTON_POWER_INDEX 6

#define M_FAN_BUTTON_INVALID 8

#define TIMEOUT_DISPLAY_TIME_OFF_MS 3000

#define FAN_SPEED_LEVEL_1 60
#define FAN_SPEED_LEVEL_2 80
#define FAN_SPEED_LEVEL_3 100

#define FAN_LIGHT_LEVEL_OFF 0
#define FAN_LIGHT_LEVEL_1 1
#define FAN_LIGHT_LEVEL_2 2
#define FAN_LIGHT_LEVEL_3 3

#define M_FAN_SWING_OFF 0
#define M_FAN_SWING_ON 1

#define REMOTE_IR_DATA_POWEROFF 0xFFA25D
// #define REMOTE_IR_DATA_DECREASE 0x1FE807F
#define REMOTE_IR_DATA_INCREASE 0xFFE21D
#define REMOTE_IR_DATA_SWING 0xFF30CF
#define REMOTE_IR_DATA_TIMER 0xFF906F
#define REMOTE_IR_DATA_NATURAL 0xFFE01F
#define REMOTE_IR_DATA_LIGHT 0xFF7A85

typedef struct
{

    uint8_t led_stt_data;
    uint8_t swing_mode;
    uint32_t last_tick_blink_led;
} m_fan_runtime_t;

uint8_t fan_get_func_from_index(uint8_t my_func);
uint8_t fan_get_index_from_func(uint8_t my_func);
void hw_led_stt_set_stt(uint8_t index, uint8_t val);
void hw_led_stt_toggle_stt(uint8_t index);
void hw_fan_set_speed_pcn(uint8_t speed);
void hw_fan_set_speed_level(uint8_t speed);
void hw_fan_set_swing(uint8_t swing);
void m_fan_need_buzz(uint8_t num_buzz);
uint8_t m_fan_get_led_stt(void);
extern volatile uint8_t prevent_buzz;
extern uint8_t buzz_num;
void buzz_loop(void);

#endif // _M_FAN_CONTROL_H_