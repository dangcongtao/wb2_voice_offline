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
#include <hal_wifi.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <blog.h>

#include <bl_os_private.h>
#include <wifi_mgmr_ext.h>

static BL_Sem_t scan_semaphore = NULL;
uint32_t counter = 0;
wifi_mgmr_ap_item_t ap_ary_p[WIFI_SCAN_MAX_AP];

static uint8_t ap_cfg_get_data = 0;

static void m_cb_scan_complete(void *data, void *param)
{
    // wifi_mgmr_ap_item_t *ap_ary_ptr = (wifi_mgmr_ap_item_t *)data;
    // int status = *((int *)param);
    // os_trace("scan complete status: %d, ssid_len = %lu\r\n", status, ap_ary_ptr->ssid_len);

    if (scan_semaphore != NULL)
    {
        bl_os_sem_give(scan_semaphore);
    }
}

static void m_cb_scan_item_parse(wifi_mgmr_ap_item_t *env, uint32_t *param1, wifi_mgmr_ap_item_t *item)
{
    /*recall the env*/
    uint32_t counter = (*param1);

    /*scan overflow check*/
    if (counter >= WIFI_SCAN_MAX_AP)
    {
        /*XXX maybe we should warning here*/
        return;
    }

    wifi_mgmr_ap_item_t *ap_ary_ptr = (wifi_mgmr_ap_item_t *)env + counter; // get and move the ptr to the next item
    counter++;

    /*copy out scan data*/
    ap_ary_ptr->channel = item->channel;
    ap_ary_ptr->rssi = item->rssi;
    memcpy(ap_ary_ptr->bssid, item->bssid, sizeof(ap_ary_ptr->bssid));
    memcpy(ap_ary_ptr->ssid, item->ssid, sizeof(ap_ary_ptr->ssid)); // XXX boundary attack, copy too much data from wifi manager internal?
    ap_ary_ptr->ssid_tail[0] = '\0';
    ap_ary_ptr->ssid_len = strlen((char *)ap_ary_ptr->ssid);
    ap_ary_ptr->auth = item->auth;

    /*store back counter*/
    (*param1) = counter;
}

/*
@ overwrite of platform api
    int wifi_mgmr_all_ap_scan(wifi_mgmr_ap_item_t **ap_ary, uint32_t *num)
*/
void scan_ap_around(void)
{
    memset(ap_ary_p, 0, sizeof(wifi_mgmr_ap_item_t) * WIFI_SCAN_MAX_AP);

    scan_semaphore = bl_os_sem_create(0);
    if (NULL == scan_semaphore)
    {
        return;
    }

    wifi_mgmr_scan(ap_ary_p, m_cb_scan_complete); // trigger the scan

    if (0 == bl_os_sem_take(scan_semaphore, BL_OS_WAITING_FOREVER))
    {
        os_trace("wifi scan Done\r\n");
    }

    /*Start to scan the AP*/
    wifi_mgmr_scan_ap_all(ap_ary_p, &counter, m_cb_scan_item_parse);

    bl_os_sem_delete(scan_semaphore);

    // for (uint8_t i = 0; i < counter; i++)
    // {
    //     bl_os_printf("index[%02d]: rssi %3d, SSID %s\r\n",
    //                  i, ap_ary_p[i].rssi, ap_ary_p[i].ssid);
    // }
}

bool apconfig_parse_wifi_info(char *json)
{
    os_trace("apconfig_parse_wifi_info\r\n");
    
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
    os_trace("set wifi ssid\r\n");

    /* get wifi password */
    jsGetObject = cJSON_GetObjectItem(js_Root, "pass");
    if (!jsGetObject)
        goto json_parse_fail;
    m_wifi_get_instance().wifi_set_passwd_cb(jsGetObject->valuestring, strlen(jsGetObject->valuestring));
    os_trace("set wifi password\r\n");

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
        return false;
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

    os_trace("have database\r\n");

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
