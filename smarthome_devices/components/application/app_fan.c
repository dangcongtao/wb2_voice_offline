#include "app_config.h"
#include "m_mqtt.h"
#include "m_ota.h"
#include "system_typedef.h"

extern char respond[512];

void app_fan_respond_light(uint8_t state, uint8_t src)
{
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
        memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":25,\"lamp\":%u,\"act_id\":%u}", gs_dev_type[device_type], state, src);

    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

void app_fan_respond_control(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint32_t uid, uint8_t act_id)
{
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
        memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":20,\"mode\":%u,\"lv\":%u,\"swing\":%u,\"pcn\":%u,\"u\":%lu,\"act_id\":%u}", gs_dev_type[device_type], mode, speed, pcn, swing, uid, act_id);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

void app_fan_send_temperature(uint32_t temper)
{
    if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
        memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":23,\"temp\":%lu}", gs_dev_type[device_type], temper);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}
