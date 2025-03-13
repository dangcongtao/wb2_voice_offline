#ifndef _M_FAN_EVENT_H_
#define _M_FAN_EVENT_H_
#include <stdint.h>

typedef struct
{
    uint8_t speed_current;
    uint32_t time_current;
    uint8_t limit_low_speed;
    uint8_t limit_high_speed;
} m_fan_natural_mode_t;

uint8_t m_fan_get_current_speed_natural(void);
void m_fan_set_time_off(uint32_t time_set);
uint32_t m_fan_get_time_remain(void);
uint32_t m_fan_get_time_set(void);
void m_fan_timer_toggle(void);
void m_fan_timer_loop(uint32_t delay_ms);

void m_fan_start_natural_mode(void);
void m_fan_toggle_natural_mode(void);

void m_fan_mode_natural_process(uint32_t delay_ms);


#endif // _M_FAN_EVENT_H_