#ifndef HNN_SECURE_H_
#define HNN_SECURE_H_

#include <stdint.h>

uint16_t encrypt_data(uint8_t *data, uint8_t len, uint8_t tid);
uint16_t decrypt_data(uint8_t *data, uint8_t len, uint8_t tid);
uint16_t calculate_checksum(uint8_t data[], uint8_t len);

#endif
