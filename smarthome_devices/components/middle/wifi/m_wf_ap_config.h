#ifndef AP_CONFIG_H_
#define AP_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cJSON.h"

void m_ap_cfg_clear_state(void);
uint8_t is_ap_config_done(void);
bool apconfig_parse_wifi_info(char* json);

#endif 