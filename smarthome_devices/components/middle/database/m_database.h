#ifndef _M_DATABASE_H_
#define _M_DATABASE_H_

#include <stdbool.h>
#include <stdint.h>

#include "system_typedef.h"

#include "m_wifi.h"
#include "m_mqtt.h"
#include "m_switch.h"
#include "m_beacon.h"

#define BIT_FLAG_SYNC_WIFI      0
#define BIT_FLAG_SYNC_MQTT      1
#define BIT_FLAG_SYNC_SWITCH    2
#define BIT_FLAG_SYNC_PAIR      3
#define BIT_FLAG_SYNC_BLE       4

#define SET(PIN,N) (PIN |=  (1<<N))
#define CLR(PIN,N) (PIN &= ~(1<<N))
#define GET(PIN,N) ((PIN >> N) & 1)

/* for bootloader */
typedef enum {
    FIRMWARE_NORMAL,
    FIRMWARE_UPDATE,
    FIRMWARE_UPDATE_SUCCESS,
    FIRMWARE_UPDATE_FAIL,
} m_bld_fw_state;

typedef struct {
    m_bld_fw_state  _firmware_state;
    uint8_t         _new_fw_len;
    uint32_t        _crc;
} m_bootloader_t;

/* For App */
typedef bool (*p_dev_get_pair_t)(void);
typedef void (*p_dev_set_pair_t)(bool new_state);
typedef char* (*p_get_dev_id_t)(void);
typedef void (*p_set_dev_id_t)(char* dev_id, uint32_t len);
typedef void (*p_db_app_sync_t)(uint8_t flag);

typedef void (*p_set_lock_channel_t)(uint32_t index, uint8_t lock);
typedef void (*p_set_default_state_t)(uint32_t index, uint8_t mode);
typedef void (*p_set_current_state_t)(uint32_t index, uint8_t mode);
typedef void (*p_set_time_release_t)(uint32_t index, uint32_t time, uint32_t mode);
typedef void (*p_get_time_release_t)(uint32_t index, uint32_t *time, uint32_t *mode);

typedef void (*p_db_reset_factory_t)(void);
typedef void (*p_backup_database_t)(void);

typedef struct {
    
    uint32_t            _device_is_paired;
    m_wifi_info_t*      _wifi_info;
    m_mqtt_info_t*      _mqtt_info;
    m_switch_info_t*    _switch_info;
    m_ble_info_t*       _ble_info;
    uint32_t checksum_crc;
#if PROJECT_SHOCK
    uint8_t disable_anti_shock;
#endif
    uint32_t version;
} m_device_t;

typedef struct {
/* properties */
    m_device_t          _m_device;

/* methods */
    p_dev_get_pair_t    _get_dev_pair_state;
    p_dev_set_pair_t    _set_dev_pair_state;
    p_set_dev_id_t      _set_dev_id;
    p_get_dev_id_t      _get_dev_id;
    p_db_app_sync_t     _request_sync_db;

    p_set_default_state_t _set_current_state_cb;
    p_set_default_state_t _set_default_state_cb;
    p_set_lock_channel_t _set_lock_channel_cb;
    p_set_time_release_t _set_time_release_cb;
    p_get_time_release_t _get_time_release_cb;

    p_db_reset_factory_t _reset_db_cb;
    p_db_reset_factory_t _reset_db_without_sync_cb;
    p_backup_database_t  _backup_db_cb;
} m_database_t;

m_wifi_info_t* m_db_get_wifi_reference(void);
m_switch_info_t* m_db_get_switch_reference(void);

void m_database_init(void);
void m_database_task_start(void);
m_database_t* m_db_get_instance(void);

void m_db_read_device_id(void); /* just only be called in main function to get type device and init hardware. Not call in other place */
uint8_t m_db_get_number_channel(void); 

void flash_set_rs_reason(uint8_t is_set);
uint8_t flash_read_rs_reason(void);

#endif