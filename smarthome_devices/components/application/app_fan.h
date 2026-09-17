#ifndef _APP_FAN_H_
#define _APP_FAN_H_

#include <stdint.h>

void app_fan_respond_light(uint8_t state, uint8_t src);
void app_fan_respond_control(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint8_t light, uint32_t uid, uint8_t act_id);
void app_fan_respond_control_voice(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint8_t light, uint32_t uid, uint8_t act_id, uint8_t cmd_id, uint8_t cwsl);
void app_fan_send_temperature(uint32_t temper);
void app_fan_send_battery(uint8_t bat_percent, uint8_t bat_charging_status);
void app_response_time_release(uint32_t uid, uint8_t result);
void app_response_time_release_voice(uint32_t uid, uint8_t result, uint8_t cmd_id, uint8_t cwsl);
uint8_t check_fan_change(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint8_t light);

#endif