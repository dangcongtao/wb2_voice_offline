#include "app_config.h"
#include "m_mqtt.h"
#include "m_ota.h"
#include "system_typedef.h"
#include "process_data.h"

extern char respond[512];

uint8_t old_mode = M_FAN_MODE_OFF;
uint8_t old_speed = M_FAN_SPEED_OFF;
uint8_t old_pcn = M_FAN_SPEED_OFF;
uint8_t old_swing = 0;
uint8_t old_light = 0;

uint8_t check_fan_change(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint8_t light)
{
    uint8_t change = 0;

    if (old_mode != mode)
    {
        old_mode = mode;
        change |= (1 << 0);
    }

    if (old_speed != speed)
    {
        old_speed = speed;
        change |= (1 << 1);
    }

    if (old_pcn != pcn)
    {
        old_pcn = pcn;
        change |= (1 << 2);
    }

    if (old_swing != swing)
    {
        old_swing = swing;
        change |= (1 << 3);
    }

    if (old_light != light)
    {
        old_light = light;
        change |= (1 << 4);
    }

    return change;
}

void app_fan_respond_light(uint8_t state, uint8_t src)
{
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
    {
        memset(respond, 0, sizeof(respond));
        sprintf(respond, "{\"%s\":25,\"lamp\":%u,\"act_id\":%u}", gs_dev_type[device_type], state, src);
        app_send_data(respond, strlen(respond));
    }
}

void app_fan_respond_control(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint8_t light, uint32_t uid, uint8_t act_id)
{
    uint8_t change = check_fan_change(mode, speed, pcn, swing, light);
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
    {
        memset(respond, 0, sizeof(respond));
        sprintf(respond, "{\"%s\":20,\"mode\":%u,\"lv\":%u,\"swing\":%u,\"pcn\":%u,\"lamp\":%u,\"u\":%lu,\"act_id\":%u,\"change\":%u}", 
            gs_dev_type[device_type], mode, speed, swing, pcn, light, uid, act_id, change);
        app_send_data(respond, strlen(respond));
    }
}

void app_fan_respond_control_voice(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint8_t light, uint32_t uid, uint8_t act_id, uint8_t cmd_id, uint8_t cwsl)
{
    uint8_t change = check_fan_change(mode, speed, pcn, swing, light);
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
    {
        memset(respond, 0, sizeof(respond));
        sprintf(respond, "{\"%s\":20,\"mode\":%u,\"lv\":%u,\"swing\":%u,\"pcn\":%u,\"lamp\":%u,\"u\":%lu,\"act_id\":%u,\"change\":%u,\"cmd_id\":%u,\"cwsl\":%u}", 
            gs_dev_type[device_type], mode, speed, swing, pcn, light, (unsigned long)uid, act_id, change, (unsigned int)cmd_id, (unsigned int)cwsl);
        app_send_data(respond, strlen(respond));
    }
}

void app_fan_send_temperature(uint32_t temper)
{
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
    {
        memset(respond, 0, sizeof(respond));
        sprintf(respond, "{\"%s\":23,\"temp\":%lu}", gs_dev_type[device_type], temper);
        app_send_data(respond, strlen(respond));
    }
}

extern uint32_t val_battery;
void app_fan_send_battery(uint8_t bat_percent, uint8_t bat_charging_status)
{
    os_trace("app_fan_send_battery\r\n");
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
    {
        os_trace("app_fan_send_battery: %u, %u\r\n", bat_percent, bat_charging_status);
        memset(respond, 0, sizeof(respond));
        sprintf(respond, "{\"%s\":29,\"bat\":%u,\"charging\":%u,\"mV\":%lu}", gs_dev_type[device_type], bat_percent, bat_charging_status, val_battery);
        app_send_data(respond, strlen(respond));
    }
}
