#ifndef _M_WIFI_H_
#define _M_WIFI_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "system_typedef.h"

typedef enum {
    WIFI_START = 0,
    WIFI_DISCONNECT,
    WIFI_CONNECTED,
    WIFI_SMARTCONFIG,
    WIFI_WAIT_SMARTCONFIG_DONE,
    WIFI_APCONFIG,
    WIFI_WAIT_APCONFIG_DONE,
    WIFI_APCONFIG_DONE,
    WIFI_CONFIG_DONE,
    WIFI_WAIT_EVENT,
    WIFI_DEV_ACTIVE,
} m_wifi_state_t;

typedef enum {
    WIFI_NORM_EVENT = 0, /* normal connection */
    WIFI_SC_EVENT, /* smart-config make wifi connection */
    WIFI_AP_EVENT, /* ap-config make wifi connection */
} m_wifi_event_source_t;

typedef void (*p_smart_config_start_t)(void);
typedef void (*p_ap_config_start_t)(void);
typedef void (*p_wifi_set_ssid_t)(char* ssid, size_t len);
typedef void (*p_wifi_set_passwd_t)(char* passwd, size_t len);
typedef void (*p_wifi_change_wifi_t)(char* ssid, char* passwd);
typedef void (*p_wifi_set_state_t)(m_wifi_state_t _new_state);
typedef m_wifi_state_t (*p_wifi_get_state_t)(void);
typedef void (*p_wifi_save_ip_t)(char* ip, size_t len);
typedef void (*p_wifi_set_event_source_t)(m_wifi_event_source_t event);

typedef struct {
    char                     _ssid[32];
    char                     _passwd[64];
    char                     _ip[16]; /* maximum len of IPv4 is 16 */
    int                     _rssi;
} m_wifi_info_t;

typedef struct {
/* properties */
    m_wifi_info_t               _wifi_info;
    m_wifi_event_source_t       event_source;
    m_wifi_state_t              wifi_state;
    uint32_t                    retry_count;
    uint32_t                    max_retry_count;

/* method */
    p_smart_config_start_t      start_smart_config_cb;
    p_ap_config_start_t         start_ap_config_cb;
    p_wifi_set_state_t          wifi_set_state_cb;
    p_wifi_get_state_t          wifi_get_state_cb;
    p_wifi_set_ssid_t           wifi_set_ssid_cb;
    p_wifi_set_passwd_t         wifi_set_passwd_cb;
    p_wifi_change_wifi_t        wifi_change_wifi_cb;
    p_wifi_save_ip_t            wifi_save_ip;
    p_wifi_set_event_source_t   wifi_set_source_event_cb;
} m_wifi_t;

/* public */
void m_wifi_init(void);
m_wifi_t m_wifi_get_instance(void);
m_wifi_info_t* m_wifi_get_info_reference(void);

#endif /* _M_WIFI_H_ */