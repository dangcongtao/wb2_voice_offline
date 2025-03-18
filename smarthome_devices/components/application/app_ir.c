#include "app_ir.h"
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
#include "event_driven.h"
#include "IRutils.h"

extern char respond[512];

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
static void AC_SanyoAC152_Callback(cJSON *jsRoot);
static void KelvinatorAC_Callback(cJSON *jsRoot);
static void HaierACYRW02_Callback(cJSON *jsRoot);
static void HaierAC176_Callback(cJSON *jsRoot);
static void Teco_Callback(cJSON *jsRoot);
static void MirageAc_Callback(cJSON *jsRoot);
static void HitachiAc264_Callback(cJSON *jsRoot);
static void HitachiAc296_Callback(cJSON *jsRoot);
static void Nagakawa_Callback(cJSON *jsRoot);
static void Asanzo_Callback(cJSON *jsRoot);
static void AC_Casper2024_Callback(cJSON *jsRoot);
static void AC_Corona_Callback(cJSON *jsRoot);
static void AC_Carrier32_Callback(cJSON *jsRoot);
static void AC_Hitachi_Callback(cJSON *jsRoot);

wswitch_handler_t array_ir_control_handler[] = {
    {AC_Casper_Callback, 103},                   // 102
    {AC_Whirlpool_Callback, WHIRLPOOL_AC},       // 34
    {AC_Corona_Callback, CORONA_AC},             // 34
    {AC_LG2_Callback, 23},                       // 23
    {AC_LG_Callback, 114},                       // 23
    {AC_Funiki_Callback, 104},                   // 12
    {AC_Funiki80_Callback, FUNIKIAC83},          // 12
    {AC_Samsung_Callback, 27},                   // 27 - 14 bytes
    {AC_Toshiba_Callback, 108},                  // 31 - 9 bytes
    {AC_Daikin_Callback, DAIKIN},                // 14 - 35 bytes
    {AC_Daikin2_Callback, 1},                    // 14 - 35 bytes
    {AC_Daikin64_Callback, DAIKIN64},            // 35 bytes
    {AC_Daikin152_Callback, 6},                  // 6 - 35 bytes
    {AC_Daikin128_Callback, 5},                  // 5 - 35 bytes
    {AC_Daikin160_Callback, 4},                  // 4 - 35 bytes
    {AC_Daikin216_Callback, 2},                  // 2 - 35 bytes
    {AC_Daikin176_Callback, DAIKIN176},          // 2 - 35 bytes
    {AC_Daikin200_Callback, DAIKIN200},          // 2 - 35 bytes
    {AC_Gree_Callback, 19},                      // 19 - 8 bytes - Electrolux
    {AC_Tcl_Callback, 29},                       // 29 - 14 bytes
    {AC_Tcl_Callback, AKITO},                    // 29 - 14 bytes
    {AC_Mitshubishi112_Callback, MITSUBISHI112}, // 91 - 14 bytes
    {AC_Midea_Callback, 1934},                   // 1934 - 14 bytes
    {AC_Sharp_Callback, 28},                     // 28 - 13 bytes
    {AC_Hitachi1_Callback, 92},
    {AC_Hitachi344_Callback, 94},
    {AC_Hitachi424_Callback, 93},
    {AC_Hitachi_Callback, HITACHI_AC},
    {AC_PanasonicAC_Callback, 106},
    {AC_PanasonicAC_Callback, 129},
    {AC_PanasonicAC32_Callback, 107},
    {AC_ElectraAC_Callback, 16}, // casper
    {AC_FujitsuAC_Callback, FUJITSU_AC},
    {AC_Fujitsu2AC_Callback, FUJITSU2_AC},
    {AC_Fujitsu3AC_Callback, FUJITSU_NEW_AC},
    {AC_VestelAc_Callback, 33},
    {AC_Sanyo_Callback, 109},
    {AC_SanyoAC152_Callback, SANYO_AC152},
    {AC_MitsubishiHeavy88_Callback, 110},
    {AC_MitsubishiHeavy152_Callback, 111},
    {AC_MitsubishiHeavy160_Callback, MITSUBISHI_HEAVY_160},
    {AC_MitshubishiAC_Callback, 112},
    {AC_Mitshubishi136_Callback, 113},
    {KelvinatorAC_Callback, 22},
    {HaierACYRW02_Callback, 115},
    {HaierAC176_Callback, HAIER_AC176},
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
            hw_led_stt_off();
            result = array_ir_data_control_handler[i]._handler(my_data, len_str);
            hw_led_stt_on();
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
void ir_control(cJSON *jsRoot)
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
                hw_led_stt_off();
                array_ir_control_handler[i]._handler(jsRoot);
                hw_led_stt_on();
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
        hw_led_stt_off();
        ir_send_data(jsRoot);
        hw_led_stt_on();
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

void ir_start_learn(cJSON *jsRoot)
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

void ir_stop_learn(cJSON *jsRoot)
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

void ir_start_learn_auto(cJSON *jsRoot)
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

void ir_stop_learn_auto(cJSON *jsRoot)
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

    Daikin312_send(state_ir);

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

    HitachiAC_send(state_ir);
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

static void AC_SanyoAC152_Callback(cJSON *jsRoot)
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

    SanyoAc152_send(state_ir);
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

static void HaierAC176_Callback(cJSON *jsRoot)
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

    HaierAC176_send(state_ir);
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

response_remote_data_t g_remote_data;
extern decode_results g_capture;

static void learnIR(decode_results *results)
{
    // decode_results cb;
    // decode_results *results = &cb;
    // results->rawlen = 739;
    // results->rawbuf = results1->rawbuf;
    // memset(results->rawbuf + results1->rawlen, 100, sizeof(uint16_t)*(results->rawlen-results1->rawlen));
    uint16_t len_temp = getCorrectedRawLength(results);
    os_trace("%s step 1 %u\r\n", __func__, len_temp);
    uint16_t result_temp[len_temp];

    resultToRawArray(results, result_temp);
    os_trace("%s step 2\r\n", __func__);

    int size_rawData = getCorrectedRawLength(results) * sizeof(uint16_t);
    os_trace("%s step 3\r\n", __func__);

    size_rawData = min(size_rawData, 1460); // max data base64

    uint8_t buffDataIR[2001];
    memset(buffDataIR, 0, sizeof(buffDataIR));
    b64_encode((unsigned char *)result_temp, size_rawData, (unsigned char *)buffDataIR);
    os_trace("%s step 4: len_buffDataIR: %u\r\n\r\n\r\n", __func__, strlen((const char *)buffDataIR));
    // os_trace("%s step 5: len_buffDataIR: %u\r\n\r\n\r\n", __func__, strlen((const char*)buffDataIR));
    os_trace("%s step 5: size_rawData: %d, len_temp: %u\r\n", __func__, size_rawData, len_temp);

    char str[MAXIMUM_SIZE_MESSAGE];
    memset(str, 0, MAXIMUM_SIZE_MESSAGE);

    sprintf(str, "{\"%s\":3,\"data\":\"%s\"}", gs_dev_type[device_type], buffDataIR);
    // os_trace("datasend[%u]:", strlen(str));
    // os_trace("'%s'\r\n", str);
    if (dev_is_online())
        m_mqtt_get_instance()._mqtt_publish(str, strlen(str));
}

extern void m_fan_process_remote_event(uint32_t button_data);
void app_ir_send_response_from_remote_brand_check_loop(void)
{
    if (g_remote_data.request_send == REQUEST_SEND_REMOTE_BRAND)
    {
        g_remote_data.request_send = 0;

        memset(respond, 0, sizeof(respond));
        int seek = 0;
        if (g_remote_data.data.ir_ac.degrees < 10 || g_remote_data.data.ir_ac.degrees > 40)
        {
            os_trace("%s invalid temperature %f\r\n", __func__, g_remote_data.data.ir_ac.degrees);
            g_remote_data.data.ir_ac.degrees = 25;
        }
        seek += sprintf(respond, "{\"%s\":10,\"type\":1,\"brand\":%d,\"model\":%d,"
                                 "\"power\":%u,\"mode\":%d,\"temp\":%.1f,\"fan\":%d,\"swingv\":%d,"
                                 "\"swingh\":%d}",
                        gs_dev_type[device_type], g_remote_data.decode_type, g_remote_data.model,
                        g_remote_data.data.ir_ac.power, g_remote_data.data.ir_ac.mode, g_remote_data.data.ir_ac.degrees,
                        g_remote_data.data.ir_ac.fanspeed, g_remote_data.data.ir_ac.swingv,
                        g_remote_data.data.ir_ac.swingh);

        os_trace("datarcv[%u]:", strlen(respond));
        if (seek < 300)
            os_trace("'%s'\r\n", respond);
        os_trace("\r\n");
        m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
    }
    else if (g_remote_data.request_send == REQUEST_SEND_IR_FAN)
    {
        g_remote_data.request_send = 0;
        m_fan_process_remote_event(g_remote_data.data.remote.value);
    }
    else if (g_remote_data.request_send_learn == REQUEST_SEND_LEARN_IR)
    {
        g_remote_data.request_send_learn = 0;
        learnIR(&g_capture);
    }
    else if (g_remote_data.request_send_learn == REQUEST_SEND_LEARN_AUTO)
    {
        g_remote_data.request_send_learn = 0;
        memset(respond, 0, 128);

        sprintf(respond, "{\"%s\":17,\"type\":1,\"brand\":\"%d\",\"model\":%d}",
                gs_dev_type[device_type], g_capture.decode_type, g_remote_data.model_learn);
        os_trace("learn_auto_datasend[%u]:", strlen(respond));
        os_trace("'%s'\r\n", respond);
        m_mqtt_get_instance()._mqtt_publish(respond, strlen(respond));
    }
}
