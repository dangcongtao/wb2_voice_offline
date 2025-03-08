#ifndef _APP_FAN_H_
#define _APP_FAN_H_

#include <stdint.h>

void app_fan_respond_light(uint8_t state, uint8_t src);
void app_fan_respond_control(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint32_t uid, uint8_t act_id);

#endif