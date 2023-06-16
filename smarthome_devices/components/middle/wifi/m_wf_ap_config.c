#include "m_wf_ap_config.h"
#include "cJSON.h"
#include "system_typedef.h"
#include "m_database.h"
#include "m_mqtt.h"
#include "system_typedef.h"
#include "aes_cbc128.h"
#include "base64.h"

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

static uint8_t ap_cfg_get_data = 0;
extern void signal_post(m_signal_t sig);

bool apconfig_parse_wifi_info(char *buf)
{
    os_trace("apconfig_parse_wifi_info\r\n");

    cJSON *jsRoot = NULL, *jsUid = NULL, *jsTimeStamp = NULL, *jsSsid = NULL, *jsWifiPwd = NULL;
    char *timestamp, *uid = NULL, *ssid = NULL, *pass = NULL;
    char *temp = NULL;
    char data[128];
    char *data_begin = NULL;
    char *data_end = NULL;

    /* pub */
    temp = strstr(buf, "name=\"data\"");
    if (temp == NULL)
        goto not_found_pub;
    /* begin */
    data_begin = strstr(temp, "\r\n\r\n");
    if (data_begin == NULL)
        goto not_found_pub;
    data_begin += strlen("\r\n\r\n");
    /* end */
    data_end = strstr(data_begin, "\r\n");
    if (data_end == NULL)
        goto not_found_pub;
    os_trace("end: %s\r\n", data_end);
    /* copy */
    memset(data, 0, sizeof(data));
    memcpy(data, data_begin, data_end - data_begin);


    jsRoot = cJSON_Parse(data);
    if (!jsRoot) {
        os_trace("Parse Json AP info post Fail\r\n");
        goto not_found_pub;
    }

    jsUid = cJSON_GetObjectItem(jsRoot, CJSON_KEY_AP_USER_ID);

    if (jsUid == NULL)
    {
        os_trace("Get JSON uid AP fail\n");
        goto not_found_pub;
    }
    uid = jsUid->valuestring;

    jsTimeStamp = cJSON_GetObjectItem(jsRoot, CJSON_KEY_TIMESTAMP);

    if (jsTimeStamp == NULL)
    {
        os_trace("Get JSON timestamp AP fail\n");
        goto not_found_pub;
    }
    timestamp = cJSON_PrintUnformatted(jsTimeStamp);

    jsSsid = cJSON_GetObjectItem(jsRoot, CJSON_KEY_SSID);
    if (jsSsid == NULL)
    {
        os_trace("Get JSON ssid fail\n");
        goto not_found_pub;
    }
    ssid = jsSsid->valuestring;

    jsWifiPwd = cJSON_GetObjectItem(jsRoot, CJSON_KEY_PASS);
    if (jsWifiPwd == NULL)
    {
        os_trace("Get JSON password fail\n");
        goto not_found_pub;
    }
    pass = jsWifiPwd->valuestring;

    char topic_pub[256], topic_sub[256];
    memset(topic_pub, 0, sizeof(topic_pub));
    memset(topic_sub, 0, sizeof(topic_sub));

    sprintf(topic_pub, "u/%s/%s/%s/ok", uid, m_db_get_instance()->_get_dev_id(), (uint32_t) timestamp);
    sprintf(topic_sub, "u/%s/%s/%s", uid, m_db_get_instance()->_get_dev_id(), (uint32_t) timestamp);

    /* save */
    os_trace("set_mqtt_topic_pub[%d]: %s\r\n", strlen(topic_pub), topic_pub);
    m_mqtt_get_instance().set_mqtt_topic_pub(topic_pub, strlen(topic_pub));

    /* save */
    os_trace("set_mqtt_topic_sub[%d]: %s\r\n", strlen(topic_sub), topic_sub);
    m_mqtt_get_instance().set_mqtt_topic_sub(topic_sub, strlen(topic_sub));

    /* ssid */
    m_wifi_get_instance().wifi_set_ssid_cb(ssid, strlen(ssid));
    os_trace("set wifi ssid %s\r\n", ssid);

    /* pass */
    m_wifi_get_instance().wifi_set_passwd_cb(pass, strlen(pass));
    os_trace("set wifi password %s\r\n", pass);

    // ========= SYNC ===========
    m_db_get_instance()->_set_dev_pair_state(true);
    m_db_get_instance()->_request_sync_db();
    vTaskDelay(pdMS_TO_TICKS(2000));

    ap_cfg_get_data = 1;
    signal_post(SIG_AP_CFG_DONE);

    return true;

not_found_pub:
    os_trace("NULL\r\n");
    return false;
}

void m_ap_cfg_clear_state(void)
{
    ap_cfg_get_data = 0;
}

uint8_t is_ap_config_done(void)
{
    return ap_cfg_get_data;
}
