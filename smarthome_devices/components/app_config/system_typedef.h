#ifndef CJSON_KEYWORD_H__
#define CJSON_KEYWORD_H__

#include "app_config.h"
#include <stdint.h>

#define DEF_AES_KEY     "yAlaCKUYI3qr0kTd"
#define DEF_AES_IV      "QFjnL4GVODlNB0eZ"

#define CJSON_KEY_ACTION        "action"
#define CJSON_KEY_ACT_ID        "act_id"
#define CJSON_KEY_USER_ID       "u"
#define CJSON_KEY_MAC_ADDRESS   "mac"
#define CJSON_KEY_ID_DEVICE     "id"
#define CJSON_KEY_TIME          "time"
#define CJSON_KEY_URL           "url"
#define CJSON_KEY_IP            "ip"
#define CJSON_KEY_SSID          "ssid"
#define CJSON_KEY_PASS          "pass"
#define CJSON_KEY_AES_KEY       "key"
#define CJSON_KEY_AES_IV        "iv"
#define CJSON_KEY_PO            "po" /* power-on state */
#define CJSON_KEY_INDEX         "index"
#define CJSON_KEY_LOCK          "lock"
#define CJSON_KEY_T_ACT         "t_ac1"
#define CJSON_KEY_T_C           "t_c"
#define CJSON_KEY_BACKUP        "backup"
#define CJSON_KEY_MODE          "mode"
/* JSON KEY USE FOR CONFIGURATION PROCESSING */
#define CJSON_KEY_SERVER        "server"
#define CJSON_KEY_S_PORT        "port"
#define CJSON_KEY_S_USER        "user"
#define CJSON_KEY_S_PASS        "pass"
#define CJSON_KEY_TOPIC_PUB     "pub"
#define CJSON_KEY_TOPIC_SUB     "sub"


#define DEFAULT_AES_KEY "0000000000000000"
#define DEFAULT_AES_IV  "0000000000000000"

#define SIZEOF_ARRAY(array) (sizeof(array) / sizeof(array[0]))

typedef enum {
    WSWITCH = 0,
    LHSWITCH,
	WSDATIC,
    DATICBSV2,
} device_type_t;

extern uint32_t device_type;
extern uint32_t max_number_channel;
extern char* gs_dev_type[];

typedef enum {
    SYSTEM_STARTUP = 0,
    SYSTEM_ACTIVE_NORMAL,
    SYSTEM_HANDLER_OTA
} state_machine_t;

typedef enum {
    CH1_ON = 1,
    CH1_OFF = 2,
    CH2_ON = 3,
    CH2_OFF = 4,
    CH3_ON = 5,
    CH3_OFF = 6,
    CH4_ON = 7,
    CH4_OFF = 8,
} switch_action_t;

typedef enum
{
    SRC_ACTION_USER = 0,
    SRC_ACTION_ALARM = 2,
} src_action_t;

typedef enum
{
    RESULT_SUCCESS = 1,
} result_t;


typedef enum
{
    LOW = 0,
    HIGH = !LOW,
} state_t;

typedef enum {
    WIFI_IDLE = 0,
    WIFI_STATION,
    WIFI_SOFT_AP,
} m_wifi_mode_t;

typedef enum
{
    TRANSMIT_INVALID = 0,
    BLE_MSG,
    MQTT_MSG,
} method_trasmit_t;

#define DATA_MAX_LEN    512

typedef struct
{
    char data[DATA_MAX_LEN];
    uint32_t length;
    uint8_t method_trasmit;
} m_data_t;

#define BLE_DATA_MAX_LEN    256

typedef struct
{
    char data[BLE_DATA_MAX_LEN];
    uint32_t length;
    uint8_t method_trasmit;
} m_ble_data_t;

typedef enum {
    M_WIFI_START = 0,
    M_WIFI_NOT_CFG,
    M_WIFI_DISCONNECT,
    M_WIFI_CONNECTED,
    M_WIFI_CONNECTING,
    M_WIFI_SMARTCONFIG,
    M_WIFI_WAIT_SMARTCONFIG_DONE,
    M_WIFI_APCONFIG,
    M_WIFI_WAIT_APCONFIG_DONE,
    M_WIFI_APCONFIG_DONE,
    M_WIFI_CONFIG_DONE,
    M_WIFI_WAIT_EVENT,
    M_WIFI_GET_MQTT,
    M_WIFI_CONN_MQTT,
    M_WIFI_MQTT_CONNECTING,
    M_WIFI_DEV_ACTIVE,
} m_state_t;

typedef enum {
    SIG_NONE = 0,
	SIG_SMART_CFG,
    SIG_SMART_CFG_DONE,
	SIG_AP_CFG,
    SIG_AP_CFG_DONE,
	SIG_WIFI_DISCONNECT,
    SIG_WIFI_CONNECTED,
    SIG_MQTT_GOT_INFO,
    SIG_MQTT_CONNECTED,
    SIG_MQTT_DISCONNECTED,
} m_signal_t;

typedef struct {
	m_state_t context;
	m_signal_t signal;
	m_state_t next_state;
    void (*signal_handler_cb)(void * arg);
} m_state_transmit_t;

#endif /* CJSON_KEYWORD_H__ */