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

#define DEF_STATE0 "default_state0"
#define DEF_STATE1 "default_state1"
#define DEF_STATE2 "default_state2"
#define DEF_STATE3 "default_state3"
char* defaut_states[] = {DEF_STATE0, DEF_STATE1, DEF_STATE2, DEF_STATE3};

#define CUR_STATE0 "current_state0"
#define CUR_STATE1 "current_state1"
#define CUR_STATE2 "current_state2"
#define CUR_STATE3 "current_state3"
char* current_states[] = {CUR_STATE0, CUR_STATE1, CUR_STATE2, CUR_STATE3};

#define LOCK_STATE0 "lock_state0"
#define LOCK_STATE1 "lock_state1"
#define LOCK_STATE2 "lock_state2"
#define LOCK_STATE3 "lock_state3"
char* lock_states[] = {LOCK_STATE0, LOCK_STATE1, LOCK_STATE2, LOCK_STATE3};

#define TIME_RELEASE0 "time_release0"
#define TIME_RELEASE1 "time_release1"
#define TIME_RELEASE2 "time_release2"
#define TIME_RELEASE3 "time_release3"
char* time_releases[] = {TIME_RELEASE0, TIME_RELEASE1, TIME_RELEASE2, TIME_RELEASE3};

#define RELEASE_MODE0 "mode_release0"
#define RELEASE_MODE1 "mode_release1"
#define RELEASE_MODE2 "mode_release2"
#define RELEASE_MODE3 "mode_release3"
char* release_modes[] = {RELEASE_MODE0, RELEASE_MODE1, RELEASE_MODE2, RELEASE_MODE3};

#define WIFI_SSID "wifi_ssid"
#define WIFI_PASS "wifi_passwd"
#define PAIR_STATE "pair_state"

#define MQTT_TOPIC_PUB "topic_pub"
#define MQTT_TOPIC_SUB "topic_sub"
#define MQTT_SERVER "server"
#define MQTT_PORT "port"
#define MQTT_USERNAME "username"
#define MQTT_PASSWD "passwd"

void flash_init(void) 
{
    easyflash_init();
}

void flash_write_database(const m_device_t object_data, uint32_t _size)
{
    uint32_t size = 0;

    /* write wifo info */
    size = sizeof(object_data._wifi_info._ssid);
    ef_set_bytes(WIFI_SSID, (uint8_t*)object_data._wifi_info._ssid, size);
    size = sizeof(object_data._wifi_info._passwd);
    ef_set_bytes(WIFI_PASS, (uint8_t*)object_data._wifi_info._passwd, size);

    /* write mqtt info */
    os_trace("write mqtt info\r\n");
    if (object_data._mqtt_info == NULL) {
        os_trace("object_data._mqtt_info NULL\r\n");
    }
    size = sizeof(object_data._mqtt_info->topic_pub);
    ef_set_bytes(MQTT_TOPIC_PUB, (uint8_t*)object_data._mqtt_info->topic_pub, size);
    size = sizeof(object_data._mqtt_info->topic_sub);
    ef_set_bytes(MQTT_TOPIC_SUB, (uint8_t*)object_data._mqtt_info->topic_sub, size);
    size = sizeof(object_data._mqtt_info->server);
    ef_set_bytes(MQTT_SERVER, (uint8_t*)object_data._mqtt_info->server, size);
    ef_set_u32(MQTT_PORT, object_data._mqtt_info->port);
    size = sizeof(object_data._mqtt_info->username);
    ef_set_bytes(MQTT_USERNAME, (uint8_t*)object_data._mqtt_info->username, size);
    size = sizeof(object_data._mqtt_info->passwd);
    ef_set_bytes(MQTT_PASSWD, (uint8_t*)object_data._mqtt_info->passwd, size);

    /* read switch info */
    uint32_t i;
    for (i = 0; i < max_number_channel; i++) {
        os_trace("write switch info %ld\r\n", i);
        ef_set_u32(defaut_states[i], object_data._switch_info.default_state[i]);
        ef_set_u32(current_states[i], object_data._switch_info.current_state[i]);
        ef_set_u32(lock_states[i], object_data._switch_info.channel_lock_state[i]);
        ef_set_u32(time_releases[i], object_data._switch_info.time_release[i]);
        ef_set_u32(release_modes[i], object_data._switch_info.release_mode[i]);
    }

    os_trace("write state pair\r\n");
    /* write state pair */
    ef_set_u32(PAIR_STATE, object_data._device_is_paired);
}

void flash_read_database(m_device_t* object_data, uint32_t* _size)
{
    uint32_t size = 0;

    /* read wifi info */
    size = sizeof(object_data->_wifi_info._ssid);
    ef_get_bytes(WIFI_SSID, (uint8_t*)object_data->_wifi_info._ssid, size);
    size = sizeof(object_data->_wifi_info._passwd);
    ef_get_bytes(WIFI_PASS, (uint8_t*)object_data->_wifi_info._passwd, size);

    /* read mqtt info */
    size = sizeof(object_data->_mqtt_info->topic_pub);
    ef_get_bytes(MQTT_TOPIC_PUB, (uint8_t*)object_data->_mqtt_info->topic_pub, size);
    size = sizeof(object_data->_mqtt_info->topic_sub);
    ef_get_bytes(MQTT_TOPIC_SUB, (uint8_t*)object_data->_mqtt_info->topic_sub, size);
    size = sizeof(object_data->_mqtt_info->server);
    ef_get_bytes(MQTT_SERVER, (uint8_t*)object_data->_mqtt_info->server, size);
    size = sizeof(object_data->_mqtt_info->port);
    ef_get_u32(MQTT_PORT, &(object_data->_mqtt_info->port));
    size = sizeof(object_data->_mqtt_info->username);
    ef_get_bytes(MQTT_USERNAME, (uint8_t*)object_data->_mqtt_info->username, size);
    size = sizeof(object_data->_mqtt_info->passwd);
    ef_get_bytes(MQTT_PASSWD, (uint8_t*)object_data->_mqtt_info->passwd, size);

    /* read switch info */
    uint32_t i;
    for (i = 0; i < max_number_channel; i++) {
        ef_get_u32(defaut_states[i], (uint32_t *)&(object_data->_switch_info.default_state[i]));
        ef_get_u32(current_states[i], (uint32_t *)&(object_data->_switch_info.current_state[i]));
        ef_get_u32(lock_states[i], (uint32_t *)&(object_data->_switch_info.channel_lock_state[i]));
        ef_get_u32(time_releases[i], (uint32_t *)&(object_data->_switch_info.time_release[i]));
        ef_get_u32(release_modes[i], (uint32_t *)&(object_data->_switch_info.release_mode[i]));
        
    }

    /* read state pair */
    if ((ef_get_u32(PAIR_STATE, &(object_data->_device_is_paired)) == false) || (object_data->_device_is_paired == 0))
    {
        os_trace("Read flash: Device was not paired\r\n");
        object_data->_switch_info.default_state[0] = SWITCH_ONE_DEFAULT_OFF;
        object_data->_switch_info.default_state[1] = SWITCH_TWO_DEFAULT_OFF;
        object_data->_switch_info.default_state[2] = SWITCH_THREE_DEFAULT_OFF;
        object_data->_switch_info.default_state[3] = SWITCH_FOUR_DEFAULT_OFF;
    }
}

void flash_read_device_id(void* id_buff, uint32_t size)
{
    ef_get_bytes(DEVICE_ID, id_buff, size);
}

void flash_write_device_id(void* device_id, uint32_t size)
{
    /* write wifo info */
    ef_set_bytes(DEVICE_ID, device_id, size);
}