#include "process_data.h"

#include <string.h>

#include "cJSON.h"
#include "base64.h"
#include "m_database.h"
#include "app_config.h"
#include "m_mqtt.h"
#include "m_ota.h"
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

static char respond[512];
extern char key[16], iv[16];

static void app_startup_message(void);
static void app_get_info(cJSON *jsRoot);
static void app_set_wifi(cJSON *jsRoot);
static void app_request_ota(cJSON *jsRoot);
static void app_set_static_ip(cJSON *jsRoot);
static void app_remove_device(cJSON *jsRoot);
static void app_backup_db(cJSON *jsRoot);
static void app_set_mqtt_info(cJSON *jsRoot);

// IR
static void ir_control(cJSON *jsRoot);
static void ir_start_learn(cJSON *jsRoot);
static void ir_stop_learn(cJSON *jsRoot);
static void ir_start_learn_auto(cJSON *jsRoot);
static void ir_stop_learn_auto(cJSON *jsRoot);

// ble
static void ble_config_step1(cJSON *jsRoot);
static void ble_config_step2(cJSON *jsRoot);

static void AC_LG2_Callback(cJSON *jsRoot);
static void AC_LG_Callback(cJSON *jsRoot);
static void AC_Casper_Callback(cJSON *jsRoot);
static void AC_Whirlpool_Callback(cJSON *jsRoot);
static void AC_Funiki_Callback(cJSON *jsRoot);
static void AC_Funiki80_Callback(cJSON *jsRoot);
static void AC_Samsung_Callback(cJSON *jsRoot);
static void AC_Toshiba_Callback(cJSON *jsRoot);
static void AC_Daikin_Callback(cJSON *jsRoot);
static void AC_Daikin2_Callback(cJSON *jsRoot);
static void AC_Daikin64_Callback(cJSON *jsRoot);
static void AC_Daikin128_Callback(cJSON *jsRoot);
static void AC_Daikin152_Callback(cJSON *jsRoot);
static void AC_Daikin160_Callback(cJSON *jsRoot);
static void AC_Daikin216_Callback(cJSON *jsRoot);
static void AC_Daikin176_Callback(cJSON *jsRoot);
static void AC_Daikin200_Callback(cJSON *jsRoot);
static void AC_Daikin312_Callback(cJSON *jsRoot);
static void AC_Gree_Callback(cJSON *jsRoot);
static void AC_Tcl_Callback(cJSON *jsRoot);
static void AC_MitshubishiAC_Callback(cJSON *jsRoot);
static void AC_Mitshubishi136_Callback(cJSON *jsRoot);
static void AC_Mitshubishi112_Callback(cJSON *jsRoot);
static void AC_MitsubishiHeavy88_Callback(cJSON *jsRoot);
static void AC_MitsubishiHeavy152_Callback(cJSON *jsRoot);
static void AC_MitsubishiHeavy160_Callback(cJSON *jsRoot);
static void AC_Midea_Callback(cJSON *jsRoot);
static void AC_Sharp_Callback(cJSON *jsRoot);
static void AC_Hitachi1_Callback(cJSON *jsRoot);
static void AC_Hitachi344_Callback(cJSON *jsRoot);
static void AC_Hitachi424_Callback(cJSON *jsRoot);
static void AC_PanasonicAC_Callback(cJSON *jsRoot);
static void AC_PanasonicAC32_Callback(cJSON *jsRoot);
static void AC_ElectraAC_Callback(cJSON *jsRoot);
static void AC_FujitsuAC_Callback(cJSON *jsRoot);
static void AC_Fujitsu2AC_Callback(cJSON *jsRoot);
static void AC_Fujitsu3AC_Callback(cJSON *jsRoot);
static void AC_VestelAc_Callback(cJSON *jsRoot);
static void AC_Sanyo_Callback(cJSON *jsRoot);
static void KelvinatorAC_Callback(cJSON *jsRoot);
static void HaierACYRW02_Callback(cJSON *jsRoot);
static void Teco_Callback(cJSON *jsRoot);
static void MirageAc_Callback(cJSON *jsRoot);
static void HitachiAc264_Callback(cJSON *jsRoot);
static void HitachiAc296_Callback(cJSON *jsRoot);
static void Nagakawa_Callback(cJSON *jsRoot);
static void Asanzo_Callback(cJSON *jsRoot);
static void AC_Casper2024_Callback(cJSON *jsRoot);
static void AC_Corona_Callback(cJSON *jsRoot);
static void AC_Carrier32_Callback(cJSON *jsRoot);

/* Hitachi */
static void AC_Hitachi_Callback(cJSON *jsRoot);

wswitch_handler_t array_handler[] = {
    {app_set_mqtt_info, 990},
    {app_get_info, 105},
    // {app_set_power_on, 121},
    {app_remove_device, 123},
    {app_set_wifi, 128},
    {app_request_ota, 129},
    // {app_set_time_release, 130},
    // {app_get_time_release, 131},
    {app_set_static_ip, 132},
    // {app_set_lock, 133},
    {app_backup_db, 151},

    {ir_control, 1},
    {ir_start_learn, 2},
    {ir_stop_learn, 4},
    {ir_start_learn_auto, 15},
    {ir_stop_learn_auto, 16},
};

wswitch_handler_t array_ble_handler[] = {
    {ble_config_step1, 1},
    {ble_config_step2, 3},
};

wswitch_handler_t array_ir_control_handler[] = {
    {AC_Casper_Callback, 103},             // 102
    {AC_Whirlpool_Callback, WHIRLPOOL_AC}, // 34
    {AC_Corona_Callback, CORONA_AC},       // 34
    {AC_LG2_Callback, 23},                 // 23
    {AC_LG_Callback, 114},                 // 23
    {AC_Funiki_Callback, 104},             // 12
    {AC_Funiki80_Callback, FUNIKIAC83},             // 12
    {AC_Samsung_Callback, 27},             // 27 - 14 bytes
    {AC_Toshiba_Callback, 108},            // 31 - 9 bytes
    {AC_Daikin_Callback, DAIKIN},          // 14 - 35 bytes
    {AC_Daikin2_Callback, 1},              // 14 - 35 bytes
    {AC_Daikin64_Callback, DAIKIN64},      // 35 bytes
    {AC_Daikin152_Callback, 6},            // 6 - 35 bytes
    {AC_Daikin128_Callback, 5},            // 5 - 35 bytes
    {AC_Daikin160_Callback, 4},            // 4 - 35 bytes
    {AC_Daikin216_Callback, 2},            // 2 - 35 bytes
    {AC_Daikin176_Callback, DAIKIN176},    // 2 - 35 bytes
    {AC_Daikin200_Callback, DAIKIN200},    // 2 - 35 bytes
    {AC_Gree_Callback, 19},                // 19 - 8 bytes - Electrolux
    {AC_Tcl_Callback, 29},                 // 29 - 14 bytes
    {AC_Tcl_Callback, AKITO},                 // 29 - 14 bytes
    {AC_Mitshubishi112_Callback, 91},      // 91 - 14 bytes
    {AC_Midea_Callback, 1934},             // 1934 - 14 bytes
    {AC_Sharp_Callback, 28},               // 28 - 13 bytes
    {AC_Hitachi1_Callback, 92},
    {AC_Hitachi344_Callback, 94},
    {AC_Hitachi424_Callback, 93},
    {AC_Hitachi_Callback, 21},
    {AC_PanasonicAC_Callback, 106},
    {AC_PanasonicAC_Callback, 129},
    {AC_PanasonicAC32_Callback, 107},
    {AC_ElectraAC_Callback, 16}, // casper
    {AC_FujitsuAC_Callback, FUJITSU_AC},
    {AC_Fujitsu2AC_Callback, FUJITSU2_AC},
    {AC_Fujitsu3AC_Callback, FUJITSU_NEW_AC},
    {AC_VestelAc_Callback, 33},
    {AC_Sanyo_Callback, 109},
    {AC_MitsubishiHeavy88_Callback, 110},
    {AC_MitsubishiHeavy152_Callback, 111},
    {AC_MitsubishiHeavy160_Callback, MITSUBISHI_HEAVY_160},
    {AC_MitshubishiAC_Callback, 112},
    {AC_Mitshubishi136_Callback, 113},
    {KelvinatorAC_Callback, 22},
    {HaierACYRW02_Callback, 115},
    {Teco_Callback, 116},
    {MirageAc_Callback, 117},
    {HitachiAc264_Callback, HITACHI_AC264},
    {HitachiAc296_Callback, HITACHI_AC296},
    {Nagakawa_Callback, NAGAKAWA},
    {Asanzo_Callback, GOODWEATHER},
    {AC_Daikin312_Callback, DAIKIN312},
    {AC_Casper2024_Callback, CASPER2024_AC},
    {AC_Carrier32_Callback, CARRIER_AC},
};

irwifi_data_brand_handler_t array_ir_data_control_handler[] = {
    {sendSaijoDenkiAC, SAIJODENKI}, // 127
    {sendFunikiAc104AC, FUNIKIAC104},
};

QueueHandle_t pqueue;
// extern m_data_t message_rcv_mqtt;
QueueHandle_t pqueue_ble;

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

    if (pqueue_ble == NULL)
    {
        pqueue_ble = xQueueCreate(8, sizeof(m_ble_data_t));
        if (pqueue_ble == NULL)
        {
            os_trace("%s ble fail\r\n", __func__);
            exit(-1);
        }
    }

    return true;
}

void push_data_to_pqueue_ble(m_ble_data_t *message)
{
    if (xQueueSend(pqueue_ble, message, pdMS_TO_TICKS(0)) != pdPASS)
    {
        os_trace("Push to queue ble data rx fail\r\n");
    }
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
    uint32_t i;
    static uint32_t num_send = 0;
    m_wifi_get_instance().wifi_sync_rssi_cb();
    /* respond server here */
    memset(respond, 0, sizeof(respond));

    sprintf(respond,
            "{\"%s\":100,\"reason\":%lu,\"id\":\"%s\",\"ver\":%s,"
            "\"ssid\":\"%s\",\"pass\":\"%s\",\"streng\":%d,"
            "\"ip\":\"%s\",\"vermqtt\":%lu}",
            gs_dev_type[device_type], num_send * 100U + last_reason_reset * 10U + (uint32_t)bl_sys_rstinfo_get_hun(), // hang chuc: flash_reason reset, hang don vi: BL_RST_REASON_E
            m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION,
            app_databse._wifi_info->_ssid, app_databse._wifi_info->_passwd,
            app_databse._wifi_info->_rssi, app_databse._wifi_info->_ip, app_databse._mqtt_info->vermqtt);

    num_send++;

    m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
}

extern bool save_mqtt_info(char *json);
static void app_set_mqtt_info(cJSON *js_Root)
{
    char temp1[512];
    char temp2[512];
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

static void ir_send_data(cJSON *jsRoot)
{
    cJSON *jsData;
    uint16_t result_temp[750];
    uint32_t len_data_b64 = 0;

    jsData = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_DATA);
    if (jsData == NULL)
    {
        os_trace("Get JSON jsData mqtt message fail\n");
        return;
    }
    char *data_dec = jsData->valuestring;
    len_data_b64 = strlen(data_dec);
    os_trace("%s len_base64_dec_data: %lu\r\n", __func__, len_data_b64);
    memset(result_temp, 0, sizeof(result_temp));
    b64_decode((unsigned char *)data_dec, len_data_b64, (unsigned char *)result_temp);
    sendRaw(result_temp, (uint16_t)((len_data_b64 * 3) / 8), 38);
}

static void ir_control_type_3(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsTypeSend, *jsU, *jsAct_ID, *jsPower, *jsTemp, *jsMode, *jsFan;
    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    uint32_t brand = 0, typesend = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    uint8_t *my_data = NULL;
    int swingv = -1, swingh = -1;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto CASE_FAILED;
    }
    brand = jsBrand->valueint;

    if (jsRoot == NULL)
    {
        os_trace("jsRoot NULL\n");
        goto CASE_FAILED;
    }

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto CASE_FAILED;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto CASE_FAILED;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto CASE_FAILED;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto CASE_FAILED;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto CASE_FAILED;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto CASE_FAILED;
    }
    fan = jsFan->valueint;

    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        swingh = jsSwingH->valueint;
    }

    // RESPONSE:
    //     /* respond server here */
    //     memset(respond, 0, sizeof(respond));
    //     sprintf(
    //         respond,
    //         "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
    //         gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    //     app_send_data(respond, strlen(respond));

    cJSON *jsData = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_DATA);
    if (jsData == NULL)
    {
        os_trace("jsData NULL\n");
        goto CASE_FAILED;
    }
    char *p_data = jsData->valuestring;

    int len_str = (strlen(p_data) / 3) + 1;
    my_data = (uint8_t *)malloc(len_str * sizeof(uint8_t));

    if (!my_data)
    {
        os_trace("Memory allocation failed\n");
        goto CASE_FAILED;
    }

    for (uint8_t i = 0; i < len_str; i++)
    {
        unsigned int c;
        sscanf(&p_data[3 * i], "%02X", &c);
        my_data[i] = (uint8_t)c;
    }

    uint32_t arr_size = sizeof(array_ir_data_control_handler) / sizeof(array_ir_data_control_handler[0]);
    os_trace("arr_size_ir_data_control: %lu\r\n", arr_size);
    for (uint32_t i = 0; i < arr_size; i++)
    {
        if (array_ir_data_control_handler[i]._cmd_number == brand)
        {
            os_trace("ir brand control data: %ld\r\n", brand);
            hw_led_stt_on();
            result = array_ir_data_control_handler[i]._handler(my_data, len_str);
            hw_led_stt_off();
            memset(respond, 0, sizeof(respond));
            sprintf(
                respond,
                "{\"%s\":1,\"type\":3,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"data\":\"%s\",\"act\":%lu,\"result\":%u}",
                gs_dev_type[device_type], brand, power, temp, mode, fan, swingv, swingh, p_data, act_id, result);
            app_send_data(respond, strlen(respond));
            free(my_data);
            my_data = NULL;
            return;
        }
    }
CASE_FAILED:
    if (!my_data)
    {
        free(my_data);
        my_data = NULL;
    }
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":1,\"type\":3,\"brand\":%lu,\"result\":0,\"err\":\"params error\"}",
        gs_dev_type[device_type], brand);
    app_send_data(respond, strlen(respond));
}

/*  {"irwifi":1,"type":1,"brand":102,"power":1,"temp":25,"mode":4,"fan":6,"act":0}  */
static void ir_control(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsTypeSend;
    uint32_t brand = 0, typesend = 0;
    uint8_t result = 0;

    if (jsRoot == NULL)
    {
        os_trace("jsRoot NULL\n");
        goto RESPONSE;
    }

    jsTypeSend = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TYPE_SEND);
    if (jsTypeSend == NULL)
    {
        os_trace("Get JSON TypeSend mqtt message fail\n");
        goto RESPONSE;
    }
    typesend = jsTypeSend->valueint;

    if (typesend == 1)
    {
        jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
        if (jsBrand == NULL)
        {
            os_trace("jsBrand NULL\n");
            goto RESPONSE;
        }
        brand = jsBrand->valueint;

        uint32_t arr_size = sizeof(array_ir_control_handler) / sizeof(array_ir_control_handler[0]);
        os_trace("arr_size_ir_control: %lu\r\n", arr_size);
        for (uint32_t i = 0; i < arr_size; i++)
        {
            if (array_ir_control_handler[i]._cmd_number == brand)
            {
                os_trace("ir brand: %ld\r\n", brand);
                hw_led_stt_on();
                array_ir_control_handler[i]._handler(jsRoot);
                hw_led_stt_off();
                break;
            }
            else if (arr_size - 1 == i)
            {
                memset(respond, 0, sizeof(respond));
                sprintf(
                    respond,
                    "{\"%s\":1,\"type\":1,\"brand\":%lu,\"result\":0,\"err\":\"not support brand\"}",
                    gs_dev_type[device_type], brand);
                app_send_data(respond, strlen(respond));
            }
        }
    }
    else if (typesend == 2)
    {
        // send by time
        hw_led_stt_on();
        ir_send_data(jsRoot);
        hw_led_stt_off();
        memset(respond, 0, sizeof(respond));
        sprintf(
            respond,
            "{\"%s\":1,\"type\":2,\"result\": 1}",
            gs_dev_type[device_type]);
        app_send_data(respond, strlen(respond));
    }
    else if (typesend == 3)
    {
        ir_control_type_3(jsRoot);
    }

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    // sprintf(
    //     respond,
    //     "{\"%s\":%ld, \"action\":%ld, \"u\":%ld, \"act\": %ld, \"result\": %d}",
    //     gs_dev_type[device_type], type, action, uid, act_id, result);
    // app_send_data(respond, strlen(respond));
}

static void ir_start_learn(cJSON *jsRoot)
{
    uint8_t result = 0;
    uint32_t type = 0;

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
    IR_start_learn();
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%ld,\"result\":%d}",
        gs_dev_type[device_type], type, result);
    app_send_data(respond, strlen(respond));
}

static void ir_stop_learn(cJSON *jsRoot)
{
    uint8_t result = 0;
    uint32_t type = 0;

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
    IR_stop_learn();
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%ld,\"result\":%d}",
        gs_dev_type[device_type], type, result);
    app_send_data(respond, strlen(respond));
}

static void ir_start_learn_auto(cJSON *jsRoot)
{
    uint8_t result = 0;
    uint32_t type = 0;

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
    IR_start_learn_auto();
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%ld,\"result\":%d}",
        gs_dev_type[device_type], type, result);
    app_send_data(respond, strlen(respond));
}

static void ir_stop_learn_auto(cJSON *jsRoot)
{
    uint8_t result = 0;
    uint32_t type = 0;

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
    IR_stop_learn_auto();
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%ld,\"result\":%d}",
        gs_dev_type[device_type], type, result);
    app_send_data(respond, strlen(respond));
}

/*  {"irwifi":1,"type":1,"brand":102,"power":1,"temp":25,"mode":4,"fan":6,"act":0}  */
static void AC_Casper_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    os_trace("power: %u\r\n", state_ir.power);

    Casper_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Whirlpool_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    os_trace("power: %u\r\n", state_ir.power);

    Whirlpool_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Corona_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    os_trace("power: %u\r\n", state_ir.power);

    Corona_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_LG_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    LG_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}
static void AC_LG2_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    LG2_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Funiki_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Funiki_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Funiki80_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    FunikiAc83_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Samsung_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Samsung_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Toshiba_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Toshiba_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Daikin_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin2_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Daikin2_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin312_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Daikin312_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin152_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Daikin152_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin64_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Daikin64_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin176_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Daikin176_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin200_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Daikin200_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin128_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Daikin128_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin160_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Daikin160_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Daikin216_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Daikin216_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Gree_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Gree_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Tcl_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;
    state_ir.protocol = brand;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Tcl112_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Mitshubishi112_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Mitsubishi112_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Midea_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Midea_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Sharp_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    SharpAc_send(state_ir);

    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Hitachi_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Hitachi_AC_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Hitachi1_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Hitachi_AC1_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Hitachi344_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Hitachi_AC344_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Hitachi424_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Hitachi_AC424_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void HitachiAc264_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    HitachiAC264_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void HitachiAc296_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    HitachiAc296_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Casper2024_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Casper2024Ac_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Carrier32_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    CarrierAc32_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void Nagakawa_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Nagakawa_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void Asanzo_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    Asanzo_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_PanasonicAC_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;
    state_ir.protocol = brand;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    PanasonicAC_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_PanasonicAC32_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    PanasonicAc32_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_ElectraAC_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    ElectraAc_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_FujitsuAC_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;
    state_ir.model = ARRAH2E;

    FujitsuAC_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Fujitsu2AC_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;
    state_ir.model = ARDB1;

    FujitsuAC_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Fujitsu3AC_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;
    state_ir.model = ARRFH2J;

    FujitsuAC_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_VestelAc_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    VestelAc_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Sanyo_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    SanyoAc_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_MitsubishiHeavy88_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    MitsubishiHeavy88Ac_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_MitsubishiHeavy152_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    MitsubishiHeavy152Ac_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_MitshubishiAC_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    MitsubishiAC_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_Mitshubishi136_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Mitsubishi136_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void KelvinatorAC_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    KelvinatorAC_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void HaierACYRW02_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    HaierACYRW02_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void AC_MitsubishiHeavy160_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    MitsubishiHeavy160_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void Teco_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    IRTecoAc_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
    app_send_data(respond, strlen(respond));
}

static void MirageAc_Callback(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsPower, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, power = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
    uint8_t result = 0;
    state_ir_t state_ir = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

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

    jsAct_ID = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_ACT);
    if (jsAct_ID == NULL)
    {
        os_trace("jsAct_ID NULL\n");
        goto RESPONSE;
    }
    act_id = jsAct_ID->valueint;

    jsBrand = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_BRAND);
    if (jsBrand == NULL)
    {
        os_trace("jsBrand NULL\n");
        goto RESPONSE;
    }
    brand = jsBrand->valueint;

    jsPower = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_POWER);
    if (jsPower == NULL)
    {
        os_trace("jsPower NULL\n");
        goto RESPONSE;
    }
    power = jsPower->valueint;

    jsTemp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TEMP);
    if (jsTemp == NULL)
    {
        os_trace("jsTemp NULL\n");
        goto RESPONSE;
    }
    temp = jsTemp->valueint;

    jsMode = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_MODE);
    if (jsMode == NULL)
    {
        os_trace("jsMode NULL\n");
        goto RESPONSE;
    }
    mode = jsMode->valueint;

    jsFan = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_FAN);
    if (jsFan == NULL)
    {
        os_trace("jsFan NULL\n");
        goto RESPONSE;
    }
    fan = jsFan->valueint;

    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    cJSON *jsSwingV = NULL, *jsSwingH = NULL;
    jsSwingV = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGV);
    if (jsSwingV != NULL)
    {
        state_ir.swingv = jsSwingV->valueint;
    }

    jsSwingH = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_SWINGH);
    if (jsSwingH != NULL)
    {
        state_ir.swingh = jsSwingH->valueint;
    }

    IRMirageAc_send(state_ir);
    result = 1;

RESPONSE:
    /* respond server here */
    memset(respond, 0, sizeof(respond));
    sprintf(
        respond,
        "{\"%s\":%lu,\"type\":1,\"brand\":%lu,\"power\":%lu,\"temp\":%lu,\"mode\":%lu,\"fan\":%lu,\"swingv\":%d,\"swingh\":%d,\"act\":%lu,\"result\":%d}",
        gs_dev_type[device_type], type, brand, power, temp, mode, fan, state_ir.swingv, state_ir.swingh, act_id, result);
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
    uint32_t i;

    m_wifi_get_instance().wifi_sync_rssi_cb();

    /* respond server here */
    memset(respond, 0, sizeof(respond));

    sprintf(respond,
            "{\"%s\":%ld,\"u\":0,\"id\":\"%s\",\"ver\":%s,"
            "\"extras\":%d,\"ssid\":\"%s\",\"pass\":\"%s\",\"streng\":%d,"
            "\"ip\":\"%s\"}",
            gs_dev_type[device_type], type,
            m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION, EXTRAS,
            app_databse._wifi_info->_ssid, app_databse._wifi_info->_passwd,
            app_databse._wifi_info->_rssi, app_databse._wifi_info->_ip);

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
                    os_trace("Parse JSON get type fail\n");
                    cJSON_Delete(jsRoot);
                    continue;
                }
            }
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

            cJSON_Delete(jsRoot);
        }
        else if (xQueueReceive(pqueue_ble, &msg_ble, 0) == pdPASS)
        {
            os_trace("raw_data ble received[%lu]: %s\r\n", msg_ble.length, msg_ble.data);

            /* decode data */
            memset(raw_data, 0, sizeof(raw_data));
            aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
            if (aes_cbc128_decrypt((uint8_t *)msg_ble.data, msg_ble.length, (uint8_t *)raw_data, (size_t *)&raw_len) != AES_SUCCESS)
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
                    os_trace("Parse JSON get type fail\n");
                    cJSON_Delete(jsRoot);
                    continue;
                }
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

            cJSON_Delete(jsRoot);
        }
        else
        {
            ble_loop();
            m_mqtt_get_instance()._mqtt_loop();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

extern bool ble_init_queue(void);
void process_data_init(void)
{
    pdata_queue_init();
    ble_init_queue();
    m_mqtt_command_startup_register_handler(app_startup_message);

    xTaskCreate(app_handler_msg_task, "ble slave", 1024 * 4, NULL, 15, NULL);
}