#ifndef _BEACON_H_
#define _BEACON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "system_typedef.h"

#define MASTER_SCAN_MAX 36
#define NAME_LEN 30

#define MAX_CTLK_SUPPORT 8
#define HUN_BLE_SERVICE_UUID 0xFBFA

#define DEFAULT_DELAY_LIGHT_OFF_CBCT    10

typedef enum
{
    DEVICE_TYPE_CTLK = 1,
    DEVICE_TYPE_CBCT,
    DEVICE_NEW
} device_stair_type_t;

typedef enum
{
    WAIT_CONFIG_REC = 1,
    RESPONSE_REQUEST_LINK,
    VERIFY_LINK,
    PING_INTERVAL,
    SYNC_STATE,
    REQUEST_CHANGE_STATE,
    NOTIFY_ONLINE,
    NOTIFY_UNLINK,
    RESET_CTLK,
    REQUEST_OTA_CTLK,
    CLTK_ACCEPT_OTA,
    SEND_SEGMENT_OTA,
    REQUEST_SEGMENT_MISS,
    SEND_SS_STATE_CBCT = 0x14,
    CMD_SS_INFO,
    SWITCH_LINKING,
    SWITCH_REMOVE_LINK,
    SWITCH_REMOVE_ALL_NODE
} ble_cmd_t;

typedef enum
{
    NOTHING = 0,
    WAIT_CONFIG,       // when master receive request link from slave
    SEND_REQUEST_LINK, // send requeste link to master
    WAIT_VERIFY_ACCEPT_LINK,
    SEND_SWITCH_REQUEST_LINK,
    WAIT_SWITCH_RESPONSE_LINK,
    RESPOND_TO_REQUEST_LINK, // when master receive accept link from slave
    MASTER_LINK_SUCCESS,
    SLAVE_LINK_SUCCESS,
    LINK_SUCCESS, // when master finished link with slave
    CONFIG_LINK_SUCCESS,
    CTLK_NOTIFY_ONLINE,
    PING_CTC_INTERVAL,
    STEP_SWITCH_REMOVE_LINK,
    STEP_SWITCH_REMOVE_ALL_NODE
} ble_config_state_t;

typedef struct
{
    int8_t rssi;
    uint8_t mac[6];
    char name[NAME_LEN];
} ble_scan_info_t;

typedef struct
{
    uint8_t len;
    uint8_t ad_type;
    uint8_t uuid[2];
    uint8_t cmd;
    uint8_t mac_addr[6];
    uint8_t tid;
    union
    {
        uint8_t mac[6];

        struct
        {
            uint8_t type;
        } config;

        struct
        {
            uint32_t ver;
            uint8_t battery;
        } ss_info;

        // struct
        // {
        // } ping ;

        struct
        {
            uint32_t firm_ver;
        } online;

        struct
        {
            uint8_t mac[6];
            uint8_t state;
        } request_change_state;

        struct
        {
            uint8_t state;
            uint8_t my_act;
        } syn_state;

        struct
        {
            uint8_t mac[6];
            uint8_t state;
            uint32_t ver;
            uint8_t battery;
        } request_change_state_cbct;

    } data;

} ble_adv_data_t;

typedef struct
{
    uint8_t step;
    uint32_t last_time_running_config;
    uint32_t last_time_change_state_light;
    uint8_t ignore_change_state_ligth;
    uint8_t arr_tid_child[MAX_CTLK_SUPPORT];
    uint8_t is_run_config;
    uint8_t type_device_config;
    uint8_t mac[6];
    uint8_t tid;
    uint8_t my_act;
    uint8_t opp_act;
    uint32_t last_time_notify_state;
    uint32_t last_time_request_sync_led_stt;
    uint32_t light_count_down_ss;
    uint32_t time_delay_light_off;

    uint8_t my_state;
    uint8_t last_state;

    uint8_t state_await;

    uint16_t time_cnt_startup;

    uint8_t id_remove;
    struct
    {
        uint32_t is_init_ble_finished : 1;
        uint32_t send_notify_change_state : 1;
        uint32_t is_next_update_ctlk : 2;
        uint32_t need_sync_led_stt : 1;
        uint32_t active_motion_detect : 1;
        uint32_t flag_state_change : 4;
        uint32_t flag_remove_link : 1;
        uint32_t : 21;
    } flags;

} m_ble_t;

typedef struct
{
    uint8_t type;
    uint8_t mac[6];
    uint8_t my_act;
    uint8_t opp_act;
    union
    {
        struct
        {
            uint32_t firmware_version : 19;
            uint32_t battery: 7;
            uint32_t : 6;
        } info;

        uint32_t holder;
    } data;
} ble_child_t;

typedef struct
{
    uint8_t num_devices_link;
    ble_child_t list_child[MAX_CTLK_SUPPORT];
    uint32_t time_delay_light_off[MAX_CTLK_SUPPORT];
} m_ble_info_t;

int ble_master_scan(void);
void ble_config_loop(void);
void ble_go_request_pair(void);
void master_accept_request_link(void);
uint8_t ble_db_remove_child(uint8_t id);
void m_ble_get_mac_addr(uint8_t *my_mac);
void m_ble_set_state_change(uint8_t channel, uint8_t level);
void ble_db_remove_all_nodes(void);
m_ble_info_t* m_ble_get_info_reference(void);

#endif