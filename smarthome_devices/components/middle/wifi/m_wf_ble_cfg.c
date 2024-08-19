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
    m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_WIFI);
    m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_MQTT);
    vTaskDelay(pdMS_TO_TICKS(2000));

    smart_cfg_get_data = 1;
    signal_post(SIG_SMART_CFG_DONE);

    return 1;

json_parse_fail:
    return 0;
}


bool save_mqtt_info(char *json)
{
    os_trace("%s\r\n");

    int serverCount = 0;
    m_mqtt_t mqtt = m_mqtt_get_instance();
    cJSON *brokers, *broker, *server, *port, *user, *pass, *sermqtt;

    cJSON *js_Root = cJSON_Parse(json);
    if (js_Root != NULL)
    {
        sermqtt = cJSON_GetObjectItem(js_Root, "vermqtt");
        mqtt.set_ver_mqtt(sermqtt->valueint);
        /* 0 */
        brokers = cJSON_GetObjectItem(js_Root, "broker");
        if (brokers != NULL)
        {
            if (brokers->type == cJSON_Array)
            {
                serverCount = cJSON_GetArraySize(brokers);
                os_trace("serverCount: %d\r\n", serverCount);
                mqtt.set_num_mqtt_broker(serverCount);

                if (serverCount > 0) {
                    m_mqtt_get_info_reference()->broker_in_used = 0;
                }

                for (int i = 0; i < serverCount; i++)
                {
                    broker = cJSON_GetArrayItem(brokers, i);
                    if (broker != NULL)
                    {
                        server = cJSON_GetObjectItem(broker, "server");
                        port = cJSON_GetObjectItem(broker, "port");
                        user = cJSON_GetObjectItem(broker, "user");
                        pass = cJSON_GetObjectItem(broker, "pass");

                        if ((server != NULL) && (port != NULL) && (user != NULL) && (pass != NULL))
                        {
                            m_mqtt_server_t server_inf;
                            memset(&server_inf, 0, sizeof(m_mqtt_server_t));
                            memcpy(server_inf.server, server->valuestring, strlen(server->valuestring));
                            memcpy(server_inf.username, user->valuestring, strlen(user->valuestring));
                            memcpy(server_inf.passwd, pass->valuestring, strlen(pass->valuestring));
                            uint32_t mqtt_port = atoi(port->valuestring);
                            server_inf.port = mqtt_port;
                            
                            m_mqtt_get_instance().set_server_info(i, &server_inf);
                        }
                        else
                        {
                            os_trace("%s Data is NULL\r\n", __func__);
                        }
                    }
                    else
                    {
                        os_trace("broker NULL\r\n");
                        cJSON_Delete(js_Root);
                        return false;
                    }
                }
            }
            else
            {
                os_trace("broker is not array\r\n");
                cJSON_Delete(js_Root);
                return false;
            }
        }
        else
        {
            os_trace("brokers NULL\r\n");
            cJSON_Delete(js_Root);
            return false;
        }
    }
    else
    {
        os_trace("%s cJSON_Parse fail\r\n", __func__);
        return false;
    }

    /* sync */
    m_db_get_instance()->_request_sync_db(BIT_FLAG_SYNC_MQTT);
    vTaskDelay(pdMS_TO_TICKS(2000));
    cJSON_Delete(js_Root);
    return true;
}

void m_ble_cfg_clear_state(void)
{
    smart_cfg_get_data = 0;
}

uint8_t is_ble_cfg_done(void)
{
    return smart_cfg_get_data;
}
