#include "m_wf_ap_config.h"
#include "cJSON.h"
#include "system_typedef.h"
#include "m_database.h"
#include "m_mqtt.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/tcpip.h>
#include <hal_wifi.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <blog.h>

#include <bl_os_private.h>
#include <wifi_mgmr_ext.h>

extern void signal_post(m_signal_t sig);

static uint8_t smart_cfg_get_data = 0;

uint8_t ble_cfg_parse_data(cJSON *js_Root)
{
    char temp[256];
    cJSON *jsGetObject;
    m_mqtt_t mqtt = m_mqtt_get_instance();

    /* get wifi ssid */
    jsGetObject = cJSON_GetObjectItem(js_Root, "ssid");
    if (!jsGetObject)
        goto json_parse_fail;
    m_wifi_get_instance().wifi_set_ssid_cb(jsGetObject->valuestring, strlen(jsGetObject->valuestring));
    os_trace("set wifi ssid\r\n");

    /* get wifi password */
    jsGetObject = cJSON_GetObjectItem(js_Root, "pass");
    if (!jsGetObject)
        goto json_parse_fail;
    m_wifi_get_instance().wifi_set_passwd_cb(jsGetObject->valuestring, strlen(jsGetObject->valuestring));
    os_trace("set wifi password\r\n");

    /* get topic publish */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_TOPIC_PUB);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    snprintf(temp, sizeof(temp), "%s", jsGetObject->valuestring);
    mqtt.set_mqtt_topic_pub(temp, strlen(temp));
    os_trace("set_mqtt_topic_pub\r\n");

    /* get topic subscribe */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_TOPIC_SUB);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    snprintf(temp, sizeof(temp), "%s", jsGetObject->valuestring);
    mqtt.set_mqtt_topic_sub(temp, strlen(temp));

    /* save database */
    m_db_get_instance()->_set_dev_pair_state(true);
    m_db_get_instance()->_request_sync_db();
    vTaskDelay(pdMS_TO_TICKS(2000));

    smart_cfg_get_data = 1;
    signal_post(SIG_SMART_CFG_DONE);

    return 1;

json_parse_fail:
    return 0;
}

/*
{"server":"mqtt2.hunonicpro.com","port":"1883","user":"bestbug","pass":"bigbugdmm",
"key":"n1vXZIQ4b1CpWJNH6E03wg==","iv":"qViTR+hNN8KJ4QtG5RW62w=="} 
*/
bool save_mqtt_info(char *json)
{
    os_trace("apconfig_parse_wifi_info\r\n");
    
    cJSON *js_Root, *jsGetObject;
    char temp[256];
    uint32_t mqtt_port = 0;
    m_mqtt_t mqtt = m_mqtt_get_instance();

    js_Root = cJSON_Parse(json);
    if (!js_Root)
        return false;

    /* get server url */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_SERVER);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    snprintf(temp, sizeof(temp), "mqtt://%s", jsGetObject->valuestring);
    mqtt.set_mqtt_server(temp, strlen(temp));
    os_trace("set_mqtt_server\r\n");

    /* get server port */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_PORT);
    if (!jsGetObject)
        goto json_parse_fail;
    mqtt_port = jsGetObject->valueint;
    mqtt.set_mqtt_port(mqtt_port);
    os_trace("set_mqtt_port\r\n");

    /* get user name */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_USER);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    snprintf(temp, sizeof(temp), "%s", jsGetObject->valuestring);
    mqtt.set_mqtt_user(temp, strlen(temp));
    os_trace("set_mqtt_user\r\n");

    /* get password */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_PASS);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    snprintf(temp, sizeof(temp), "%s", jsGetObject->valuestring);
    mqtt.set_mqtt_passwd(temp, strlen(temp));
    os_trace("set_mqtt_passwd\r\n");

    m_db_get_instance()->_request_sync_db();
    vTaskDelay(pdMS_TO_TICKS(2000));

    cJSON_Delete(js_Root);

    signal_post(SIG_MQTT_GOT_INFO);

    return true;

json_parse_fail:
    cJSON_Delete(js_Root);
    return false;
}

void m_ble_cfg_clear_state(void)
{
    smart_cfg_get_data = 0;
}

uint8_t is_ble_cfg_done(void)
{
    return smart_cfg_get_data;
}
