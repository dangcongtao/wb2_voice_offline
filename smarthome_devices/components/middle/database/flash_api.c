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

#define WIFI_INFO "wifi_info"
#define MQTT_INFO "mqtt_info"
#define SWITCH_INFO "switch_info"
#define PAIR_STATE "pair_state"

void flash_init(void)
{
    easyflash_init();
}
/*
    m_wifi_info_t*      _wifi_info;
    m_mqtt_info_t*      _mqtt_info;
    m_switch_info_t*    _switch_info;
 */
void flash_write_database(m_device_t object_data)
{
    uint32_t size = 0;

    /* write wifo info */
    if (object_data._wifi_info == NULL)
    {
        os_trace("object_data._wifi_info NULL\r\n");
    }
    else
    {
        size = sizeof(m_wifi_info_t);
        ef_set_bytes(WIFI_INFO, (uint8_t *)object_data._wifi_info, size);
    }

    if (object_data._mqtt_info == NULL)
    {
        os_trace("object_data._mqtt_info NULL\r\n");
    }
    else
    {
        size = sizeof(m_mqtt_info_t);
        ef_set_bytes(MQTT_INFO, (uint8_t *)object_data._mqtt_info, size);
    }

    if (object_data._switch_info == NULL)
    {
        os_trace("object_data._switch_info NULL\r\n");
    }
    else
    {
        size = sizeof(m_switch_info_t);
        ef_set_bytes(SWITCH_INFO, (uint8_t *)object_data._switch_info, size);
    }

    /* write state pair */
    ef_set_u32(PAIR_STATE, object_data._device_is_paired);
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

    if (object_data->_switch_info == NULL)
    {
        os_trace("object_data->_switch_info NULL\r\n");
    }
    else
    {
        /* read wifi info */
        size = sizeof(m_switch_info_t);
        ef_get_bytes(SWITCH_INFO, (uint8_t *)object_data->_switch_info, size);
    }

    /* read state pair */
    if ((ef_get_u32(PAIR_STATE, &(object_data->_device_is_paired)) == false) || (object_data->_device_is_paired == 0))
    {
        os_trace("Read flash: Device was not paired\r\n");
        object_data->_switch_info->default_state[0] = SWITCH_ONE_DEFAULT_OFF;
        object_data->_switch_info->default_state[1] = SWITCH_TWO_DEFAULT_OFF;
        object_data->_switch_info->default_state[2] = SWITCH_THREE_DEFAULT_OFF;
        object_data->_switch_info->default_state[3] = SWITCH_FOUR_DEFAULT_OFF;
    }
}

void flash_read_device_id(void *id_buff, uint32_t size)
{
    sprintf(id_buff, "HUN168593906440lhswitch2vN1");
    return;
    ef_get_bytes(DEVICE_ID, id_buff, size);
}

void flash_write_device_id(void *device_id, uint32_t size)
{
    /* write wifo info */
    ef_set_bytes(DEVICE_ID, device_id, size);
}