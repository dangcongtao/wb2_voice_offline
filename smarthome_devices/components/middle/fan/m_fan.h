#ifndef _M_FAN_H_
#define _M_FAN_H_
#include <stdint.h>

typedef struct
{
    uint8_t mode;
    uint8_t speed;
    uint8_t pcn;
    uint8_t swing;
    uint8_t light;
    uint8_t disable_buzz;
} m_fan_info_t;

void m_fan_set_swing(uint8_t swing);
void m_fan_toogle_swing(void);

void m_fan_set_speed(uint8_t mode_set, uint8_t speed_set);
void m_fan_set_pcn(uint8_t pcn_set);
void m_fan_increase_speed(void);

void m_fan_set_light(uint8_t light);
void m_fan_toggle_light(void);

void m_fan_set_buzz_disable(uint8_t disable_buzz);
void m_buzz_play(void);

void m_fan_off(uint8_t src);

void m_fan_control(uint8_t mode_set, uint8_t speed_set, uint8_t pcn_set, uint8_t swing_set, uint32_t uid, uint8_t act_id, uint8_t respond);
void m_fan_set_time_off(uint32_t time_set);
void m_fan_toggle_natural_mode(void);

uint32_t m_fan_get_time_remain(void);
uint32_t m_fan_get_time_set(void);
uint8_t m_fan_get_current_speed_natural(void);

void m_fan_process_button_event(uint8_t button_pin);

void m_led_7_seg_loop(uint32_t delay_ms);
void m_fan_init(void);
m_fan_info_t *m_fan_get_instance(void);

#endif /* _M_FAN_H_ */