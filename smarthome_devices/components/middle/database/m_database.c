#include "m_database.h"
#include "m_mqtt.h"
#include "flash_api.h"

#include <string.h>
#include <stdio.h>

#include "app_config.h"
#include "aes_cbc128.h"

#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>

char key[16], iv[16];

static char         s_device_id[64];
static bool         s_request_sync_db = false;
static bool         s_request_reset_db = false;
static bool         s_request_backup_db = false;
static uint8_t      s_database_was_deleted_before = false;
static m_database_t s_m_database;
static m_device_t   s_m_device_backup;

extern uint32_t max_number_channel;
extern uint32_t device_type;
extern char* gs_dev_type[];

/* parser device id to get number channel - HUN21210803110509154wswitch2v3 (3 channel) */
/* HUN163696975919lhswitch2v4 lahu */
void m_db_read_device_id(void)
{
    // uint32_t len_device_id = sizeof(s_device_id);
    // flash_read_device_id((void*)(&s_device_id), len_device_id);
    memset(s_device_id, 0, sizeof(s_device_id));
    sprintf(s_device_id, "HUN21210803110509154wswitch2v4");
    os_trace("m_db_read_device_id: %s\r\r\n", s_device_id);

    max_number_channel = s_device_id[strlen(s_device_id)-1] - '0';
    os_trace("max_number_channel: %ld\r\r\n", max_number_channel);

    /* get device type */
    if (strstr(s_device_id, gs_dev_type[WSWITCH]) != NULL) {
        device_type = WSWITCH;
    } else if (strstr(s_device_id, gs_dev_type[LHSWITCH]) != NULL) {
        device_type = LHSWITCH;
    }  else if (strstr(s_device_id, gs_dev_type[WSDATIC]) != NULL) {
        device_type = WSDATIC;
    }
}

uint8_t m_db_get_number_channel(void)
{
    return max_number_channel;
}

static bool m_db_dev_get_pair(void)
{
    os_trace("Device was %s\r\r\n", s_m_database._m_device._device_is_paired? "paired" : "not paired");
    return s_m_database._m_device._device_is_paired;
}

static void m_db_dev_set_pair(bool new_state)
{
    os_trace("Set device %s\r\r\n", new_state? "paired" : "not paired");
    s_m_database._m_device._device_is_paired = new_state;
}

static char* m_get_dev_id(void)
{
    os_trace("m_get_dev_id\r\r\n");
    return s_device_id;
}

static void m_db_request_sync_database(void)
{
    s_request_sync_db = true;
}

static void m_db_reset_factory_database(void)
{
    s_request_reset_db = true;
}

static void m_db_backup_database(void)
{
    s_request_backup_db = true;
}

static void set_current_state(uint32_t index, uint8_t action)
{
    if (index > m_db_get_number_channel())
        return;

    s_m_database._m_device._switch_info.current_state[index] = action;
}

static void set_default_state(uint32_t index, uint8_t mode)
{
    if (index > m_db_get_number_channel())
        return;

    s_m_database._m_device._switch_info.default_state[index] = mode;
    m_db_request_sync_database();
}

static void set_lock_channel(uint32_t index, uint8_t lock)
{
    if (index > m_db_get_number_channel())
        return;

    s_m_database._m_device._switch_info.channel_lock_state[index] = lock;
    m_db_request_sync_database();
}

static void set_time_release(uint32_t index, uint32_t time, uint32_t mode)
{
    s_m_database._m_device._switch_info.time_release[index] = time;
    s_m_database._m_device._switch_info.release_mode[index] = mode;
    os_trace("mode: %ld\r\n", s_m_database._m_device._switch_info.release_mode[index]);
    m_db_request_sync_database();
}

static void get_time_release(uint32_t index, uint32_t *time, uint32_t *mode)
{
    *time = s_m_database._m_device._switch_info.time_release[index];
    *mode = s_m_database._m_device._switch_info.release_mode[index];
    os_trace("mode: %ld\r\n", s_m_database._m_device._switch_info.release_mode[index]);
}

static void generate_aes_key_iv(void)
{
    uint8_t device_id_encoded[256];
    uint32_t len_of_code = sizeof(device_id_encoded);

    os_trace("generate key iv\r\n");
    os_trace("device_id: %s\r\n", s_device_id);

    aes_cbc128_init((uint8_t*)DEFAULT_AES_KEY, (uint8_t*)DEFAULT_AES_IV);
    memset(device_id_encoded, 0, sizeof(device_id_encoded));
    uint32_t aes_encrypt_result = aes_cbc128_encrypt(
                                            (uint8_t *)s_device_id, strlen(s_device_id), 
                                            device_id_encoded, (size_t *)&len_of_code);
    if (aes_encrypt_result != AES_SUCCESS) {
        os_trace("Can not encode AES Device ID %ld\r\n", aes_encrypt_result);
    } else {
        memset(key, 0, sizeof(key));
        memset(iv, 0, sizeof(iv));

        memcpy(key, &(device_id_encoded[4]), 16);
        memcpy(iv, &(device_id_encoded[12]), 16);

        os_trace("Get key - iv success\r\n");
    }
}

static void m_load_databse(void)
{
    uint8_t i = 0;
    
    os_trace("=============== app load database ===============\r\n");
    uint32_t len_database = sizeof(m_device_t);

    m_db_read_device_id();
    flash_read_database((void*)(&s_m_database._m_device), &len_database);

    os_trace("device_id: %s\r\n", s_device_id);
    os_trace("server: %s\r\n", s_m_database._m_device._mqtt_info->server);
    os_trace("port: %ld\r\n", s_m_database._m_device._mqtt_info->port);
    os_trace("username: %s\r\n", s_m_database._m_device._mqtt_info->username);
    os_trace("passwd: %s\r\n", s_m_database._m_device._mqtt_info->passwd);
    os_trace("topic_pub: %s\r\n", s_m_database._m_device._mqtt_info->topic_pub);
    os_trace("topic_sub: %s\r\n", s_m_database._m_device._mqtt_info->topic_sub);

    os_trace("default state: ");
    for (i = 0; i < m_db_get_number_channel(); i++) {
        os_trace("%u ", s_m_database._m_device._switch_info.default_state[i]);
    }
    os_trace("\r\n");

    os_trace("current state: ");
    for (i = 0; i < m_db_get_number_channel(); i++) {
        os_trace("%u ", s_m_database._m_device._switch_info.current_state[i]);
    }
    os_trace("\r\n");

    os_trace("lock mode: ");
    for (i = 0; i < m_db_get_number_channel(); i++) {
        os_trace("%u ", s_m_database._m_device._switch_info.channel_lock_state[i]);
    }
    os_trace("\r\n");

    os_trace("time release: ");
    for (i = 0; i < m_db_get_number_channel(); i++) {
        os_trace("%ld ", s_m_database._m_device._switch_info.time_release[i]);
    }
    os_trace("\r\n");

    if (s_m_database._m_device._device_is_paired) {
        os_trace("device was paired\r\n");
    } else {
        os_trace("device was not paired\r\n");
    }
    os_trace("============================================\r\n");
}

extern void clear_cyclic_timer(void);
static void m_database_sync_task(void *param)
{
    while(1) {
        if (s_request_reset_db) {
            os_trace("have reset factory request\r\n");

            if (s_database_was_deleted_before != true) {
                /* save backup */
                memset(&s_m_device_backup, 0, sizeof(s_m_device_backup));
                memcpy(&s_m_device_backup, &(s_m_database._m_device), sizeof(m_device_t));
                s_database_was_deleted_before = true;
            }
            
            /* reset factory */
            clear_cyclic_timer();
            memset(&s_m_database._m_device, 0, sizeof(m_device_t));
            s_m_database._m_device._switch_info.default_state[0] = SWITCH_ONE_DEFAULT_OFF;
            s_m_database._m_device._switch_info.default_state[1] = SWITCH_TWO_DEFAULT_OFF;
            s_m_database._m_device._switch_info.default_state[2] = SWITCH_THREE_DEFAULT_OFF;
            s_m_database._m_device._switch_info.default_state[3] = SWITCH_FOUR_DEFAULT_OFF;

            s_request_reset_db = 0;
            s_request_sync_db = 1;
        }

        if (s_request_backup_db) {
            os_trace("have backup database request\r\n");
            memcpy(&s_m_database._m_device._switch_info, &(s_m_device_backup._switch_info), sizeof(s_m_device_backup._switch_info));
            s_request_backup_db = 0;
            s_request_sync_db = 1;
        }

        if (s_request_sync_db) {
            os_trace("app sync database\r\n");
            // flash_write_database(s_m_database._m_device, sizeof(m_device_t));

            if (s_m_database._m_device._device_is_paired) {
                s_database_was_deleted_before = false;
            }

            s_request_sync_db = false;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); /* should be 500ms to make it's priority low */
    }
}

void m_database_init(void)
{
    s_m_database._m_device._mqtt_info = m_mqtt_get_info_reference();
    s_m_database._set_dev_pair_state = m_db_dev_set_pair;
    s_m_database._get_dev_pair_state = m_db_dev_get_pair;
    s_m_database._get_dev_id = m_get_dev_id;
    s_m_database._request_sync_db = m_db_request_sync_database;
    s_m_database._reset_db_cb = m_db_reset_factory_database;
    s_m_database._backup_db_cb = m_db_backup_database;

    s_m_database._set_current_state_cb = set_current_state;
    s_m_database._set_default_state_cb = set_default_state;
    s_m_database._set_lock_channel_cb = set_lock_channel;
    s_m_database._set_time_release_cb = set_time_release;
    s_m_database._get_time_release_cb = get_time_release;

    m_load_databse();
    generate_aes_key_iv();

    xTaskCreate(m_database_sync_task,       /* Function that implements the task. */
                "m_database_sync_task",          /* Text name for the task. */
                1024*5+500,      /* Stack size in words, not bytes. */
                NULL,    /* Parameter passed into the task. */
                1,/* Priority at which the task is created. */
                NULL);
}

m_wifi_info_t* m_db_get_wifi_reference(void)
{
    m_wifi_info_t* p_wifi_reference = &s_m_database._m_device._wifi_info;
    return p_wifi_reference;
}

m_database_t *m_db_get_instance(void)
{
    return &s_m_database;
}