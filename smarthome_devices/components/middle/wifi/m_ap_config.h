#ifndef AP_CONFIG_H_
#define AP_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cJSON.h"
#define WIFI_SCAN_MAX_AP      20

typedef enum {
    START_APCONFIG = 0,
    SCAN_WIFI_AP,
    WAIT_WIFI_SCAN,
    START_ACCESS_POINT,
    WAIT_USER_CONFIG,
    CONNECT_WIFI,
    AP_CONFIG_DONE
} m_apconfig_step_t;

void m_ap_cfg_clear_state(void);
uint8_t is_ap_config_done(void);

void scan_ap_around(void);
bool apconfig_parse_wifi_info(char* json);
uint8_t ble_cfg_parse_data(cJSON *js_Root);

#endif 