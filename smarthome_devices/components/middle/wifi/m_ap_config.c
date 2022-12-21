#include "m_ap_config.h"
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
#include <wifi_mgmr_ext.h>
#include <hal_wifi.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <blog.h>

static uint8_t ap_cfg_get_data = 0;

bool apconfig_parse_wifi_info(char *json)
{
    cJSON *js_Root, *jsGetObject;
    char temp[256];
    uint32_t mqtt_port = 0;
    m_mqtt_t mqtt = m_mqtt_get_instance();

    js_Root = cJSON_Parse(json);
    if (!js_Root)
        return false;

    /* get wifi ssid */
    jsGetObject = cJSON_GetObjectItem(js_Root, "ssid");
    if (!jsGetObject)
        goto json_parse_fail;
    m_wifi_get_instance().wifi_set_ssid_cb(jsGetObject->valuestring, strlen(jsGetObject->valuestring));

    /* get wifi password */
    jsGetObject = cJSON_GetObjectItem(js_Root, "pass");
    if (!jsGetObject)
        goto json_parse_fail;
    m_wifi_get_instance().wifi_set_passwd_cb(jsGetObject->valuestring, strlen(jsGetObject->valuestring));

    /* get server url */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_SERVER);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "mqtt://%s", jsGetObject->valuestring);
    mqtt.set_mqtt_server(temp, strlen(temp));

    /* get server port */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_PORT);
    if (!jsGetObject)
        return false;
    mqtt_port = jsGetObject->valueint;
    mqtt.set_mqtt_port(mqtt_port);

    /* get user name */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_USER);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "%s", jsGetObject->valuestring);
    mqtt.set_mqtt_user(temp, strlen(temp));

    /* get password */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_PASS);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "%s", jsGetObject->valuestring);
    mqtt.set_mqtt_passwd(temp, strlen(temp));

    /* get topic publish */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_TOPIC_PUB);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "%s", jsGetObject->valuestring);
    mqtt.set_mqtt_topic_pub(temp, strlen(temp));

    /* get topic subscribe */
    jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_TOPIC_SUB);
    if (!jsGetObject)
        goto json_parse_fail;
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "%s", jsGetObject->valuestring);
    mqtt.set_mqtt_topic_sub(temp, strlen(temp));

    m_db_get_instance()->_set_dev_pair_state(true);
    m_db_get_instance()->_request_sync_db();
    vTaskDelay(pdMS_TO_TICKS(100));

    ap_cfg_get_data = 1;

json_parse_fail:
    cJSON_Delete(js_Root);

    return true;
}

void ap_cfg_clear_state(void)
{
    ap_cfg_get_data = 0;
}

uint8_t is_ap_config_done(void)
{
    return ap_cfg_get_data;
}
