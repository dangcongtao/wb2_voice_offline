#ifndef _BLE_H_
#define _BLE_H_

#include <stdint.h>

void ble_init(void);
void ble_deinit(void);
void ble_send_data(char *data, uint32_t length);

#endif