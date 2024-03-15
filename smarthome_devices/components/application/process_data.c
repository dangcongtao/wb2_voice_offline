#include "process_data.h"

#include <string.h>

#include "cJSON.h"
#include "base64.h"
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
#include "bl_sys.h"
#include "IRremoteWB2.h"
#include "IRsend.h"
#include "IRrecv.h"
#include "hardware.h"

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

// IR
static void ir_control(cJSON *jsRoot);
static void ir_start_learn(cJSON *jsRoot);
static void ir_stop_learn(cJSON *jsRoot);
static void ir_start_learn_auto(cJSON *jsRoot);
static void ir_stop_learn_auto(cJSON *jsRoot);

#if PROJECT_SHOCK
static void app_get_leakage(cJSON *jsRoot);
static void app_fake_leakage(cJSON *jsRoot);
#endif

static void ble_config_step1(cJSON *jsRoot);
static void ble_config_step2(cJSON *jsRoot);

static void AC_LG_Callback(cJSON *jsRoot);
static void AC_Casper_Callback(cJSON *jsRoot);
static void AC_Funiki_Callback(cJSON *jsRoot);
static void AC_Aqua_Callback(cJSON *jsRoot);
static void AC_Samsung_Callback(cJSON *jsRoot);
static void AC_Toshiba_Callback(cJSON *jsRoot);
static void AC_Daikin_Callback(cJSON *jsRoot);
static void AC_Daikin2_Callback(cJSON *jsRoot);
static void AC_Daikin128_Callback(cJSON *jsRoot);
static void AC_Daikin152_Callback(cJSON *jsRoot);
static void AC_Daikin160_Callback(cJSON *jsRoot);
static void AC_Daikin216_Callback(cJSON *jsRoot);
static void AC_Gree_Callback(cJSON *jsRoot);
static void AC_Tcl_Callback(cJSON *jsRoot);
static void AC_MitshubishiAC_Callback(cJSON *jsRoot);
static void AC_Mitshubishi136_Callback(cJSON *jsRoot);
static void AC_Mitshubishi112_Callback(cJSON *jsRoot);
static void AC_MitsubishiHeavy88_Callback(cJSON *jsRoot);
static void AC_MitsubishiHeavy152_Callback(cJSON *jsRoot);
static void AC_Midea_Callback(cJSON *jsRoot);
static void AC_Sharp_Callback(cJSON *jsRoot);
static void AC_Hitachi1_Callback(cJSON *jsRoot);
static void AC_Hitachi344_Callback(cJSON *jsRoot);
static void AC_Hitachi424_Callback(cJSON *jsRoot);
static void AC_PanasonicAC_Callback(cJSON *jsRoot);
static void AC_PanasonicAC32_Callback(cJSON *jsRoot);
static void AC_ElectraAC_Callback(cJSON *jsRoot);
static void AC_FujitsuAC_Callback(cJSON *jsRoot);
static void AC_VestelAc_Callback(cJSON *jsRoot);
static void AC_Sanyo_Callback(cJSON *jsRoot);
static void KelvinatorAC_Callback(cJSON *jsRoot);

/* Hitachi */
static void AC_Hitachi_Callback(cJSON *jsRoot);

wswitch_handler_t array_handler[] = {
    // {app_control_pin, 0},        
    // {app_start_cyclic_timer, 64},
    // {app_stop_cyclic_timer, 65}, 

#if PROJECT_SHOCK
    {app_get_leakage, 71}, 
    {app_fake_leakage, 72},
#endif
    {app_get_info, 105},
    // {app_set_power_on, 121},     
    {app_remove_device, 123},
    {app_set_wifi, 128},
    {app_request_ota, 129},
    // {app_set_time_release, 130}, 
    // {app_get_time_release, 131},
    {app_set_static_ip, 132},    
    {app_set_lock, 133},
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
    {AC_Casper_Callback, 103}, // 102
    {AC_LG_Callback, 23}, // 23
    {AC_Funiki_Callback, 104}, // 12
    {AC_Aqua_Callback, 101} , //101
    {AC_Samsung_Callback, 27} , //27 - 14 bytes
    {AC_Toshiba_Callback, 108} , // 31 - 9 bytes
    {AC_Daikin_Callback, 14} , // 14 - 35 bytes
    {AC_Daikin2_Callback, 1} , // 14 - 35 bytes
    {AC_Daikin152_Callback, 6} , // 6 - 35 bytes
    {AC_Daikin128_Callback, 5} , // 5 - 35 bytes
    {AC_Daikin160_Callback, 4} , // 4 - 35 bytes
    {AC_Daikin216_Callback, 2} , // 2 - 35 bytes
    {AC_Gree_Callback, 19} , // 19 - 8 bytes - Electrolux
    {AC_Tcl_Callback, 29} , // 29 - 14 bytes
    {AC_Mitshubishi112_Callback, 91} , // 91 - 14 bytes
    {AC_Midea_Callback, 1934} , // 1934 - 14 bytes
    {AC_Sharp_Callback, 28} , // 28 - 13 bytes
    {AC_Hitachi1_Callback, 92} ,
    {AC_Hitachi344_Callback, 94} ,
    {AC_Hitachi424_Callback, 93} ,
    {AC_Hitachi_Callback, 21},
    {AC_PanasonicAC_Callback, 106},
    {AC_PanasonicAC32_Callback, 107},
    {AC_ElectraAC_Callback, 16}, // casper
    {AC_FujitsuAC_Callback, 17},
    {AC_VestelAc_Callback, 33},
    {AC_Sanyo_Callback, 109},
    {AC_MitsubishiHeavy88_Callback, 110},
    {AC_MitsubishiHeavy152_Callback, 111},
    {AC_MitshubishiAC_Callback, 112},
    {AC_Mitshubishi136_Callback, 113},
    {KelvinatorAC_Callback, 22},

};

// QueueHandle_t pqueue;
extern m_data_t message_rcv_mqtt;
uint8_t has_mess = 0;
QueueHandle_t pqueue_ble;

bool pdata_queue_init(void)
{
    // if (pqueue == NULL) {
    //     pqueue = xQueueCreate(MAXIMUM_QUEUE_ITEM, sizeof(m_data_t));
    //     if(pqueue == NULL){
    //         os_trace("%s fail\r\n", __func__);
    //         exit(-1);
    //     }
    // }

    if (pqueue_ble == NULL) {
        pqueue_ble = xQueueCreate(8, sizeof(m_ble_data_t));
        if(pqueue_ble == NULL){
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

void push_data_to_pqueue(void)
{
    has_mess = 1;
    // if (xQueueSend(pqueue, message, pdMS_TO_TICKS(0)) != pdPASS)
    // {
    //     os_trace("Push to queue data rx fail\r\n");
    // }
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
    extern uint8_t last_reason_reset;
    static uint32_t num_send = 0;

    /* respond server here */
    memset(respond, 0, sizeof(respond));

    buffer_seek +=
        sprintf(respond,
                "{\"%s\":100,\"reason\":%lu,\"id\":\"%s\",\"ver\":%s,"
                "\"ssid\":\"%s\",\"pass\":\"%s\",\"streng\":%d, "
                "\"ip\":\"%s\",\"po\":[",
                gs_dev_type[device_type], num_send * 100U + last_reason_reset * 10U + (uint32_t)bl_sys_rstinfo_get_hun(), // hang chuc: flash_reason reset, hang don vi: BL_RST_REASON_E
                m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION,
                app_databse._wifi_info->_ssid, app_databse._wifi_info->_passwd,
                app_databse._wifi_info->_rssi, app_databse._wifi_info->_ip);

    num_send++;

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
    char* data_dec = jsData->valuestring;
    len_data_b64 = strlen(data_dec);
    os_trace("%s len_base64_dec_data: %lu\r\n", __func__, len_data_b64);
    memset(result_temp, 0, sizeof(result_temp));
    b64_decode((unsigned char *)data_dec, len_data_b64, (unsigned char *)result_temp);
    sendRaw(result_temp, (uint16_t)((len_data_b64 * 3) / 8), 38);
}

/*  {"irwifi":1,"type":1,"brand":102,"power":1,"temp":25,"mode":4,"fan":6,"act":0}  */
static void ir_control(cJSON *jsRoot)
{
    cJSON *jsBrand, *jsTypeSend, *jsTemp, *jsMode, *jsFan, *jsU, *jsAct_ID;
    uint32_t type = 0, brand = 0, typesend = 0, temp = 0, mode = 0, fan = 0, act_id = 0;
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

    jsTypeSend = cJSON_GetObjectItem(jsRoot, CJSON_KEY_IR_TYPE_SEND);
    if (jsTypeSend == NULL)
    {
        os_trace("Get JSON TypeSend mqtt message fail\n");
        goto RESPONSE;
    }
    typesend = jsTypeSend->valueint;

    if (typesend == 1) {
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
                // Irrecv_pause();
                hw_led_stt_on();
                array_ir_control_handler[i]._handler(jsRoot);
                hw_led_stt_off();
                // resume();
                break;
            }
        }
    } else if (typesend == 2) {
        // send by time
        ir_send_data(jsRoot);
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

static void AC_Aqua_Callback(cJSON *jsRoot)
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Aqua_send(state_ir);

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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Samsung_send(state_ir);

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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Daikin_send(state_ir);

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

    state_ir_t state_ir;
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

static void AC_Daikin152_Callback(cJSON *jsRoot)
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Daikin152_send(state_ir);

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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Gree_send(state_ir);

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


static void AC_Tcl_Callback(cJSON *jsRoot)
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    Tcl112_send(state_ir);

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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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


static void AC_PanasonicAC_Callback(cJSON *jsRoot)
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    PanasonicAC_send(state_ir);
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

static void AC_PanasonicAC32_Callback(cJSON *jsRoot)
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    PanasonicAc32_send(state_ir);
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

static void AC_ElectraAC_Callback(cJSON *jsRoot)
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

    ElectraAc_send(state_ir);
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

    state_ir_t state_ir;
    state_ir.power = (power ? true : false);
    state_ir.degrees = (float)temp;
    state_ir.mode = mode;
    state_ir.fanspeed = fan;

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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    state_ir_t state_ir;
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

    m_wifi_get_instance().wifi_sync_rssi_cb();

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
    m_db_get_instance()->_request_sync_db();

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

extern void ble_loop(void);
extern void ble_init_first(void);

void app_handler_msg_task(void * arg)
{
    // m_data_t msg;
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
        if (has_mess) 
        {
            has_mess = 0;
            // os_trace("raw_data received: %s\r\n", msg.data);

            /* decode data */
            memset(raw_data, 0, sizeof(raw_data));
            aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
            if (aes_cbc128_decrypt((uint8_t *)message_rcv_mqtt.data, message_rcv_mqtt.length, (uint8_t *)raw_data, (size_t *)&raw_len) != AES_SUCCESS)
            {
                os_trace("Can not decrypt message\r\n");
                continue;
            }
            if (raw_len < 450) {
                printf("\r\ndecoded[%lu]: %s\r\n", raw_len, raw_data);
            }

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
        else if (xQueueReceive(pqueue_ble, &msg_ble, 1) == pdPASS) 
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
            if (raw_len < 450) {
                printf("\r\ndecoded[%lu]: %s\r\n", raw_len, raw_data);
            }

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