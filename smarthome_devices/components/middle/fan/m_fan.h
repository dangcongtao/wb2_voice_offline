#ifndef _M_FAN_H_
#define _M_FAN_H_
#include <stdint.h>

typedef struct
{
    uint8_t speed;
    uint8_t swing;
} m_fan_runtime_t;

typedef struct
{
    /* data */
} m_fan_info_t;

void m_fan_toggle_speed(void);
void m_fan_toogle_swing(void);

#endif /* _M_FAN_H_ */