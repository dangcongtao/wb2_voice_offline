#include "process_data.h"

#include <string.h>

#include "cJSON.h"
#include "m_database.h"
#include "app_config.h"
#include "m_mqtt.h"
#include "m_ota.h"
#include "m_switch.h"
#include "system_typedef.h"
#include "m_ble.h"
#include "m_wf_ble_cfg.h"
#include "hardware.h"

#include "aes_cbc128.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>

#include "m_http_services.h"

static char respond[512];
extern char key[16], iv[16];

static void app_startup_message(void);
static void app_control_pin(cJSON *jsRoot);
static void app_get_info(cJSON *jsRoot);
static void app_set_power_on(cJSON *jsRoot);
static void app_set_lock(cJSON *jsRoot);
static void app_set_time_release(cJSON *jsRoot);
static void app_get_time_release(cJSON *jsRoot);
static void app_set_wifi(cJSON *jsRoot);
static void app_request_ota(cJSON *jsRoot);
static void app_set_static_ip(cJSON *jsRoot);
static void app_remove_device(cJSON *jsRoot);
static void app_start_cyclic_timer(cJSON *jsRoot);
static void app_stop_cyclic_timer(cJSON *jsRoot);
static void app_backup_db(cJSON *jsRoot);

#if PROJECT_SHOCK
static void app_get_leakage(cJSON *jsRoot);
static void app_fake_leakage(cJSON *jsRoot);
#endif

static void ble_config_step1(cJSON *jsRoot);
static void ble_config_step2(cJSON *jsRoot);

wswitch_handler_t array_handler[] = {
    {app_control_pin, 0},        
    {app_start_cyclic_timer, 64},
    {app_stop_cyclic_timer, 65}, 

#if PROJECT_SHOCK
    {app_get_leakage, 71}, 
    {app_fake_leakage, 72},
#endif
    {app_get_info, 105},
    {app_set_power_on, 121},     
    {app_remove_device, 123},
    {app_set_wifi, 128},
    {app_request_ota, 129},
    {app_set_time_release, 130}, 
    {app_get_time_release, 131},
    {app_set_static_ip, 132},    
    {app_set_lock, 133},
    {app_backup_db, 151},

    {ble_config_step1, 1}, 
    {ble_config_step2, 3}, 
};

QueueHandle_t pqueue;

bool pdata_queue_init(void)
{
    if (pqueue == NULL) {
        pqueue = xQueueCreate(MAXIMUM_QUEUE_ITEM, sizeof(m_data_t));
        if(pqueue == NULL){
            os_trace("%s fail\r\n", __func__);
            exit(-1);
        }
    }

    return true;
}

void push_data_to_pqueue(m_data_t *message)
{
    if (xQueueSend(pqueue, message, pdMS_TO_TICKS(0)) != pdPASS)
    {
        os_trace("Push to queue data rx fail\r\n");
    }
}

void app_send_data(char *data, uint32_t length)
{
    m_mqtt_get_instance()._mqtt_publish(data, length);
    ble_send_data(data, length);
}

void app_send_offline_message(uint8_t src)
{
    memset(respond, 0, sizeof(respond));
    sprintf(respond,"{\"%s\":1000,\"offline\":\"device offline\", \"src\": %d}", gs_dev_type[device_type], src);
    m_mqtt_get_instance()._mqtt_publish_directly(respond, strlen(respond));
    vTaskDelay(pdMS_TO_TICKS(200));
}

static void app_startup_message(void)
{
    m_device_t app_databse = m_db_get_instance()->_m_device;
    uint32_t buffer_seek = 0, i;

    /* respond server here */
    memset(respond, 0, sizeof(respond));

    buffer_seek +=
        sprintf(respond,
                "{\"%s\": 100, \"u\":0, \"id\":\"%s\", \"ver\":%s, \"extras\":%d,"
                "\"ssid\":\"%s\", \"pass\":\"%s\",\"streng\":%d, "
                "\"ip\":\"%s\", \"po\":[",
                gs_dev_type[device_type],
                m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION, EXTRAS,
                app_databse._wifi_info->_ssid, app_databse._wifi_info->_passwd,
                app_databse._wifi_info->_rssi, app_databse._wifi_info->_ip);

    /* default state value */
    for (i = 0; i < m_db_get_number_channel(); i++)
    {
        buffer_seek += sprintf(&respond[buffer_seek], "%d,",
                               app_databse._switch_info->default_state[i]);
    }
    buffer_seek--; /* remove last comma */
    buffer_seek += sprintf(&respond[buffer_seek], "],");

    /* current state value */
    buffer_seek += sprintf(&respond[buffer_seek], "\"ps\":[");
    for (i = 0; i < m_db_get_number_channel(); i++)
    {
        buffer_seek += sprintf(&respond[buffer_seek], "%d,",
                               app_databse._switch_info->current_state[i]);
    }
    buffer_seek--; /* remove last comma */
    buffer_seek += sprintf(&respond[buffer_seek], "],");

    /* channel_lock_state */
    buffer_seek += sprintf(&respond[buffer_seek], "\"lock\":[");
    for (i = 0; i < m_db_get_number_channel(); i++)
    {
        buffer_seek += sprintf(&respond[buffer_seek], "%d,",
                               app_databse._switch_info->channel_lock_state[i]);
    }
    buffer_seek--; /* remove last comma */
    buffer_seek += sprintf(&respond[buffer_seek], "],");

    /* time_release */
    buffer_seek += sprintf(&respond[buffer_seek], "\"press_mode\":[");
    for (i = 0; i < m_db_get_number_channel(); i++)
    {
        buffer_seek += sprintf(&respond[buffer_seek], "%ld,",
                               app_databse._switch_info->time_release[i]);
    }
    buffer_seek--; /* remove last comma */
    buffer_seek += sprintf(&respond[buffer_seek], "]}");

    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

static void app_control_pin(cJSON *jsRoot)
{
    cJSON *jsAction, *jsU, *jsAct_ID;
    uint32_t type = 0, action = 0, uid = 0, act_id = 0;
    uint8_t result = 0;

    if (jsRoot == NULL)
    {
        os_trace("jsRoot NULL\n");
        goto RESPONSE;
    }

    cJSON *jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsAction = cJSON_GetObjectItem(jsRoot, CJSON_KEY_ACTION);
    if (jsAction == NULL)
    {
        os_trace("jsAction NULL\n");
        goto RESPONSE;
    }
    action = jsAction->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("jsU NULL\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_ACT_ID);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    /* Implement control switch */
    result = m_switch_get_instance()._control_switch_cb(action, act_id, uid,
                                                        true, true);

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%ld, \"action\":%ld, \"u\":%ld, \"act_id\": %ld, \"result\": %d}",
        gs_dev_type[device_type], type, action, uid, act_id, result);
    app_send_data(respond, strlen(respond));
}

/* 105 */
static void app_get_info(cJSON *jsRoot)
{
    uint32_t type = 0;

    cJSON *jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        return;
    }
    type = jsType->valueint;

    /* respond server here */
    m_device_t app_databse = m_db_get_instance()->_m_device;
    uint32_t buffer_seek = 0, i;

    /* respond server here */
    memset(respond, 0, sizeof(respond));

    buffer_seek +=
        sprintf(respond,
                "{\"%s\":%ld, \"u\":0, \"id\":\"%s\", \"ver\":%s, "
                "\"extras\":%d,\"ssid\":\"%s\",\"pass\":\"%s\",\"streng\":%d, "
                "\"ip\":\"%s\", \"po\":[",
                gs_dev_type[device_type], type,
                m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION, EXTRAS,
                app_databse._wifi_info->_ssid, app_databse._wifi_info->_passwd,
                app_databse._wifi_info->_rssi, app_databse._wifi_info->_ip);

    /* default state value */
    for (i = 0; i < m_db_get_number_channel(); i++)
    {
        buffer_seek += sprintf(&respond[buffer_seek], "%u,",
                               app_databse._switch_info->default_state[i]);
    }
    buffer_seek--; /* remove last comma */
    buffer_seek += sprintf(&respond[buffer_seek], "],");

    /* current state value */
    buffer_seek += sprintf(&respond[buffer_seek], "\"ps\":[");
    for (i = 0; i < m_db_get_number_channel(); i++)
    {
        buffer_seek += sprintf(&respond[buffer_seek], "%u,",
                               app_databse._switch_info->current_state[i]);
    }
    buffer_seek--; /* remove last comma */
    buffer_seek += sprintf(&respond[buffer_seek], "],");

    /* channel_lock_state */
    buffer_seek += sprintf(&respond[buffer_seek], "\"lock\":[");
    for (i = 0; i < m_db_get_number_channel(); i++)
    {
        buffer_seek += sprintf(&respond[buffer_seek], "%d,",
                               app_databse._switch_info->channel_lock_state[i]);
    }
    buffer_seek--; /* remove last comma */
    buffer_seek += sprintf(&respond[buffer_seek], "],");

    /* time_release */
    buffer_seek += sprintf(&respond[buffer_seek], "\"press_mode\":[");
    for (i = 0; i < m_db_get_number_channel(); i++)
    {
        buffer_seek += sprintf(&respond[buffer_seek], "%ld,",
                               app_databse._switch_info->time_release[i]);
    }
    buffer_seek--; /* remove last comma */
    buffer_seek += sprintf(&respond[buffer_seek], "]}");

    app_send_data(respond, strlen(respond));
}

/* {"TYPE":121, "po":po, "u":uid} */
static void app_set_power_on(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsPo;
    uint32_t type = 0, uid = 0, po = 0;

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsPo = cJSON_GetObjectItem(jsRoot, CJSON_KEY_PO);
    if (jsPo == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    po = jsPo->valueint;

    if (po > SWITCH_FOUR_DEFAULT_OFF)
    {
        goto RESPONSE;
    }

    uint8_t index = po / 3;
    m_db_get_instance()->_set_default_state_cb(index, po);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld,\"u\":%ld,\"po\":%ld, \"result\": %d}",
            gs_dev_type[device_type], type, uid, po, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":133, "index": i, "lock": lock,"u":uid} */
static void app_set_lock(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsIndex, *jsLock;
    uint32_t type = 0, uid = 0, index = 0, lock = 0;

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON uid mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsIndex = cJSON_GetObjectItem(jsRoot, CJSON_KEY_INDEX);
    if (jsIndex == NULL)
    {
        os_trace("Get JSON index mqtt message fail\n");
        goto RESPONSE;
    }
    index = jsIndex->valueint;

    jsLock = cJSON_GetObjectItem(jsRoot, CJSON_KEY_LOCK);
    if (jsLock == NULL)
    {
        os_trace("Get JSON lock mqtt message fail\n");
        goto RESPONSE;
    }
    lock = jsLock->valueint;

    m_db_get_instance()->_set_lock_channel_cb(index, lock);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond,
            "{\"%s\":%ld,\"u\":%ld,\"index\":%ld,\"lock\":%ld, \"result\": %d}",
            gs_dev_type[device_type], type, uid, index, lock, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":130,"u":uid, "index":i,"time":s} */
static void app_set_time_release(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsIndex, *jsTime, *jsMode;
    uint32_t type = 0, uid = 0, index = 0, time = 0, mode = 0;

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsIndex = cJSON_GetObjectItem(jsRoot, CJSON_KEY_INDEX);
    if (jsIndex == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    index = jsIndex->valueint;

    jsTime = cJSON_GetObjectItem(jsRoot, CJSON_KEY_TIME);
    if (jsTime == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    time = jsTime->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_MODE);
    if (jsMode == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    m_db_get_instance()->_set_time_release_cb(index, time, mode);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond,
            "{\"%s\":%ld,\"u\":%ld,\"index\":%ld, \"time\":%ld, \"mode\": %ld, "
            "\"result\": %d}",
            gs_dev_type[device_type], type, uid, index, time, mode, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"atmwifi":131, "u":1,"index":0} */
static void app_get_time_release(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsIndex;
    uint32_t type = 0, uid = 0, index = 0, time = 0, mode = 0;

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsIndex = cJSON_GetObjectItem(jsRoot, CJSON_KEY_INDEX);
    if (jsIndex == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    index = jsIndex->valueint;

    /* implement */
    m_db_get_instance()->_get_time_release_cb(index, &time, &mode);

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond,
            "{\"%s\":%ld,\"u\":%ld,\"index\":%ld, \"time\":%ld, \"mode\": %ld, "
            "\"result\": %d}",
            gs_dev_type[device_type], type, uid, index, time, mode, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":128, "ssid":"Wifi Name", "pass":"Wifi Password", "u":uid} */
static void app_set_wifi(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsSsid, *jsPass;
    uint32_t type = 0, uid = 0;
    char *ssid = NULL, *pass = NULL;

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsSsid = cJSON_GetObjectItem(jsRoot, CJSON_KEY_SSID);
    if (jsSsid == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    ssid = jsSsid->valuestring;

    jsPass = cJSON_GetObjectItem(jsRoot, CJSON_KEY_PASS);
    if (jsPass == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    pass = jsPass->valuestring;

    /* implement */
    m_wifi_get_instance().wifi_change_wifi_cb(ssid, pass);

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond,
            "{\"%s\":%ld, \"ssid\":\"%s\", \"pass\":\"%s\", \"u\": %ld, "
            "\"result\": %d}",
            gs_dev_type[device_type], type, ssid, pass, uid, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":129, "url":url,"u":uid} */
static void app_request_ota(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsUrl;
    uint32_t type = 0, uid = 0;
    char *url;

    os_trace("Get request OTA\r\n");

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\r\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\r\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsUrl = cJSON_GetObjectItem(jsRoot, CJSON_KEY_URL);
    if (jsUrl == NULL)
    {
        os_trace("Get JSON type mqtt message fail\r\n");
        goto RESPONSE;
    }
    url = jsUrl->valuestring;

    /* respond server here */
    result = 1;
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"result\": %d}",
            gs_dev_type[device_type], type, uid, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));

    /* implement here */
    extern void ble_deinit(void);
    ble_deinit();
    m_ota_start(url);

    return;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"result\": %d}",
            gs_dev_type[device_type], type, uid, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":132, "ip":"192.168.1.100"} */
static void app_set_static_ip(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsIp;
    uint32_t type = 0, uid = 0;
    char *ip = "";

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsIp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IP);
    if (jsIp == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    ip = jsIp->valuestring;

    /* implement here */
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\": %ld, \"ip\":\"%s\", \"result\": %d}",
            gs_dev_type[device_type], type, uid, ip, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":123, "u":uid} */
static void app_remove_device(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU;
    uint32_t type = 0, uid = 0;

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    /* implement here */
    app_send_offline_message(2);
    /* factory reset database */
    m_db_get_instance()->_reset_db_cb();
    m_wifi_get_instance().start_smart_config_cb();
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"result\": %d}",
            gs_dev_type[device_type], type, uid, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":64, "action":A,"t_act":T,"t_c":C} */
static void app_start_cyclic_timer(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsAction, *jsTact, *jsTc;
    uint32_t type = 0, uid = 0, action = 0, time_act1 = 0, time_cycle = 0;

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsAction = cJSON_GetObjectItem(jsRoot, CJSON_KEY_ACTION);
    if (jsAction == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    action = jsAction->valueint;

    jsTact = cJSON_GetObjectItem(jsRoot, CJSON_KEY_T_ACT);
    if (jsTact == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    time_act1 = jsTact->valueint;

    jsTc = cJSON_GetObjectItem(jsRoot, CJSON_KEY_T_C);
    if (jsTc == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    time_cycle = jsTc->valueint;

    /* implement here */
    uint32_t time_act2 = time_cycle - time_act1;
    start_cylcic_timer(action, time_act1, time_act2);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond,
            "{\"%s\":%ld, \"u\":%ld, \"action\": %ld, \"t_act\": %ld, \"t_c\": %ld, "
            "\"result\": %d}",
            gs_dev_type[device_type], type, uid, action, time_act1, time_cycle,
            result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":65, "index":index} */
static void app_stop_cyclic_timer(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsIndex;
    uint32_t type = 0, uid = 0, index = 0;

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsIndex = cJSON_GetObjectItem(jsRoot, CJSON_KEY_INDEX);
    if (jsIndex == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    index = jsIndex->valueint;

    /* implement here */
    os_trace("app_stop_cyclic_timer index %ld\n", index);
    stop_cylcic_timer(index);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"index\": %ld, \"result\": %d}",
            gs_dev_type[device_type], type, uid, index, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":151, "u":uid, “backup”: 1} */
static void app_backup_db(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType, *jsU, *jsBackup;
    uint32_t type = 0, uid = 0, backup = 0;

    os_trace("Back up database handler\n");

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsBackup = cJSON_GetObjectItem(jsRoot, CJSON_KEY_BACKUP);
    if (jsBackup == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    backup = jsBackup->valueint;

    /* implement here */
    m_db_get_instance()->_backup_db_cb();

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"backupt\": %ld, \"result\":%d}",
            gs_dev_type[device_type], type, uid, backup, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

#if PROJECT_SHOCK
static void app_get_leakage(cJSON *jsRoot)
{
    cJSON *jsType;
    uint32_t type = 0;

    os_trace("app_get_leakage get\n");

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;


    /* implement here */

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"%s\":%d}",
            gs_dev_type[device_type], type, CJSON_KEY_LEAKAGE, is_leakage_current());
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

static void app_fake_leakage(cJSON *jsRoot)
{
    cJSON *jsType, *jsU;
    uint32_t type = 0, uid = 0;
    uint8_t result = 0;

    os_trace("app_get_leakage get\n");

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;


    /* implement here */
    fake_current_leak();
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"result\":%d}",
            gs_dev_type[device_type], type, uid, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}
#endif

/* 
    @jsRoot: will be NULL, not required 
*/
static void ble_config_step1(cJSON *jsRoot)
{
    m_ble_config_step1();
}

/* {"cmd":3,"pub":"u/15/HUN167291413297wswitch2vN1/1673509108/ok",
    "sub":"u/15/HUN167291413297wswitch2vN1/1673509108",
    "ssid":"RD HUNONIC 1","pass":"66668888"} */
static void ble_config_step2(cJSON *jsRoot)
{
    ble_cfg_parse_data(jsRoot);

    m_ble_config_step2();
}

void app_handler_msg_task(void * arg)
{
    m_data_t msg;
    memset(&msg, 0, sizeof(msg));
    char *root_type = NULL;
    uint32_t raw_len = 0;
    char raw_data[MAXIMUM_SIZE_MESSAGE];
    cJSON *jsRoot = NULL, *jsType = NULL;

    while (1) 
    {
        if (xQueueReceive(pqueue, &msg, 1) == pdPASS) 
        {
            // os_trace("raw_data received: %s\r\n", msg.data);

            /* decode data */
            memset(raw_data, 0, sizeof(raw_data));
            aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
            if (aes_cbc128_decrypt((uint8_t *)msg.data, msg.length, (uint8_t *)raw_data, (size_t *)&raw_len) != AES_SUCCESS)
            {
                os_trace("Can not decrypt message\r\n");
                continue;
            }
            
            printf("\r\ndecoded: %s\r\n", raw_data);

            /* find handler of data */
            jsRoot = cJSON_Parse(raw_data);
            if (!jsRoot) {
                os_trace("Parse Json Fail\r\n");
                continue;
            }

            root_type = gs_dev_type[device_type];
            jsType = cJSON_GetObjectItem(jsRoot, root_type);
            if (jsType == NULL)
            {
                root_type = "cmd";
                jsType = cJSON_GetObjectItem(jsRoot, root_type);
                if (jsType == NULL)
                {
                    os_trace("Parse JSON get type fail\n");
                    cJSON_Delete(jsRoot);
                    continue;
                }
            }
            uint32_t cmd = jsType->valueint;

            uint32_t arr_size = sizeof(array_handler) / sizeof(array_handler[0]);
            os_trace("arr_size: %lu\r\n", arr_size);

            for (uint32_t i = 0; i < arr_size; i++)
            {
                if (array_handler[i]._cmd_number == cmd)
                {
                    os_trace("_cmd_number: %ld\r\n", cmd);
                    array_handler[i]._handler(jsRoot);
                    break;
                }
            }

            cJSON_Delete(jsRoot);
        }

        m_mqtt_get_instance()._mqtt_loop();
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void process_data_init(void)
{
    pdata_queue_init();
    m_mqtt_command_startup_register_handler(app_startup_message);

    xTaskCreate(app_handler_msg_task, "ble slave", 1024 * 3, NULL, 15, NULL);
}