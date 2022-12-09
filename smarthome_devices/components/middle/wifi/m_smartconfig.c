#include "m_smartconfig.h"

// #include <string.h>
// #include <stdlib.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include "nvs_flash.h"
// #include "tcpip_adapter.h"
// #include "esp_smartconfig.h"
// #include "smartconfig_ack.h"

// #include "app_config.h"
// #include "m_wifi.h"
// #include "m_mqtt.h"
// #include "m_http_server.h"
// #include "m_led_stt.h"
// #include "m_database.h"
// #include "cJSON.h"

// /* The event group allows multiple bits for each event,
//    but we only care about one event - are we connected
//    to the AP with an IP? */
// static const char* TAG = "smartconfig_example";

// // #define ESP_SMARTCOFNIG_TYPE      SC_TYPE_ESPTOUCH_V2 //SC_TYPE_ESPTOUCH 
// #define ESP_SMARTCOFNIG_TYPE         SC_TYPE_ESPTOUCH 

// static smartconfig_state_t smart_config_step = 0;
// static uint8_t sc_connected_wifi = 0;
// static uint8_t sc_got_server_info = 0;

// extern void m_wifi_retry_connect(void);

// static void smartconfig_event_handler(void* arg, esp_event_base_t event_base,
//                           int32_t event_id, void* event_data)
// {
//     /* wifi event */
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         ESP_LOGI(TAG, "Smart config Event STA START");
//     }
//     /* auto reconnect wifi */
//     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         ESP_LOGI(TAG, "Wifi disconnected");
//         ESP_ERROR_CHECK(esp_wifi_connect());
//         // m_wifi_get_instance().wifi_set_state_cb(WIFI_DISCONNECT);
//     }
    
//     /* IP event */
//     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         ESP_LOGI("Smartconfig: WiFi Connected, got ip:", ip4addr_ntoa(&event->ip_info.ip));
        
//         char ip[16];
//         memset(ip, 0, sizeof(ip));
//         sprintf(ip, "%s", ip4addr_ntoa(&event->ip_info.ip));
//         m_wifi_get_instance().wifi_save_ip(ip, strlen(ip));
//     }

//     /* smart config event */
//     else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
//         ESP_LOGI(TAG, "Scan done");
//     }
//     else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
//         ESP_LOGI(TAG, "Found channel");
//     }
//     else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {

//         smartconfig_event_got_ssid_pswd_t* evt = (smartconfig_event_got_ssid_pswd_t*)event_data;
//         wifi_config_t wifi_config;
//         uint8_t rvd_data[33] = { 0 };

//         /* save wifi info */
//         m_wifi_get_instance().wifi_set_ssid_cb((char*)evt->ssid, strlen((char*)evt->ssid));
//         m_wifi_get_instance().wifi_set_passwd_cb((char*)evt->password, strlen((char*)evt->password));
//         ESP_LOGI(TAG, "Got SSID and password");
//         ESP_LOGI(TAG, "SSID:%s", m_wifi_get_instance().wifi_info->_ssid);
//         ESP_LOGI(TAG, "PASSWORD:%s", m_wifi_get_instance().wifi_info->_passwd);

//         m_led_stt_get_instance()._set_led_stt_not_config_cb();

//         /* config wifi with ssid and password was received */
//         bzero(&wifi_config, sizeof(wifi_config_t));
//         memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
//         memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
//         wifi_config.sta.bssid_set = evt->bssid_set;

//         if (wifi_config.sta.bssid_set == true) {
//             memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
//         }

//         if (evt->type == SC_TYPE_ESPTOUCH_V2) {
//             ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
//             ESP_LOGI(TAG, "RVD_DATA:%s", rvd_data);
//         }

//         /* start wifi connection */
//         ESP_ERROR_CHECK(esp_wifi_disconnect());
//         ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//         ESP_ERROR_CHECK(esp_wifi_connect());
//     }
//     else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
//         ESP_LOGI(TAG, "smartconfig over");
//         esp_smartconfig_stop();
//         sc_connected_wifi = 1;
//     }
// }

// bool m_smartconfig_parse_data(char* json)
// {
//     cJSON *js_Root = cJSON_Parse(json);
//     if(!js_Root) return false;

//     /* get server url */
//     cJSON *jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_SERVER);
//     if(!jsGetObject) goto json_parse_fail;
//     memset(m_mqtt_get_instance().mqtt_info->server, 0, sizeof(m_mqtt_get_instance().mqtt_info->server));
//     sprintf(m_mqtt_get_instance().mqtt_info->server, "mqtt://%s", jsGetObject->valuestring);

//     /* get server port */
//     jsGetObject = cJSON_GetObjectItem(js_Root, CJSON_KEY_S_PORT);
//     if(!jsGetObject) goto json_parse_fail;
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
    

//     // m_mqtt_get_instance()._mqtt_start();
//     m_db_get_instance()->_set_dev_pair_state(true);
//     m_db_get_instance()->_request_sync_db();;
//     vTaskDelay(pdMS_TO_TICKS(100));
    
// 	sc_got_server_info = 1;

// json_parse_fail:
//     cJSON_Delete(js_Root);

//     return true;
// }

// void smartconfig_init(void)
// {
//     os_trace( "\tdungnt98 smartconfig init\n");

//     esp_smartconfig_stop();
//     esp_wifi_stop();

//     tcpip_adapter_init();
//     ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
//     ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &smartconfig_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL)); /* smart config event */

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     ESP_ERROR_CHECK(esp_smartconfig_set_type(ESP_SMARTCOFNIG_TYPE));
//     smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
// }

// void reset_smartconfig(void)
// {
//     smart_config_step = SC_START;
//     sc_connected_wifi = 0;
// 	sc_got_server_info = 0;

//     ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler));
//     ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &smartconfig_event_handler));
//     ESP_ERROR_CHECK(esp_event_handler_unregister(SC_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler)); /* smart config event */
// }

// smartconfig_state_t start_smartconfig(void)
// {
//     switch (smart_config_step)
//     {
//     case SC_START: {
//         os_trace("\tdungnt98 smartconfig start\n");
// 		http_server_init();
//         m_mqtt_get_instance()._mqtt_stop();
//         smartconfig_init();
//         start_webserver();
//         smart_config_step = SC_WAIT_CONFIG;
//     } break;

//     case SC_WAIT_CONFIG: {
//         if (sc_connected_wifi) {
//             smart_config_step = SC_WAIT_INFO;
//             sc_connected_wifi = 0;
//             esp_smartconfig_stop();
//         }
//     } break;

// 	case SC_WAIT_INFO: {
// 		if (sc_got_server_info) {
// 			sc_got_server_info = 0;
// 			os_trace("\tdungnt98 smartconfig done\n");	
// 			smart_config_step = SMARTCONFIG_DONE;
// 		}
// 	} break;

//     default:
//         break;
//     }

//     return smart_config_step;
// }

// /*
// hunonic esp8266 smart config

// 1. Smartconfig done, esp connect to wifi
// 2. esp create http server.
// 3. smartphone connect to http server.
// 4. esp respond smartphone: {"id":"HUN1234567890wswitch1","version":210702}
// 5. smartphone send : {"server":"mqtt2.hunonicpro.com","su":"bestbug","sp":"bigbugdmm","port":1883,"pub":"topic pub","sub":"topic sub"}
// 6. esp respond smartphone that processing was successful.

// */