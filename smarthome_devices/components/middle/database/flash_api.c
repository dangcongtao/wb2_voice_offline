#include "flash_api.h"
#include "hardware.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <blog.h>
#include "bl_sys.h"
#include "easyflash_common.h"

#define DEVICE_ID "device_id"
#define REASON_RS   "reset_reason"

#define WIFI_INFO "wifi_info"
#define MQTT_INFO "mqtt_info"
#define AC_INFO "ac_info"
#define FAN_INFO "fan_info"
#define PAIR_STATE "pair_state"
#define VERSION_INFO "ver"

void flash_init(void)
{
    easyflash_init();
}

extern uint16_t dfu_crc16(uint16_t crc, unsigned char *pD, int len);
/*
    m_wifi_info_t*      _wifi_info;
    m_mqtt_info_t*      _mqtt_info;
    m_switch_info_t*    _switch_info;
 */
void flash_write_database(m_device_t object_data, uint8_t request_flag)
{
    uint32_t size = 0;

    /* write wifo info */
    if (object_data._wifi_info == NULL)
    {
        os_trace("object_data._wifi_info NULL\r\n");
    }
    else if (GET(request_flag, BIT_FLAG_SYNC_WIFI))
    {
        size = sizeof(m_wifi_info_t);
        os_trace("sync wifi info %lu\r\n", size);
        ef_set_bytes(WIFI_INFO, (uint8_t *)object_data._wifi_info, size);
    }

    if (object_data._mqtt_info == NULL)
    {
        os_trace("object_data._mqtt_info NULL\r\n");
    }
    else if (GET(request_flag, BIT_FLAG_SYNC_MQTT))
    {
        size = sizeof(m_mqtt_info_t);
        os_trace("sync mqtt info %lu\r\n", size);
        ef_set_bytes(MQTT_INFO, (uint8_t *)object_data._mqtt_info, size);
    }

    if (object_data._ac_info == NULL)
    {
        os_trace("object_data._ac_info NULL\r\n");
    }
    else if (GET(request_flag, BIT_FLAG_SYNC_AC))
    {
        size = sizeof(m_ac_info_t);
        os_trace("sync ac info %lu\r\n", size);
        ef_set_bytes(AC_INFO, (uint8_t *)object_data._ac_info, size);
    }

    if (object_data._fan_info== NULL)
    {
        os_trace("object_data._fan_info NULL\r\n");
    }
    else if (GET(request_flag, BIT_FLAG_SYNC_FAN))
    {
        size = sizeof(m_fan_info_t);
        os_trace("sync fan info %lu\r\n", size);
        ef_set_bytes(FAN_INFO, (uint8_t *)object_data._fan_info, size);
    }

    /* write state pair */
    if (GET(request_flag, BIT_FLAG_SYNC_PAIR)) {
        os_trace("sync pair state info\r\n");
        ef_set_u32(PAIR_STATE, object_data._device_is_paired);
    }
}

void flash_read_database(m_device_t *object_data)
{
    uint32_t size = 0;
    printf("%s\r\n", __func__);

    if (object_data == NULL)
        return;

    if (object_data->_wifi_info == NULL)
    {
        os_trace("object_data->_wifi_info NULL\r\n");
    }
    else
    {
        /* read wifi info */
        size = sizeof(m_wifi_info_t);
        ef_get_bytes(WIFI_INFO, (uint8_t *)object_data->_wifi_info, size);
    }

    if (object_data->_ac_info == NULL)
    {
        os_trace("object_data->_ac_info NULL\r\n");
    }
    else
    {
        /* read ac info */
        size = sizeof(m_ac_info_t);
        ef_get_bytes(AC_INFO, (uint8_t *)object_data->_ac_info, size);
    }

    if (object_data->_fan_info == NULL)
    {
        os_trace("object_data->_fan_info NULL\r\n");
    }
    else
    {
        /* read fan info */
        size = sizeof(m_fan_info_t);
        ef_get_bytes(FAN_INFO, (uint8_t *)object_data->_fan_info, size);
    }

    ef_get_bytes(VERSION_INFO, (uint8_t *)&(object_data->version), sizeof(uint32_t));
    if (object_data->version > 0) {
        if (object_data->_mqtt_info == NULL)
        {
            os_trace("object_data->_mqtt_info NULL\r\n");
        }
        else
        {
            /* read wifi info */
            size = sizeof(m_mqtt_info_t);
            ef_get_bytes(MQTT_INFO, (uint8_t *)object_data->_mqtt_info, size);
        }
    } else {
        object_data->version = FIRMWARE_VERSION_NUMBER;
        ef_set_bytes(VERSION_INFO, (uint8_t *)&(object_data->version), sizeof(uint32_t));

        m_mqtt_info_old_t mqtt_old;
        size = sizeof(m_mqtt_info_old_t);
        ef_get_bytes(MQTT_INFO, (uint8_t *)&mqtt_old, size);

        object_data->_mqtt_info->num_broker = 0;
        object_data->_mqtt_info->broker_in_used = 0;

        memset(object_data->_mqtt_info->topic_sub, 0, sizeof(object_data->_mqtt_info->topic_sub));
        memset(object_data->_mqtt_info->topic_pub, 0, sizeof(object_data->_mqtt_info->topic_pub));

        memcpy(object_data->_mqtt_info->topic_sub, mqtt_old.topic_sub, sizeof(object_data->_mqtt_info->topic_sub));
        memcpy(object_data->_mqtt_info->topic_pub, mqtt_old.topic_pub, sizeof(object_data->_mqtt_info->topic_pub));

        // use old server if cannot get mqtt server
        object_data->_mqtt_info->broker_in_used = 0;
        object_data->_mqtt_info->num_broker = 1;
        memcpy(object_data->_mqtt_info->servers[0].server, mqtt_old.server, sizeof(object_data->_mqtt_info->servers[0].server));
        memcpy(object_data->_mqtt_info->servers[0].username, mqtt_old.username, sizeof(object_data->_mqtt_info->servers[0].username));
        memcpy(object_data->_mqtt_info->servers[0].passwd, mqtt_old.passwd, sizeof(object_data->_mqtt_info->servers[0].passwd));
        object_data->_mqtt_info->servers[0].port = mqtt_old.port;
        object_data->_mqtt_info->servers[0].type = 1;
        m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_MQTT);

    }

    /* read state pair */
    if ((ef_get_u32(PAIR_STATE, &(object_data->_device_is_paired)) == false) || (object_data->_device_is_paired == 0))
    {
        os_trace("Read flash: Device was not paired\r\n");
    }
}

int32_t flash_read_wifi_info(m_wifi_info_t *wifi_info)
{
    int read_length;

    if (wifi_info == NULL) {
        return -1;
    }

    memset(wifi_info, 0, sizeof(*wifi_info));
    read_length = ef_get_bytes(WIFI_INFO,
                               (uint8_t *)wifi_info,
                               sizeof(*wifi_info));
    if (read_length != (int)sizeof(*wifi_info)) {
        memset(wifi_info, 0, sizeof(*wifi_info));
        return -1;
    }

    if ((memchr(wifi_info->_ssid, '\0', sizeof(wifi_info->_ssid)) == NULL) ||
        (memchr(wifi_info->_passwd, '\0', sizeof(wifi_info->_passwd)) == NULL) ||
        (wifi_info->_ssid[0] == '\0')) {
        memset(wifi_info, 0, sizeof(*wifi_info));
        return -1;
    }
    wifi_info->_ip[sizeof(wifi_info->_ip) - 1U] = '\0';
    return 0;
}

void flash_read_device_id(void *id_buff, uint32_t size)
{
    ef_get_bytes(DEVICE_ID, id_buff, size);
}

void flash_write_device_id(void *device_id, uint32_t size)
{
    /* write wifo info */
    ef_set_bytes(DEVICE_ID, device_id, size);
}

void flash_save_fan_info(const m_fan_info_t *fan_info)
{
    if (fan_info != NULL)
    {
        ef_set_bytes(FAN_INFO, (const uint8_t *)fan_info, sizeof(m_fan_info_t));
    }
}

void flash_set_rs_reason(uint8_t is_set)
{
    ef_set_bytes(REASON_RS, &is_set, sizeof(is_set));
    m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_FAN);
}

uint8_t flash_read_rs_reason(void)
{
    if (m_db_get_instance()->_get_dev_pair_state() == 0) {
        return 0;
    }

    uint8_t reason = 0;
    ef_get_bytes(REASON_RS, &reason, sizeof(reason));
    return reason;
}