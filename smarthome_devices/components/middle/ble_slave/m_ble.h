#ifndef _BLE_H_
#define _BLE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "system_typedef.h"


typedef struct __attribute__((packed))
{
    uint8_t src_mac[6];
    uint8_t cmd_id;
} encode_data_t;

typedef struct __attribute__((packed))
{
    uint8_t service_uuid[2];
    uint8_t cmd;
    uint8_t tid;
    uint16_t checksum;
    encode_data_t content;
} hunonic_voicebox_msg_t;

// void ble_init(void);
void ble_deinit(void);
bool ble_is_inited(void);
void ble_send_data(char *data, uint32_t length);
void ble_set_name(char* name, uint8_t restart_adv);

void m_ble_send_voicebox_state(uint8_t cmd_id);
void m_ble_beacon_loop(void);
void m_ble_get_mac_addr(uint8_t *my_mac);

#endif