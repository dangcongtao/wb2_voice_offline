#include "m_database.h"
#include "m_wifi.h"
#include "m_mqtt.h"
#include "flash_api.h"

#include <string.h>
#include <stdio.h>

#include "app_config.h"
#include "aes_cbc128.h"

#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>

#include "IRrecv.h"

char key[16], iv[16];

char s_device_id[64];
// uint8_t last_reason_reset = 0;
static uint8_t s_request_sync_db = 0;
static bool s_request_reset_db = false;
static bool s_request_reset_db_without_sync = false;
static bool s_request_backup_db = false;
static uint8_t s_database_was_deleted_before = false;
static m_database_t s_m_database;
/* backup database */
static m_wifi_info_t s_m_wifi_info_backup;
static m_mqtt_info_t s_m_mqtt_info_backup;
static uint32_t s_m_pair_state_backup;

extern uint32_t max_number_channel;
extern uint32_t device_type;
extern char *gs_dev_type[];

/* parser device id to get number channel - HUN21210803110509154wswitch2v3 (3 channel) */
/* HUN163696975919lhswitch2v4 lahu */
void m_db_read_device_id(void)
{
    uint32_t len_device_id = sizeof(s_device_id);
    memset(s_device_id, 0, sizeof(s_device_id));
#if 0
    sprintf(s_device_id, "HUN178641908061fanacirN1");
    flash_write_device_id((void *)(&s_device_id), len_device_id);
#endif
    flash_read_device_id((void *)(&s_device_id), len_device_id);
    // sprintf(s_device_id, "HUN167351046986wswitch2vN1");
    // sprintf(s_device_id, "HUN167590918658daticbsv2N1");
    os_trace("m_db_read_device_id: %s\r\n", s_device_id);

    max_number_channel = s_device_id[strlen(s_device_id) - 1] - '0';
    os_trace("max_number_channel: %ld\r\n", max_number_channel);

    /* get device type */
    if (strstr(s_device_id, gs_dev_type[IRWIFI]) != NULL)
    {
        device_type = IRWIFI;
    }
    else if (strstr(s_device_id, gs_dev_type[FANDC]) != NULL)
    {
        device_type = FANDC;
    }
    else if (strstr(s_device_id, gs_dev_type[FANACIR]) != NULL)
    {
        device_type = FANACIR;
    }
    else if (strstr(s_device_id, gs_dev_type[FANAC]) != NULL)
    {
        device_type = FANAC;
    }

    os_trace("device_type: %s\r\n", gs_dev_type[device_type]);
}

uint8_t m_db_get_number_channel(void)
{
    return max_number_channel;
}

static bool m_db_dev_get_pair(void)
{
    return s_m_database._m_device._device_is_paired;
}

static char *m_get_dev_id(void)
{
    return s_device_id;
}

static void m_db_request_sync_database(uint8_t bit_flag)
{
    SET(s_request_sync_db, bit_flag);
}

static void m_db_dev_set_pair(bool new_state)
{
    s_m_database._m_device._device_is_paired = new_state;
    m_db_request_sync_database(BIT_FLAG_SYNC_PAIR);
}

static void m_db_reset_factory_database(void)
{
    s_request_reset_db = true;
}

static void m_db_reset_factory_database_without_sync(void)
{
    s_request_reset_db_without_sync = true;
}

static void m_db_backup_database(void)
{
    os_trace("m_db_backup_database\r\n");

    memcpy(s_m_database._m_device._wifi_info, &s_m_wifi_info_backup, sizeof(m_wifi_info_t));
    memcpy(s_m_database._m_device._mqtt_info, &s_m_mqtt_info_backup, sizeof(m_mqtt_info_t));
    s_m_database._m_device._device_is_paired = s_m_pair_state_backup;

    s_request_backup_db = true;
}

static void generate_aes_key_iv(void)
{
    uint8_t device_id_encoded[256];
    uint32_t len_of_code = sizeof(device_id_encoded);

    os_trace("generate key iv\r\n");

    aes_cbc128_init((uint8_t *)DEFAULT_AES_KEY, (uint8_t *)DEFAULT_AES_IV);
    memset(device_id_encoded, 0, sizeof(device_id_encoded));
    uint32_t aes_encrypt_result = aes_cbc128_encrypt(
        (uint8_t *)s_device_id, strlen(s_device_id),
        device_id_encoded, (size_t *)&len_of_code);
    if (aes_encrypt_result != AES_SUCCESS)
    {
        os_trace("Can not encode AES Device ID %ld\r\n", aes_encrypt_result);
    }
    else
    {
        memset(key, 0, sizeof(key));
        memset(iv, 0, sizeof(iv));

        memcpy(key, &(device_id_encoded[4]), 16);
        memcpy(iv, &(device_id_encoded[12]), 16);

        os_trace("Get key - iv success\r\n");
    }
}

// static void flash_proccess_reset_reason(void)
// {
//     uint8_t reason_reset = flash_read_rs_reason();
//     if (reason_reset)
//     {
//         last_reason_reset = reason_reset;
//         flash_set_rs_reason(0);
//     }
// }

uint16_t dfu_crc16(uint16_t crc, unsigned char *pD, int len)
{
    uint16_t poly[2] = {0, 0xa001}; // 0x8005 <==> 0xa001
    int i, j;

    for (j = len; j > 0; j--)
    {
        unsigned char ds = *pD++;
        for (i = 0; i < 8; i++)
        {
            crc = (crc >> 1) ^ poly[(crc ^ ds) & 1];
            ds = ds >> 1;
        }
    }

    // os_trace("crc: %u\n", crc);
    return crc;
}

static void m_load_databse(void)
{
    m_db_read_device_id();
    flash_read_database(&s_m_database._m_device);

    if (!s_m_database._m_device._fan_info->thresh_hold_temperature[0] ||
        !s_m_database._m_device._fan_info->thresh_hold_temperature[1] ||
        !s_m_database._m_device._fan_info->thresh_hold_temperature[2])
    {
        os_trace("%s SET DEFAULT THRESHOLD VALUE TEMPERATURE\r\n", __func__);
        s_m_database._m_device._fan_info->thresh_hold_temperature[0] = 200;
        s_m_database._m_device._fan_info->thresh_hold_temperature[1] = 260;
        s_m_database._m_device._fan_info->thresh_hold_temperature[2] = 290;
        m_db_request_sync_database(BIT_FLAG_SYNC_FAN);
    }
    // flash_proccess_reset_reason();

    // memset(s_m_database._m_device._ble_info, 0, sizeof(m_ble_info_t));
    // flash_ble_write_database(s_m_database._m_device);

    // memset(s_m_database._m_device._wifi_info, 0, sizeof(m_wifi_info_t));
    // memset(s_m_database._m_device._mqtt_info, 0, sizeof(m_mqtt_info_t));
    // memset(s_m_database._m_device._switch_info, 0, sizeof(m_device_t));
    // s_m_database._m_device._device_is_paired = 0;

    os_trace("=============== app load database ===============\r\n");
    os_trace("device_id: %s\r\n", s_device_id);

    os_trace("ssid: %s\r\n", s_m_database._m_device._wifi_info->_ssid);
    os_trace("pass: %s\r\n", s_m_database._m_device._wifi_info->_passwd);

    os_trace("topic_pub: %s\r\n", s_m_database._m_device._mqtt_info->topic_pub);
    os_trace("topic_sub: %s\r\n", s_m_database._m_device._mqtt_info->topic_sub);

    os_trace("vermqtt: %d\r\n", s_m_database._m_device._mqtt_info->vermqtt);
    os_trace("num broker: %d\r\n", s_m_database._m_device._mqtt_info->num_broker);
    os_trace("broker_in_used: %d\r\n", s_m_database._m_device._mqtt_info->broker_in_used);
    for (int i = 0; i < s_m_database._m_device._mqtt_info->num_broker; i++)
    {
        os_trace("server: %s\r\n", s_m_database._m_device._mqtt_info->servers[i].server);
        os_trace("port: %ld\r\n", s_m_database._m_device._mqtt_info->servers[i].port);
        os_trace("username: %s\r\n", s_m_database._m_device._mqtt_info->servers[i].username);
        os_trace("passwd: %s\r\n", s_m_database._m_device._mqtt_info->servers[i].passwd);
    }

    if (s_m_database._m_device._device_is_paired)
    {
        os_trace("device was paired\r\n");
    }
    else
    {
        os_trace("device was not paired\r\n");
    }
    os_trace("FIRMWARE_VERSION: %s\r\n", FIRMWARE_VERSION);

    os_trace("============================================\r\n");
}

static void m_database_sync_task(void *param)
{
    while (1)
    {
        if (s_request_reset_db)
        {
            os_trace("have reset factory request\r\n");

            if (s_database_was_deleted_before != true)
            {
                s_database_was_deleted_before = true;

                /* save backup */
                memset(&s_m_wifi_info_backup, 0, sizeof(s_m_wifi_info_backup));
                memset(&s_m_mqtt_info_backup, 0, sizeof(s_m_mqtt_info_backup));
                s_m_pair_state_backup = 0;

                // memcpy(&s_m_wifi_info_backup, s_m_database._m_device._wifi_info, sizeof(m_wifi_info_t));
                // memcpy(&s_m_mqtt_info_backup, s_m_database._m_device._mqtt_info, sizeof(m_mqtt_info_t));
                // s_m_pair_state_backup = s_m_database._m_device._device_is_paired;
            }

            /* reset factory */
            memset(s_m_database._m_device._wifi_info, 0, sizeof(m_wifi_info_t));
            memset(s_m_database._m_device._mqtt_info, 0, sizeof(m_mqtt_info_t));
            s_m_database._m_device._device_is_paired = 0;
            s_m_database._m_device._fan_info->thresh_hold_temperature[0] = 200;
            s_m_database._m_device._fan_info->thresh_hold_temperature[1] = 260;
            s_m_database._m_device._fan_info->thresh_hold_temperature[2] = 290;

            s_request_reset_db = 0;
            s_request_sync_db = 0xFF;
        }

        if (s_request_reset_db_without_sync)
        {
            if (s_database_was_deleted_before != true)
            {
                s_database_was_deleted_before = true;

                /* save backup */
                memset(&s_m_wifi_info_backup, 0, sizeof(s_m_wifi_info_backup));
                memset(&s_m_mqtt_info_backup, 0, sizeof(s_m_mqtt_info_backup));
                memcpy(&s_m_wifi_info_backup, s_m_database._m_device._wifi_info, sizeof(m_wifi_info_t));
                memcpy(&s_m_mqtt_info_backup, s_m_database._m_device._mqtt_info, sizeof(m_mqtt_info_t));
                s_m_pair_state_backup = s_m_database._m_device._device_is_paired;
            }

            /* reset factory */
            memset(s_m_database._m_device._wifi_info, 0, sizeof(m_wifi_info_t));
            memset(s_m_database._m_device._mqtt_info, 0, sizeof(m_mqtt_info_t));

            s_request_reset_db_without_sync = 0;
        }

        if (s_request_backup_db)
        {
            os_trace("have backup database request\r\n");
            s_request_backup_db = 0;
            s_request_sync_db = 0XFF;
        }

        if (s_request_sync_db)
        {
            // os_trace("app sync database\r\n");
            flash_write_database(s_m_database._m_device, s_request_sync_db);
            if (s_m_database._m_device._device_is_paired)
            {
                s_database_was_deleted_before = false;
            }

            // os_trace("ssid: %s\r\n", s_m_database._m_device._wifi_info->_ssid);
            // os_trace("pass: %s\r\n", s_m_database._m_device._wifi_info->_passwd);
            // os_trace("server: %s\r\n", s_m_database._m_device._mqtt_info->server);
            // os_trace("port: %ld\r\n", s_m_database._m_device._mqtt_info->port);
            // os_trace("username: %s\r\n", s_m_database._m_device._mqtt_info->username);
            // os_trace("passwd: %s\r\n", s_m_database._m_device._mqtt_info->passwd);
            // os_trace("topic_pub: %s\r\n", s_m_database._m_device._mqtt_info->topic_pub);
            // os_trace("topic_sub: %s\r\n", s_m_database._m_device._mqtt_info->topic_sub);

            s_request_sync_db = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); /* should be 500ms to make it's priority low */
    }
}

void m_database_init(void)
{
    os_trace("m_database_init\r\n");

    s_m_database._m_device._mqtt_info = m_mqtt_get_info_reference();
    s_m_database._m_device._wifi_info = m_wifi_get_info_reference();
    s_m_database._m_device._ac_info = m_ac_info_get_reference();
    s_m_database._m_device._fan_info = m_fan_get_instance();

    s_m_database._set_dev_pair_state = m_db_dev_set_pair;
    s_m_database._get_dev_pair_state = m_db_dev_get_pair;
    s_m_database._get_dev_id = m_get_dev_id;
    s_m_database._request_sync_db = m_db_request_sync_database;
    s_m_database._reset_db_cb = m_db_reset_factory_database;
    s_m_database._reset_db_without_sync_cb = m_db_reset_factory_database_without_sync;
    s_m_database._backup_db_cb = m_db_backup_database;

    m_load_databse();
    generate_aes_key_iv();

    xTaskCreate(m_database_sync_task,   /* Function that implements the task. */
                "m_database_sync_task", /* Text name for the task. */
                1024,                   /* Stack size in words, not bytes. */
                NULL,                   /* Parameter passed into the task. */
                1,                      /* Priority at which the task is created. */
                NULL);
}

m_database_t *m_db_get_instance(void)
{
    return &s_m_database;
}