#ifndef _M_WIFI_H_
#define _M_WIFI_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "system_typedef.h"

typedef void (*p_wifi_sync_rssi_t)(void);
typedef void (*p_smart_config_start_t)(void);
typedef void (*p_ap_config_start_t)(void);
typedef void (*p_wifi_set_ssid_t)(char* ssid, size_t len);
typedef void (*p_wifi_set_passwd_t)(char* passwd, size_t len);
typedef void (*p_wifi_change_wifi_t)(char* ssid, char* passwd);
typedef void (*p_wifi_save_ip_t)(char* ip, size_t len);

typedef void (*p_wifi_sta_start_t)(char* ssid, char* passwd);
typedef void (*p_wifi_sta_stop_t)(void);
typedef void (*p_wifi_ap_start_t)(void);
typedef void (*p_wifi_ap_stop_t)(void);
typedef void (*p_wifi_scan_t)(void);

typedef struct {
    char                     _ssid[32];
    char                     _passwd[64];
    char                     _ip[16]; /* maximum len of IPv4 is 16 */
    int                     _rssi;
} m_wifi_info_t;

typedef struct {
/* properties */
    m_wifi_info_t               _wifi_info;

/* method */
    p_wifi_set_ssid_t           wifi_set_ssid_cb;
    p_wifi_set_passwd_t         wifi_set_passwd_cb;
    p_wifi_change_wifi_t        wifi_change_wifi_cb;
    p_wifi_save_ip_t            wifi_save_ip;
    p_wifi_sync_rssi_t          wifi_sync_rssi_cb;

    p_ap_config_start_t         start_ap_config_cb;
    p_smart_config_start_t      start_smart_config_cb;
    
    p_wifi_scan_t               wifi_scan_cb;
    p_wifi_ap_stop_t            wifi_ap_stop_cb;
    p_wifi_ap_start_t           wifi_ap_start_cb;
    p_wifi_sta_stop_t           wifi_sta_stop_cb;
    p_wifi_sta_start_t          wifi_sta_start_cb;
} m_wifi_t;

/* public */
void m_wifi_init(void);
void m_wifi_start(void);

m_wifi_t m_wifi_get_instance(void);
m_wifi_info_t* m_wifi_get_info_reference(void);

#endif /* _M_WIFI_H_ */