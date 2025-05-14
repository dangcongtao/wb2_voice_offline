#include "app_config.h"
#include "m_mqtt.h"
#include "m_ota.h"
#include "system_typedef.h"
#include "process_data.h"

extern char respond[512];

void app_fan_respond_light(uint8_t state, uint8_t src)
{
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
        memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":25,\"lamp\":%u,\"act_id\":%u}", gs_dev_type[device_type], state, src);

    app_send_data(respond, strlen(respond));
}

void app_fan_respond_control(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint8_t light, uint32_t uid, uint8_t act_id)
{
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
        memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":20,\"mode\":%u,\"lv\":%u,\"swing\":%u,\"pcn\":%u,\"lamp\":%u,\"u\":%lu,\"act_id\":%u}", gs_dev_type[device_type], mode, speed, swing, pcn, light, uid, act_id);
    app_send_data(respond, strlen(respond));
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
