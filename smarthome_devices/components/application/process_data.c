#include "process_data.h"
#include "aiio_ci13162_uart.h"

#include <string.h>

#include "cJSON.h"
#include "base64.h"
#include "m_database.h"
#include "aiio_ci13162_config.h"
#include "app_config.h"
#include "m_mqtt.h"
#include "m_ota.h"
#include "m_ci13162_ota.h"
#include "m_ci13162_ota_boot.h"
#include "ci13162_ota_manifest.h"
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
#include "bl_sys.h"
#include "IRremoteWB2.h"
#include "IRsend.h"
#include "IRrecv.h"
#include "hardware.h"
#include "event_driven.h"
#include "m_fan_event.h"
#include "app_ir.h"
#include "flash_api.h"
#include "m_fan_control.h"
#include "app_fan.h"
#include "app_cwsl.h"
#include "app_volume.h"

char respond[512];
extern char key[16], iv[16];
extern uint32_t val_temperature;
extern uint8_t bat_percent;
extern uint8_t bat_charging_status;
extern uint8_t is_ota;
static int previous_otp;

static void app_startup_message(void);
static void app_get_info(cJSON *jsRoot);
static void app_get_fan_info(cJSON *jsRoot);
static void app_set_wifi(cJSON *jsRoot);
static void app_request_ota(cJSON *jsRoot);
static void app_request_ci13162_ota(cJSON *jsRoot);
static void app_set_static_ip(cJSON *jsRoot);
static void app_remove_device(cJSON *jsRoot);
static void app_backup_db(cJSON *jsRoot);
static void app_set_mqtt_info(cJSON *jsRoot);

// fan
static void app_set_time_release(cJSON *jsRoot);
static void app_get_time_release(cJSON *jsRoot);
static void app_control_fan(cJSON *js_Root);
static void app_control_fan_with_one(cJSON *js_Root);
static void app_control_swing(cJSON *js_Root);
static void app_control_timer(cJSON *js_Root);
static void app_set_buzz(cJSON *js_Root);
static void app_set_light(cJSON *js_Root);
static void app_set_power_saving(cJSON *js_Root);
static void app_set_auto_threshold(cJSON *js_Root);
static void app_get_auto_threshold(cJSON *js_Root);

// IR
extern void ir_control(cJSON *jsRoot);
extern void ir_start_learn(cJSON *jsRoot);
extern void ir_stop_learn(cJSON *jsRoot);
extern void ir_start_learn_auto(cJSON *jsRoot);
extern void ir_stop_learn_auto(cJSON *jsRoot);

// ble
static void ble_config_step1(cJSON *jsRoot);
static void ble_config_step2(cJSON *jsRoot);
static void app_ble_discover(cJSON *js_root);
static void app_ble_authen_connection(cJSON *js_root);

static void app_change_device_id(cJSON *js_root);

wswitch_handler_t array_handler[] = {
    {app_change_device_id, 322},
    {app_set_mqtt_info, 990},
    {app_get_info, 105},
    {app_get_fan_info, 107},
    // {app_set_power_on, 121},
    {app_remove_device, 123},
    {app_set_wifi, 128},
    {app_request_ota, 129},
    {app_request_ci13162_ota, 134},
    {app_set_time_release, 130},
    {app_get_time_release, 131},
    {app_set_static_ip, 132},
    // {app_set_lock, 133},
    {app_backup_db, 151},

    {ir_control, 1},
    {ir_start_learn, 2},
    {ir_stop_learn, 4},
    {ir_start_learn_auto, 15},
    {ir_stop_learn_auto, 16},

    // fan
    {app_control_fan, 20},
    {app_control_fan_with_one, 21},
    // {app_control_swing, 21},
    // {app_control_timer, 22},
    {app_set_buzz, 24},
    {app_set_light, 25},
    {app_set_power_saving, 26},
    {app_set_auto_threshold, 27},
    {app_get_auto_threshold, 28},

    {app_ble_discover, 35},
    {app_ble_authen_connection, 36},

    {app_cwsl_learn, 220},
    {app_cwsl_delete, 221},
    {app_cwsl_stop, 222},
    {app_cwsl_query, 223},
    {app_volume_set, 225},
    {app_volume_get, 226},
};

wswitch_handler_t array_ble_handler[] = {
    {ble_config_step1, 1},
    {ble_config_step2, 3},
};

QueueHandle_t pqueue;
// extern m_data_t message_rcv_mqtt;

bool pdata_queue_init(void)
{
    if (pqueue == NULL)
    {
        pqueue = xQueueCreate(MAXIMUM_QUEUE_ITEM, sizeof(m_data_t));
        if (pqueue == NULL)
        {
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
    sprintf(respond, "{\"%s\":1000,\"offline\":\"device offline\", \"src\": %d}", gs_dev_type[device_type], src);
    m_mqtt_get_instance()._mqtt_publish_directly(respond, strlen(respond));
    vTaskDelay(pdMS_TO_TICKS(200));
}

extern uint8_t last_reason_reset;
static void app_startup_message(void)
{
    m_device_t app_databse = m_db_get_instance()->_m_device;
    static uint32_t num_send = 0;
    m_wifi_get_instance().wifi_sync_rssi_cb();
    /* respond server here */
    memset(respond, 0, sizeof(respond));

    sprintf(respond,
            "{\"%s\":100,\"reason\":%lu,\"id\":\"%s\",\"ver\":%s,\"voice_ver\":%lu,"
            "\"ssid\":\"%s\",\"pass\":\"%s\",\"streng\":%d,"
            "\"ip\":\"%s\",\"vermqtt\":%lu,\"mode\":%u,\"lv\":%u,\"pcn\":%u,\"swing\":%u,"
            "\"buzz\":%u,\"t\":%lu,\"temp\":%lu,\"lamp\":%u,\"lp\":%u,\"bat\":%u,\"charging\":%u}",
            gs_dev_type[device_type], num_send * 100U + last_reason_reset * 10U + (uint32_t)bl_sys_rstinfo_get_hun(), // hang chuc: flash_reason reset, hang don vi: BL_RST_REASON_E
            m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION,
            (unsigned long)aiio_ci13162_uart_get_voice_version(),
            app_databse._wifi_info->_ssid, app_databse._wifi_info->_passwd,
            app_databse._wifi_info->_rssi, app_databse._wifi_info->_ip, app_databse._mqtt_info->vermqtt,
            app_databse._fan_info->mode, app_databse._fan_info->speed, app_databse._fan_info->pcn, app_databse._fan_info->swing,
            !app_databse._fan_info->disable_buzz, m_fan_get_time_remain(), val_temperature, app_databse._fan_info->light, app_databse._fan_info->power_save,
            bat_percent, bat_charging_status);

    num_send++;

    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

extern char temp1[512];
extern char temp2[512];
extern bool save_mqtt_info(char *json);
static void app_set_mqtt_info(cJSON *js_Root)
{
    uint8_t result = 0;
    cJSON *jsType;
    uint32_t type = 990;

    if (!js_Root)
    {
        return;
    }

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    cJSON *jsGetObject = cJSON_GetObjectItem(js_Root, "data");
    if (!jsGetObject)
    {
        os_trace("Get JSON data message fail\n");
        goto RESPONSE;
    }
    memset(temp1, 0, sizeof(temp1));
    snprintf(temp1, sizeof(temp1), "%s", jsGetObject->valuestring);

    /* Decode base64: @temp1: input-data, @temp2: output-data */
    uint16_t base64_len;
    memset(temp2, 0, sizeof(temp2));
    base64_len = b64_decode((uint8_t *)temp1, strlen(temp1), (uint8_t *)temp2);

    /* Decode AES:  @temp2: input-data, @temp1: output-data */
    size_t real_msg_len = 0;
    memset(temp1, 0, sizeof(temp1));
    // aes_cbc128_init((uint8_t*)DEF_AES_KEY, (uint8_t*)DEF_AES_IV);
    aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
    if (aes_cbc128_decrypt((uint8_t *)temp2, base64_len, (uint8_t *)temp1, &real_msg_len) != AES_SUCCESS)
    {
        goto RESPONSE;
    }

    /* save */
    printf("mqtt_info[%d]: [%s]\r\n", strlen(temp1), temp1);
    if (save_mqtt_info(temp1))
    {
        result = 1;
    }

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"result\":%d}", gs_dev_type[device_type], type, result);
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
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsTime = cJSON_GetObjectItem(jsRoot, CJSON_KEY_TIME);
    if (jsTime == NULL)
    {
        os_trace("Get JSON jsTime mqtt message fail\n");
        goto RESPONSE;
    }
    time = jsTime->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_MODE);
    if (jsMode == NULL)
    {
        os_trace("Get JSON jsMode mqtt message fail\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsIndex = cJSON_GetObjectItem(jsRoot, CJSON_KEY_INDEX);
    if (jsIndex == NULL)
    {
        os_trace("Get JSON jsIndex mqtt message fail\n");
        goto RESPONSE;
    }
    index = jsIndex->valueint;

    m_fan_set_time_off(time);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond,
            "{\"%s\":%ld,\"u\":%ld,\"index\":%ld,\"time\":%ld,\"mode\":%ld,"
            "\"result\": %d}",
            gs_dev_type[device_type], type, uid, index, time, mode, result);
    app_send_data(respond, strlen(respond));
}

void app_response_time_release(uint32_t uid, uint8_t result)
{
    uint32_t time = m_fan_get_time_set();
    memset(respond, 0, sizeof(respond));
    sprintf(respond,
            "{\"%s\":131,\"u\":%ld,\"index\":0,\"time\":%ld,\"mode\":1,"
            "\"result\": %d}",
            gs_dev_type[device_type], uid, time, result);
    app_send_data(respond, strlen(respond));
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
    time = m_fan_get_time_set();
    result = 1;

RESPONSE:
    /* respond server here */
    app_response_time_release(uid, result);
}

// {"type":20, "mode":1,"lv":1, "u":55519, "act_id":0}
static void app_control_fan(cJSON *js_Root)
{
    cJSON *jsMode, *jsLevel, *jsPcn, *jsLamp, *jsU, *jsAct, *jsType, *jsSwing;
    uint8_t mode = 0, level = 0, swing = 0, pcn = 0, lamp = 0, result = 0, change = 0;
    uint32_t uid = 0, act_id = 0, type = 0;
    m_fan_info_t *my_fan = m_fan_get_instance();

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsMode = cJSON_GetObjectItem(js_Root, "mode");
    if (jsMode == NULL)
    {
        os_trace("Get JSON jsMode mqtt message fail\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsLevel = cJSON_GetObjectItem(js_Root, "lv");
    if (jsLevel == NULL)
    {
        os_trace("Get JSON jsLevel mqtt message fail\n");
        goto RESPONSE;
    }
    level = jsLevel->valueint;

    jsSwing = cJSON_GetObjectItem(js_Root, "swing");
    if (jsSwing == NULL)
    {
        os_trace("Get JSON jsSwing mqtt message fail\n");
        goto RESPONSE;
    }
    swing = jsSwing->valueint;

    jsPcn = cJSON_GetObjectItem(js_Root, "pcn");
    if (jsPcn != NULL)
    {
        pcn = jsPcn->valueint;
    }

    jsLamp = cJSON_GetObjectItem(js_Root, "lamp");
    if (jsLamp != NULL)
    {
        lamp = jsLamp->valueint;
    }

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsAct = cJSON_GetObjectItem(js_Root, CJSON_KEY_ACT_ID);
    if (jsAct == NULL)
    {
        os_trace("Get JSON jsAct mqtt message fail\n");
        goto RESPONSE;
    }
    act_id = jsAct->valueint;

    // m_fan_set_light(lamp);
    m_fan_control(mode, level, pcn, swing, lamp, uid, act_id, 0);
    change = check_fan_change(mode, level, pcn, swing, lamp);

    if (mode != M_FAN_MODE_PCN)
    {
        switch (level)
        {
        case 1:
            my_fan->pcn = 33;
            break;
        case 2:
            my_fan->pcn = 66;
            break;
        case 3:
            my_fan->pcn = 100;
            break;
        case 0:
            my_fan->pcn = 0;
            break;
        default:
            my_fan->pcn = 0;
            break;
        }
    }

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld,\"mode\":%u,\"lv\":%u,\"swing\":%u,\"pcn\":%u,\"lamp\":%u,\"u\":%lu,\"act_id\":%lu,\"change\":%u,\"result\":%d}", 
        gs_dev_type[device_type], type, my_fan->mode, my_fan->speed, my_fan->swing, my_fan->pcn, my_fan->light, uid, act_id, change, result);
    app_send_data(respond, strlen(respond));
}

// {"type":21, "mode":1,"lv":1, "u":55519, "act_id":0}
static void app_control_fan_with_one(cJSON *js_Root)
{
    cJSON *jsMode, *jsLevel, *jsU, *jsAct, *jsType, *jsSwing, *jsPcn, *jsLamp;
    uint8_t mode = 0, level = 0, swing = 0, pcn = 0, lamp = 0, change = 0, result = 0;
    uint32_t uid = 0, act_id = 0, type = 0;
    m_fan_info_t *my_fan = m_fan_get_instance();

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsAct = cJSON_GetObjectItem(js_Root, CJSON_KEY_ACT_ID);
    if (jsAct == NULL)
    {
        os_trace("Get JSON jsAct mqtt message fail\n");
        goto RESPONSE;
    }
    act_id = jsAct->valueint;

    jsMode = cJSON_GetObjectItem(js_Root, "mode");
    if (jsMode != NULL)
    {
        mode = jsMode->valueint;
        os_trace("Get JSON jsMode mqtt message\n");
        m_fan_need_buzz(1);
        m_fan_set_speed(mode, M_FAN_SPEED_INVALID);
    }

    jsLevel = cJSON_GetObjectItem(js_Root, "lv");
    if (jsLevel != NULL)
    {
        os_trace("Get JSON jsLevel mqtt message\n");
        level = jsLevel->valueint;
        m_fan_need_buzz(1);
        m_fan_set_speed(M_FAN_MODE_INVALID, level);
    }

    jsSwing = cJSON_GetObjectItem(js_Root, "swing");
    if (jsSwing != NULL)
    {
        os_trace("Get JSON jsSwing mqtt message\n");
        swing = jsSwing->valueint;
        m_fan_need_buzz(1);
        m_fan_set_swing(swing);
    }

    jsPcn = cJSON_GetObjectItem(js_Root, "pcn");
    if (jsPcn != NULL)
    {
        os_trace("Get JSON jsPcn mqtt message\n");
        pcn = jsPcn->valueint;
        m_fan_need_buzz(1);
        m_fan_set_pcn(pcn);
    }

    jsLamp = cJSON_GetObjectItem(js_Root, "lamp");
    if (jsLamp != NULL)
    {
        os_trace("Get JSON jsLamp mqtt message\n");
        lamp = jsLamp->valueint;
        m_fan_need_buzz(1);
        m_fan_set_light(lamp);
    }

    change = check_fan_change(my_fan->mode, my_fan->speed, my_fan->pcn, my_fan->swing, my_fan->light);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":20,\"mode\":%u,\"lv\":%u,\"swing\":%u,\"pcn\":%u,\"lamp\":%u,\"u\":%lu,\"act_id\":%lu,\"change\":%u,\"result\":%d}", gs_dev_type[device_type], my_fan->mode, my_fan->speed, my_fan->swing, my_fan->pcn, my_fan->light, uid, act_id, change, result);
    app_send_data(respond, strlen(respond));
}

//{"type":21, "swing":1,"u":55519, "act_id":0}
static void app_control_swing(cJSON *js_Root)
{
    cJSON *jsSwing, *jsU, *jsAct, *jsType;
    uint8_t swing = 0, result = 0;
    uint32_t uid = 0, act_id = 0, type = 0;

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsSwing = cJSON_GetObjectItem(js_Root, "swing");
    if (jsSwing == NULL)
    {
        os_trace("Get JSON jsSwing mqtt message fail\n");
        goto RESPONSE;
    }
    swing = jsSwing->valueint;

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    m_fan_set_swing(swing);

    result = 1;
RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld,\"swing\":%u,\"u\":%lu,\"act_id\":%lu,\"result\":%d}", gs_dev_type[device_type], type, swing, uid, act_id, result);
    app_send_data(respond, strlen(respond));
}

// {"type":22, "time":120,"u":55519, "act_id":0}
static void app_control_timer(cJSON *js_Root)
{
    cJSON *jsTime, *jsU, *jsAct, *jsType;
    uint8_t result = 0;
    uint32_t uid = 0, act_id = 0, time = 0, type = 0;

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsTime = cJSON_GetObjectItem(js_Root, "time");
    if (jsTime == NULL)
    {
        os_trace("Get JSON jsTime mqtt message fail\n");
        goto RESPONSE;
    }
    time = jsTime->valueint;

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    m_fan_set_time_off(time);

    result = 1;
RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%lu,\"time\":%lu,\"u\":%lu,\"act_id\":%lu,\"result\":%u}", gs_dev_type[device_type], type, time, uid, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void app_set_buzz(cJSON *js_Root)
{
    cJSON *jsBuzz, *jsU, *jsAct, *jsType;
    uint8_t result = 0;
    uint32_t uid = 0, act_id = 0, buzz = 0, type = 0;

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsBuzz = cJSON_GetObjectItem(js_Root, "buzz");
    if (jsBuzz == NULL)
    {
        os_trace("Get JSON jsBuzz mqtt message fail\n");
        goto RESPONSE;
    }
    buzz = jsBuzz->valueint;

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    m_fan_set_buzz_disable(!buzz);

    result = 1;
RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%lu,\"buzz\":%lu,\"u\":%lu,\"act_id\":%lu,\"result\":%u}", gs_dev_type[device_type], type, buzz, uid, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void app_set_light(cJSON *js_Root)
{
    cJSON *jsBuzz, *jsU, *jsAct, *jsType;
    uint8_t result = 0;
    uint32_t uid = 0, act_id = 0, lamp = 0, type = 0;

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsBuzz = cJSON_GetObjectItem(js_Root, "lamp");
    if (jsBuzz == NULL)
    {
        os_trace("Get JSON jsBuzz mqtt message fail\n");
        goto RESPONSE;
    }
    lamp = jsBuzz->valueint;

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    m_fan_set_light(lamp);

    result = 1;
RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%lu,\"lamp\":%lu,\"u\":%lu,\"act_id\":%lu,\"result\":%u}", gs_dev_type[device_type], type, lamp, uid, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void app_set_power_saving(cJSON *js_Root)
{
    cJSON *jsPower, *jsU, *jsAct, *jsType;
    uint8_t result = 0;
    uint32_t uid = 0, power_save = 0, type = 0;

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsPower = cJSON_GetObjectItem(js_Root, "lp");
    if (jsPower == NULL)
    {
        os_trace("Get JSON jsPower mqtt message fail\n");
        goto RESPONSE;
    }
    power_save = jsPower->valueint;

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    m_fan_set_powersaving(power_save);

    result = 1;
RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%lu,\"lp\":%lu,\"u\":%lu,\"result\":%u}", gs_dev_type[device_type], type, power_save, uid, result);
    app_send_data(respond, strlen(respond));
}

static void app_set_auto_threshold(cJSON *js_Root)
{
    cJSON *jsAutoEnable, *jsU, *jsAct, *jsType, *jsThresh1, *jsThresh2, *jsThresh3;
    uint8_t result = 0, auto_enable = 0;
    uint32_t uid = 0, type = 0, thresh1 = 0, thresh2 = 0, thresh3 = 0;

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsAutoEnable = cJSON_GetObjectItem(js_Root, "auto");
    if (jsAutoEnable == NULL)
    {
        os_trace("Get JSON jsAutoEnable mqtt message fail\n");
        goto RESPONSE;
    }
    auto_enable = jsAutoEnable->valueint;

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    jsThresh1 = cJSON_GetObjectItem(js_Root, "thresh1");
    if (jsThresh1 == NULL)
    {
        os_trace("Get JSON jsThresh1 mqtt message fail\n");
        goto RESPONSE;
    }
    thresh1 = jsThresh1->valueint;

    jsThresh2 = cJSON_GetObjectItem(js_Root, "thresh2");
    if (jsThresh2 == NULL)
    {
        os_trace("Get JSON jsThresh2 mqtt message fail\n");
        goto RESPONSE;
    }
    thresh2 = jsThresh2->valueint;

    jsThresh3 = cJSON_GetObjectItem(js_Root, "thresh3");
    if (jsThresh3 == NULL)
    {
        os_trace("Get JSON jsThresh3 mqtt message fail\n");
        goto RESPONSE;
    }
    thresh3 = jsThresh3->valueint;

    // m_fan_set_powersaving(power_save);
    m_fan_set_auto_mode_settings(auto_enable, thresh1, thresh2, thresh3);

    result = 1;
RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%lu,\"u\":%lu,\"auto\":%u,\"thresh1\":%lu,\"thresh2\":%lu,\"thresh3\":%lu,\"result\":%u}", gs_dev_type[device_type], type, uid, auto_enable, thresh1, thresh2, thresh3, result);
    app_send_data(respond, strlen(respond));
}

static void app_get_auto_threshold(cJSON *js_Root)
{
    cJSON *jsAutoEnable, *jsU, *jsAct, *jsType;
    uint8_t result = 0, auto_enable = 0;
    uint32_t uid = 0, type = 0, thresh1 = 0, thresh2 = 0, thresh3 = 0;

    jsType = cJSON_GetObjectItem(js_Root, gs_dev_type[device_type]);
    if (jsType == NULL)
    {
        os_trace("Get JSON type mqtt message fail\n");
        goto RESPONSE;
    }
    type = jsType->valueint;

    jsU = cJSON_GetObjectItem(js_Root, CJSON_KEY_USER_ID);
    if (jsU == NULL)
    {
        os_trace("Get JSON jsU mqtt message fail\n");
        goto RESPONSE;
    }
    uid = jsU->valueint;

    m_fan_info_t *my_fan = m_fan_get_instance();
    auto_enable = my_fan->enable_auto_mode;
    thresh1 = my_fan->thresh_hold_temperature[0];
    thresh2 = my_fan->thresh_hold_temperature[1];
    thresh3 = my_fan->thresh_hold_temperature[2];

    result = 1;
RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%lu,\"u\":%lu,\"auto\":%u,\"thresh1\":%lu,\"thresh2\":%lu,\"thresh3\":%lu,\"result\":%u}", gs_dev_type[device_type], type, uid, auto_enable, thresh1, thresh2, thresh3, result);
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

    m_wifi_get_instance().wifi_sync_rssi_cb();

    /* respond server here */
    memset(respond, 0, sizeof(respond));

    sprintf(respond,
            "{\"%s\":%ld,\"u\":0,\"id\":\"%s\",\"ver\":%s,\"voice_ver\":%lu,"
            "\"extras\":%d,\"ssid\":\"%s\",\"pass\":\"%s\",\"streng\":%d,"
            "\"ip\":\"%s\",\"mode\":%u,\"lv\":%u,\"pcn\":%u,\"swing\":%u,"
            "\"buzz\":%u,\"t\":%lu,\"temp\":%lu,\"lamp\":%u,\"lp\":%u,\"bat\":%u,\"charging\":%u}",
            gs_dev_type[device_type], type,
            m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION,
            (unsigned long)aiio_ci13162_uart_get_voice_version(), EXTRAS,
            app_databse._wifi_info->_ssid, app_databse._wifi_info->_passwd,
            app_databse._wifi_info->_rssi, app_databse._wifi_info->_ip,
            app_databse._fan_info->mode, app_databse._fan_info->speed, app_databse._fan_info->pcn, app_databse._fan_info->swing,
            !app_databse._fan_info->disable_buzz, m_fan_get_time_remain(), val_temperature, app_databse._fan_info->light, app_databse._fan_info->power_save,
            bat_percent, bat_charging_status);

    app_send_data(respond, strlen(respond));
}
/* 107 */
static void app_get_fan_info(cJSON *jsRoot)
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
    memset(respond, 0, sizeof(respond));

    sprintf(respond,
            "{\"%s\":%ld,"
            "\"mode\":%u,\"lv\":%u,\"pcn\":%u,\"swing\":%u,"
            "\"buzz\":%u,\"t\":%lu,\"temp\":%lu,\"lamp\":%u,\"lp\":%u,"
            "\"bat\":%u,\"charging\":%u}",
            gs_dev_type[device_type], type,
            app_databse._fan_info->mode, app_databse._fan_info->speed, app_databse._fan_info->pcn, app_databse._fan_info->swing,
            !app_databse._fan_info->disable_buzz, m_fan_get_time_remain(), val_temperature, app_databse._fan_info->light, app_databse._fan_info->power_save,
            bat_percent, bat_charging_status);

    app_send_data(respond, strlen(respond));
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
    m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_WIFI);

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

    if (m_ci13162_ota_is_busy() ||
        m_ota_is_busy() ||
        app_cwsl_is_pending() ||
        app_volume_is_pending())
    {
        os_trace("OTA, CWSL, or volume operation already in progress\r\n");
        goto RESPONSE;
    }

    result = 1;

    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"result\": %d}",
            gs_dev_type[device_type], type, uid, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));

    /* implement here */
    extern void ble_deinit(void);
    ble_deinit();
    (void)m_ota_start(url);

    return;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"result\": %d}",
            gs_dev_type[device_type], type, uid, result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

/* {"TYPE":134,"url":manifest_url,"u":uid} */
static void app_request_ci13162_ota(cJSON *jsRoot)
{
    uint8_t result = 0;
    cJSON *jsType;
    cJSON *jsU;
    cJSON *jsUrl;
    ci13162_ota_manifest_urls_t resolved_urls;
    uint32_t type = AIIO_CI13162_OTA_COMMAND;
    uint32_t uid = 0U;

    if (jsRoot == NULL)
    {
        return;
    }

    jsType = cJSON_GetObjectItem(jsRoot, gs_dev_type[device_type]);
    jsU = cJSON_GetObjectItem(jsRoot, CJSON_KEY_USER_ID);
    jsUrl = cJSON_GetObjectItem(jsRoot, CJSON_KEY_URL);

    if ((jsType == NULL) || (jsType->type != cJSON_Number) ||
        (jsU == NULL) || (jsU->type != cJSON_Number) ||
        (jsUrl == NULL) || (jsUrl->type != cJSON_String) ||
        (jsUrl->valuestring == NULL))
    {
        goto RESPONSE;
    }

    type = (uint32_t)jsType->valueint;
    uid = (uint32_t)jsU->valueint;
    if ((type != AIIO_CI13162_OTA_COMMAND) ||
        m_ci13162_ota_is_busy() ||
        m_ota_is_busy() ||
        app_cwsl_is_pending() ||
        app_volume_is_pending())
    {
        goto RESPONSE;
    }

    os_trace("[CI13162][CMD] Resolve manifest\r\n");
    if (ci13162_ota_manifest_fetch(jsUrl->valuestring, &resolved_urls) != 0)
    {
        os_trace("[CI13162][CMD] Resolve manifest failed\r\n");
        goto RESPONSE;
    }
    os_trace("[CI13162][CMD] Firmware URL %s\r\n", resolved_urls.firmware_url);
    os_trace("[CI13162][CMD] Agent URL %s\r\n", resolved_urls.agent_url);
    os_trace("[CI13162][CMD] erase_nv %u\r\n",
             (unsigned int)resolved_urls.erase_nv);

    os_trace("[CI13162][CMD] Save pending request\r\n");
    if (m_ci13162_ota_boot_save(resolved_urls.firmware_url,
                                resolved_urls.agent_url,
                                (resolved_urls.erase_nv != 0U)) != 0)
    {
        os_trace("[CI13162][CMD] Save pending request failed\r\n");
        goto RESPONSE;
    }
    os_trace("[CI13162][CMD] Save pending request done\r\n");

    result = 1U;
    is_ota = STATE_OTA_PROCESS;

RESPONSE:
    memset(respond, 0, sizeof(respond));
    snprintf(respond,
             sizeof(respond),
             "{\"%s\":%lu, \"u\":%lu, \"result\": %u}",
             gs_dev_type[device_type],
             (unsigned long)type,
             (unsigned long)uid,
             (unsigned int)result);
    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
    if (result == 1U) {
        os_trace("start_ota\r\n");
        vTaskDelay(pdMS_TO_TICKS(AIIO_CI13162_OTA_MQTT_ACK_DELAY_MS));
        bl_sys_reset_por();
    }
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
    if (!app_cwsl_factory_clear())
    {
        uint32_t wait_count;

        for (wait_count = 0U; wait_count < 40U; wait_count++)
        {
            aiio_ci13162_uart_process();
            app_cwsl_poll();
            app_volume_poll();
            if (app_cwsl_factory_clear_done())
            {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(50U));
        }
    }
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

/* 35 */
void app_ble_discover(cJSON *js_root)
{
    /* check online */
    if (dev_is_doing_config())
    {
        return;
    }
    uint32_t cmd = 35;
    uint32_t uid = 0;
    cJSON *js_val = NULL;

    js_val = cJSON_GetObjectItem(js_root, CJSON_KEY_USER_ID);
    if (js_val == NULL)
    {
        os_trace("get uid fail \r\n");
        return;
    }
    uid = js_val->valueint;

    /* generate OTP with 6 digit */
    previous_otp = rand() % 1000000;
    os_trace(" otp generated: %d\r\n", previous_otp);

    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"otp\": %d}",
            gs_dev_type[device_type], cmd, uid, previous_otp);

    ble_send_data(respond, strlen(respond));
    // app_send_data(respond, strlen(respond));
}

/* 36 */
void app_ble_authen_connection(cJSON *js_root)
{
    uint32_t cmd = 36;
    uint32_t uid = 0;
    int opt = 0;
    cJSON *js_val = NULL;

    js_val = cJSON_GetObjectItem(js_root, CJSON_KEY_USER_ID);
    if (js_val == NULL)
    {
        os_trace("get uid fail \r\n");
        return;
    }
    uid = js_val->valueint;

    js_val = cJSON_GetObjectItem(js_root, CJSON_KEY_OTP);
    if (js_val == NULL)
    {
        os_trace("get opt fail \r\n");
        return;
    }
    opt = js_val->valueint;

    /* compare OTP */
    if (previous_otp != opt)
    {
        os_trace("wrong otp \r\n");
        return;
    }

    memset(respond, 0, sizeof(respond));
    sprintf(respond, "{\"%s\":%ld, \"u\":%ld, \"otp\": %d}",
            gs_dev_type[device_type], cmd, uid, previous_otp);

    ble_send_data(respond, strlen(respond));
    // app_send_data(respond, strlen(respond));
}

void app_change_device_id(cJSON *js_root)
{
    cJSON *js_val = NULL;
    char *dev_id_str = NULL;

    js_val = cJSON_GetObjectItem(js_root, "dev_id");
    if (js_val == NULL)
    {
        os_trace("get device id fail \r\n");
        return;
    }
    dev_id_str = js_val->valuestring;

    flash_write_device_id((void *)(dev_id_str), strlen(dev_id_str));

    // app_send_data(respond, strlen(respond));
}

extern void ble_loop(void);
extern void ble_init_first(void);

void app_handler_msg_task(void *arg)
{
    // m_data_t msg;
    m_data_t message_rcv_mqtt;
    m_ble_data_t msg_ble;
    // memset(&msg, 0, sizeof(msg));
    memset(&msg_ble, 0, sizeof(msg_ble));
    char *root_type = NULL;
    uint32_t raw_len = 0;
    char raw_data[MAXIMUM_SIZE_MESSAGE];
    cJSON *jsRoot = NULL, *jsType = NULL;
    ble_init_first();

    while (1)
    {
        aiio_ci13162_uart_process();
        app_cwsl_poll();
        app_volume_poll();

        if (xQueueReceive(pqueue, &message_rcv_mqtt, 0) == pdPASS)
        {
            // os_trace("raw_data received: %s\r\n", msg.data);

            /* decode data */
            memset(raw_data, 0, sizeof(raw_data));
            aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
            if (aes_cbc128_decrypt((uint8_t *)message_rcv_mqtt.data, message_rcv_mqtt.length, (uint8_t *)raw_data, (size_t *)&raw_len) != AES_SUCCESS)
            {
                os_trace("Can not decrypt message\r\n");
                continue;
            }
            if (raw_len < 450)
            {
                printf("\r\ndecoded[%lu]: %s\r\n", raw_len, raw_data);
            }

            /* find handler of data */
            jsRoot = cJSON_Parse(raw_data);
            if (!jsRoot)
            {
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
                    os_trace("Parse JSON get type fail '%s'\n", root_type);
                    cJSON_Delete(jsRoot);
                    continue;
                }
                uint32_t cmd = jsType->valueint;
                uint32_t arr_size = sizeof(array_ble_handler) / sizeof(array_ble_handler[0]);
                os_trace("arr_size_ble: %lu\r\n", arr_size);
                for (uint32_t i = 0; i < arr_size; i++)
                {
                    if (array_ble_handler[i]._cmd_number == cmd)
                    {
                        os_trace("_cmd_number: %ld\r\n", cmd);
                        array_ble_handler[i]._handler(jsRoot);
                        break;
                    }
                }
            }
            else
            {
                uint32_t cmd = jsType->valueint;

                uint32_t arr_size = sizeof(array_handler) / sizeof(array_handler[0]);
                os_trace("arr_size_mqtt: %lu\r\n", arr_size);
                for (uint32_t i = 0; i < arr_size; i++)
                {
                    if (array_handler[i]._cmd_number == cmd)
                    {
                        os_trace("_cmd_number: %ld\r\n", cmd);
                        array_handler[i]._handler(jsRoot);
                        break;
                    }
                }
            }

            cJSON_Delete(jsRoot);
        }
        else
        {
            ble_loop();
            m_mqtt_get_instance()._mqtt_loop();
            app_ir_send_response_from_remote_brand_check_loop();
        }

        aiio_ci13162_uart_poll();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

extern bool ble_init_queue(void);
void process_data_init(void)
{
#if defined(BUILD_TEST) && (BUILD_TEST == 1)
    (void)ci13162_ota_manifest_self_test();
#endif
    app_cwsl_init();
    app_volume_init();
    pdata_queue_init();
    ble_init_queue();
    m_mqtt_command_startup_register_handler(app_startup_message);

    xTaskCreate(app_handler_msg_task, "ble slave", 1024 * 4, NULL, 15, NULL);
}