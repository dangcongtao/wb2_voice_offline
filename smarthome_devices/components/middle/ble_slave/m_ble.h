#ifndef _BLE_H_
#define _BLE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "system_typedef.h"


void ble_init(void);
void ble_deinit(void);
void ble_send_data(char *data, uint32_t length);
void ble_set_name(char* name, uint8_t restart_adv);

#endif