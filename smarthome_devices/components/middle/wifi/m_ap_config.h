#ifndef AP_CONFIG_H_
#define AP_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_SSID_SCAN      20


typedef enum {
    START_APCONFIG = 0,
    SCAN_WIFI_AP,
    WAIT_WIFI_SCAN,
    START_ACCESS_POINT,
    WAIT_USER_CONFIG,
    CONNECT_WIFI,
    AP_CONFIG_DONE
} m_apconfig_step_t;


typedef struct {
    char ssid[MAX_SSID_SCAN][32];
    int8_t rssi[MAX_SSID_SCAN];
    uint32_t num_ap_available;
} m_ap_around_t;

void start_webserver(void);

m_apconfig_step_t apconfig_start(void);
void reset_ap_config(void);

bool apconfig_parse_wifi_info(char* json);

#endif 