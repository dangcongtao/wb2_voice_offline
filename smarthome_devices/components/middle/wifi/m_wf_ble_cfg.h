#ifndef AP_CONFIG_H_
#define AP_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cJSON.h"

void m_ble_cfg_clear_state(void);
uint8_t is_ble_cfg_done(void);
uint8_t ble_cfg_parse_data(cJSON *js_Root);

#endif 