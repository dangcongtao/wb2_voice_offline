#ifndef _M_FAN_H_
#define _M_FAN_H_
#include <stdint.h>

#define LED_STT_ORDER               0
#define LED_STT_FAN_ON              1
#define LED_STT_FAN_SWING           2
#define LED_STT_FAN_MODE_NATURAL    3
typedef struct
{
    uint32_t time_set;
    uint32_t time_remain;
    uint8_t step;
    uint8_t led_7_seg_data;
    uint8_t led_stt_data;
} m_fan_runtime_t;

typedef struct
{
    uint8_t mode;
    uint8_t speed;
    uint8_t swing;
    uint8_t buzz;
} m_fan_info_t;

void m_fan_set_swing(uint8_t swing);
void m_fan_set_speed(uint8_t mode_set, uint8_t speed_set);
void m_fan_set_time_off(uint32_t time_set);
void m_fan_toggle_speed(void);
void m_fan_toogle_swing(void);
uint32_t m_fan_get_time_remain(void);
void m_led_7_seg_loop(void);
m_fan_info_t *m_fan_get_instance(void);

#endif /* _M_FAN_H_ */