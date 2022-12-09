/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "m_http_server.h"
#include <FreeRTOS.h>
#include <task.h>

#include "m_database.h"
#include "m_mqtt.h"
#include "m_ap_config.h"
#include "m_smartconfig.h"
#include "app_config.h"

// static const char *TAG = "m_http_server";
// extern m_ap_around_t access_point_around;

// static char resp_str[2048];
// static uint8_t enable_service = 0;
// static httpd_handle_t s_server = NULL;

// static void stop_webserver(void);

// /* An HTTP WIFICONFIG POST handler */
// static esp_err_t http_apconfig_step1(httpd_req_t *req)
// {
//     char buf[512];
//     int ret, remaining = req->content_len;

//     os_trace("\tApconfig step 1\n");
//     memset(buf, 0, sizeof(buf));

//     while (remaining > 0) {
//         /* Read the data for the request */
//         if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
//             if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//                 /* Retry receiving if timeout occurred */
//                 continue;
//             }
//             return ESP_FAIL;
//         }
        
//         remaining -= ret;

//     }

//     /* Send response */
//     httpd_resp_send_chunk(req, "{\"result\": 1}", strlen("{\"result\": 1}"));
//     httpd_resp_send_chunk(req, NULL, 0);

//     /* parser data to get info of wifi & mqtt server */
//     os_trace("m_http_server receive: [%s]", buf);
//     apconfig_parse_wifi_info(buf);
    
//     stop_webserver();

//     return ESP_OK;
// }

// httpd_uri_t wificonfig = {
//     .uri       = "/wificonfig",
//     .method    = HTTP_POST,
//     .handler   = http_apconfig_step1,
//     .user_ctx  = NULL
// };

// /* An HTTP GET handler */
// static  esp_err_t http_apconfig_step2(httpd_req_t *req)
// {
//     int n = 0;

//     os_trace("\tApconfig step 2\n");

//     /* create respond */
//     memset(resp_str, 0, sizeof(resp_str));
//     n = sprintf(resp_str, "{\"APs\": [");
//     for (uint8_t i = 0; i < MIN(access_point_around.num_ap_available, MAX_SSID_SCAN); i++) {
//         n += sprintf(&resp_str[n], "{\"id\": %d, \"rssi\": %d, \"ssid\": \"%s\"},",
//                                     i, access_point_around.rssi[i], access_point_around.ssid[i]);
//     }
//     n += sprintf(&resp_str[n-1], "], \"device_id\": \"%s\", \"extra\": %d, \"type\": \"%s\"}", 
//                                 m_db_get_instance()->_get_dev_id(), EXTRAS, gs_dev_type[device_type]);

//      /* send wifi ap around */
//     httpd_resp_send(req, resp_str, strlen(resp_str));
//     os_trace("%s\n", resp_str);

//     return ESP_OK;
// }

// httpd_uri_t apconfig = {
//     .uri       = "/apconfig",
//     .method    = HTTP_GET,
//     .handler   = http_apconfig_step2,
//     .user_ctx  = NULL
// };

// static  esp_err_t http_smartconfig_step1(httpd_req_t *req)
// {
//     os_trace("\nSmartconfig http step 1 \n");

//     /* Send response with custom headers and body set as the string passed in user context*/
//     memset(resp_str, 0, sizeof(resp_str));
//     sprintf(resp_str, "{\"id\":\"%s\",\"version\":\"%s\"}",  
//                         m_db_get_instance()->_get_dev_id(),
//                         FIRMWARE_VERSION);
    
//     httpd_resp_send(req, resp_str, strlen(resp_str));
//     os_trace("%s\n", resp_str);

//     return ESP_OK;
// }

// httpd_uri_t root = {
//     .uri       = "/",
//     .method    = HTTP_GET,
//     .handler   = http_smartconfig_step1,
//     .user_ctx  = NULL
// };

// /* An HTTP POST handler */
// static esp_err_t http_smartconfig_step2(httpd_req_t *req)
// {
//     char buf[1024];
//     int ret, remaining = req->content_len;

//     os_trace("\nSmartconfig http step 2 \n");

//     memset(buf, 0, sizeof(buf));
//     while (remaining > 0) {
//         if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
//             if (ret == HTTPD_SOCK_ERR_TIMEOUT)
//                 continue;

//             return ESP_FAIL;
//         }
//         remaining -= ret;
//     }

//     /* Send response */
//     httpd_resp_send_chunk(req, "{\"result\": 1}", strlen("{\"result\": 1}"));
//     httpd_resp_send_chunk(req, NULL, 0);

//     /* parser data to get info of mqtt server */
//     os_trace("m_http_server receive: [%s]", buf);
//     if (m_smartconfig_parse_data(buf) != true) {
//         os_trace("Smartconfig - http parse wifi fail\n");
//     }

//     stop_webserver();

//     return ESP_OK;
// }

// httpd_uri_t setting = {
//     .uri       = "/setting/",
//     .method    = HTTP_POST,
//     .handler   = http_smartconfig_step2,
//     .user_ctx  = NULL
// };

// static void stop_webserver(void)
// {
//     os_trace("stop web server\n");
//     enable_service = 0;
// }

// void start_webserver(void)
// {
//     os_trace("Start web server\n");
//     enable_service = 1;
// }

// void http_server_init(void)
// {
//     ESP_LOGI(TAG, "Starting http s_server");

//     if (s_server == NULL) {
//         httpd_config_t config = HTTPD_DEFAULT_CONFIG();

//         if (httpd_start(&s_server, &config) == ESP_OK) {
//             ESP_LOGI(TAG, "Registering URI handlers");

//             httpd_register_uri_handler(s_server, &root);
//             httpd_register_uri_handler(s_server, &apconfig);
//             httpd_register_uri_handler(s_server, &setting);
//             httpd_register_uri_handler(s_server, &wificonfig);
//         } else {
//             os_trace("Cannot start http server");
//         }
//     }
// }