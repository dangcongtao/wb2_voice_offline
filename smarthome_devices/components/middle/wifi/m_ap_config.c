/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "m_ap_config.h"

// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// #include "esp_smartconfig.h"
// #include "m_http_server.h"
// #include "cJSON.h"

// #include "m_wifi.h"
// #include "m_mqtt.h"
// #include "m_database.h"
// #include "app_config.h"

// #define EXAMPLE_MAX_STA_CONN       3
// #define EXAMPLE_ESP_WIFI_PASS       ""

// static const char *TAG = "wifi softAP";

// static m_apconfig_step_t ap_config_step = START_APCONFIG;
// m_ap_around_t access_point_around;
// static uint8_t scan_wifi_done = 0;
// static uint8_t user_config_done = 0;

// static void wifi_event_handler(void* arg, esp_event_base_t event_base,
//                                     int32_t event_id, void* event_data)
// {
//     if (event_id == WIFI_EVENT_AP_STACONNECTED) {
//         wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
//         ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
//     }
//     else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
//         wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
//         ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
//     }
// }

// static void scan_done_handler(void* arg, esp_event_base_t event_base,
//                               int32_t event_id, void* event_data)
// {
//     uint8_t i;
//     uint16_t sta_number = 0;
//     wifi_ap_record_t *ap_list_buffer;
    
//     esp_wifi_scan_get_ap_num(&sta_number);

//     if (sta_number <= 0)
//         return;
    
//     access_point_around.num_ap_available = sta_number > MAX_SSID_SCAN ? MAX_SSID_SCAN : sta_number;

//     ap_list_buffer = malloc(sta_number * sizeof(wifi_ap_record_t));
//     if (ap_list_buffer == NULL) {
//         ESP_LOGE(TAG, "Failed to malloc buffer to print scan results");
//         return;
//     }

//     if (esp_wifi_scan_get_ap_records(&sta_number,(wifi_ap_record_t *)ap_list_buffer) == ESP_OK) {
//         for (i = 0; i < sta_number; i++) {
//             ESP_LOGI(TAG, "[%s][rssi=%d]", ap_list_buffer[i].ssid, ap_list_buffer[i].rssi);

//             if (strlen((char*)ap_list_buffer[i].ssid) < 32) {
//                 memset(access_point_around.ssid[i], 0, sizeof(access_point_around.ssid[i]));
//                 sprintf(access_point_around.ssid[i], "%s", ap_list_buffer[i].ssid);
//                 access_point_around.rssi[i] = ap_list_buffer[i].rssi;
//             }
//         }
//     }

//     for (i = 0; i < sta_number; i++) {
//         os_trace("{\"id\": %d, \"rssi\": %d, \"ssid\": \"%s\"}\n", i, access_point_around.rssi[i], access_point_around.ssid[i]);   
//     }

//     free(ap_list_buffer);

//     ESP_LOGI(TAG, "sta scan done");

//     scan_wifi_done = 1;
// }

// static void m_apconfig_wifi_init(void)
// {
//     esp_smartconfig_stop();
//     esp_wifi_restore();
//     esp_wifi_stop();
//     tcpip_adapter_init();

//     ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &scan_done_handler, NULL));
//     ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
//     ESP_ERROR_CHECK( esp_wifi_start());
// }

// static void wifi_start_scan_ap(void)
// {
//     ESP_LOGI(TAG, "ap-config start access point");

//     wifi_scan_config_t scan_config = { 0 };
//     ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK( esp_wifi_scan_start(&scan_config, false));
// }


// static void start_AccessPoint(void)
// {
//     /* start AP */
//     esp_wifi_restore();
//     esp_wifi_stop();

//     tcpip_adapter_init();
//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

//     wifi_config_t wifi_config = {
//         .ap = {
//             .password = EXAMPLE_ESP_WIFI_PASS,
//             .max_connection = EXAMPLE_MAX_STA_CONN,
//             .authmode = WIFI_AUTH_WPA_WPA2_PSK
//         },
//     };

//     memset(wifi_config.ap.ssid, 0, sizeof(wifi_config.ap.ssid));
//     strncpy((char*)wifi_config.ap.ssid, m_db_get_instance()->_get_dev_id(),
//             strlen(m_db_get_instance()->_get_dev_id()) > sizeof(wifi_config.ap.ssid) ? strlen(m_db_get_instance()->_get_dev_id()) : sizeof(wifi_config.ap.ssid));
//     strcat((char*)wifi_config.ap.ssid, "@");
//     wifi_config.ap.ssid_len = strlen((char*)wifi_config.ap.ssid);

//     if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
//         wifi_config.ap.authmode = WIFI_AUTH_OPEN;
//     }

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
//              wifi_config.ap.ssid, EXAMPLE_ESP_WIFI_PASS);
// }


// bool apconfig_parse_wifi_info(char* json)
// {
//     cJSON *js_Root, *jsGetObject;
    
//     js_Root = cJSON_Parse(json);
//     if(!js_Root) return false;

//     /* get ssid */
//     jsGetObject = cJSON_GetObjectItem(js_Root, "ssid");
//     if(!jsGetObject) goto json_parse_fail;
//     m_wifi_get_instance().wifi_set_ssid_cb(jsGetObject->valuestring, strlen(jsGetObject->valuestring));

//     /* get password */
//     jsGetObject = cJSON_GetObjectItem(js_Root, "pass");
//     if(!jsGetObject) goto json_parse_fail;
//     m_wifi_get_instance().wifi_set_passwd_cb(jsGetObject->valuestring, strlen(jsGetObject->valuestring));

//     /* get server url */
//     jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_SERVER);
//     if(!jsGetObject) goto json_parse_fail;
//     memset(m_mqtt_get_instance().mqtt_info->server, 0, sizeof(m_mqtt_get_instance().mqtt_info->server));
//     sprintf(m_mqtt_get_instance().mqtt_info->server, "mqtt://%s", jsGetObject->valuestring);

//     /* get server port */
//     jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_PORT);
//     if(!jsGetObject) return false;
//     m_mqtt_get_instance().mqtt_info->port =  jsGetObject->valueint;

//     /* get user name */
//     jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_USER);
//     if(!jsGetObject) goto json_parse_fail;
//     memset(m_mqtt_get_instance().mqtt_info->username, 0, sizeof(m_mqtt_get_instance().mqtt_info->username));
//     sprintf(m_mqtt_get_instance().mqtt_info->username, "%s", jsGetObject->valuestring);

//     /* get pass word */
//     jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_PASS);
//     if(!jsGetObject) goto json_parse_fail;
//     memset(m_mqtt_get_instance().mqtt_info->passwd, 0, sizeof(m_mqtt_get_instance().mqtt_info->passwd));
//     sprintf(m_mqtt_get_instance().mqtt_info->passwd, "%s", jsGetObject->valuestring);
    
//     /* get topic publish */
//     jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_TOPIC_PUB);
//     if(!jsGetObject) goto json_parse_fail;
//     memset(m_mqtt_get_instance().mqtt_info->topic_pub, 0, sizeof(m_mqtt_get_instance().mqtt_info->topic_pub));
//     sprintf(m_mqtt_get_instance().mqtt_info->topic_pub, "%s", jsGetObject->valuestring);

//     /* get topic subscribe */
//     jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_TOPIC_SUB);
//     if(!jsGetObject) goto json_parse_fail;
//     memset(m_mqtt_get_instance().mqtt_info->topic_sub, 0, sizeof(m_mqtt_get_instance().mqtt_info->topic_sub));
//     sprintf(m_mqtt_get_instance().mqtt_info->topic_sub, "%s", jsGetObject->valuestring);


//     m_db_get_instance()->_set_dev_pair_state(true);
//     m_db_get_instance()->_request_sync_db();;
//     vTaskDelay(pdMS_TO_TICKS(100));

//     user_config_done = 1;

//     m_db_get_instance()->_request_sync_db();;
//     vTaskDelay(pdMS_TO_TICKS(100));

// json_parse_fail:
//     cJSON_Delete(js_Root);

//     return true;
// }

// void reset_ap_config(void)
// {
//     ESP_LOGI(TAG, "ap-config start");

//     m_mqtt_get_instance()._mqtt_stop();

//     ap_config_step = START_APCONFIG;
//     scan_wifi_done = 0;
//     user_config_done = 0;
// }

// extern void m_wifi_connect(char* ssid, char* password);
// m_apconfig_step_t apconfig_start(void)
// {
//     switch (ap_config_step)
//     {
//     case START_APCONFIG: {
//         m_apconfig_wifi_init();
//         start_webserver();
//         ap_config_step  = SCAN_WIFI_AP;
//     } break;

//     case SCAN_WIFI_AP: {
//         wifi_start_scan_ap();
//         ap_config_step = WAIT_WIFI_SCAN;
//     } break;

//     case WAIT_WIFI_SCAN: {
//         /* do nothing, waitting for event scan done */
//         if (scan_wifi_done)
//             ap_config_step = START_ACCESS_POINT;
//     } break;

//     case START_ACCESS_POINT: {
//         start_AccessPoint();
//         ap_config_step = WAIT_USER_CONFIG;
//     } break;

//     case WAIT_USER_CONFIG: {
//         if (user_config_done)
//             ap_config_step = CONNECT_WIFI;
//     } break;

//     case CONNECT_WIFI: {
//         os_trace("apconfig connect wifi\n");
//         ap_config_step = AP_CONFIG_DONE;
//     } break;
    
//     default:
//         break;
//     }

//     return ap_config_step;
// }
