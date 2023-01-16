#include "m_http_services.h"
#include <string.h>
#include <stdint.h>

#include "app_config.h"
#include "system_typedef.h"
#include "m_database.h"
#include "m_wf_ap_config.h"
#include "m_wf_ble_cfg.h"
#include <wifi_mgmr_ext.h>
#include "m_wf_scan.h"

#define MAX_NUM_SERVICES    4
#define MAX_RESP_LEN        1024

char* m_apconfig_step1(char* req);
char* m_apconfig_step2(char* req);

extern uint32_t counter;
extern wifi_mgmr_ap_item_t ap_ary_p[WIFI_SCAN_MAX_AP];
extern void ble_send_data(char *data, uint32_t length);

http_service_t apconfig = {
    .uri       = "/apconfig",
    .method    = "GET",
    .handler   = m_apconfig_step1,
};

http_service_t wificonfig = {
    .uri       = "/wificonfig",
    .method    = "POST",
    .handler   = m_apconfig_step2,
};

char* m_apconfig_step1(char *req)
{
    int n = 0;

    os_trace("\tAPCONFIG STEP 1\r\n");

    /* show wifi around me */
    char* resp = malloc(sizeof(char) * MAX_RESP_LEN);
    memset(resp, 0, sizeof(resp));

    n = sprintf(resp, "{\"APs\": [");
    for (uint8_t i = 0; i < counter; i++)
    {
        n += sprintf(&resp[n], "{\"id\": %d, \"rssi\": %d, \"ssid\": \"%s\"},",
                     i, ap_ary_p[i].rssi, ap_ary_p[i].ssid);
    }
    n += sprintf(&resp[n - 1], "], \"device_id\": \"%s\", \"extra\": %d, \"type\": \"%s\"}",
                 m_db_get_instance()->_get_dev_id(), EXTRAS, gs_dev_type[device_type]);

    return resp;
}

char* m_apconfig_step2(char* req)
{
    os_trace("APCONFIG STEP 2\r\n");

    char* temp = strstr(req, "\r\n\r\n");
    if (temp == NULL) {
        os_trace("not found http header ending\r\n");
        return NULL;
    }
    temp += strlen("\r\n\r\n");

    /* parser data to get info of wifi & mqtt server */
    apconfig_parse_wifi_info(temp);

    /* Send response */
    char* resp = malloc(sizeof(char) * MAX_RESP_LEN);
    memset(resp, 0, sizeof(resp));
    sprintf(resp, "{\"result\": 1}");

    return resp;
}

void m_ble_config_step1(void)
{
    os_trace("\tm_ble_config_step1\r\n");

    /* show wifi around me */
    char resp[256];
    memset(resp, 0, sizeof(resp));
    sprintf(resp, "{\"cmd\": 1, \"device_id\": \"%s\", \"type\": \"%s\", \"lengthWf\": %d}",
                 m_db_get_instance()->_get_dev_id(), gs_dev_type[device_type], counter);
    ble_send_data(resp, strlen(resp));

    uint8_t i = 0;
    for (i = 0; i < counter - 2; i += 3)
    {
        memset(resp, 0, sizeof(resp));
        sprintf(resp, "{\"cmd\": 2, \"wfs\": ["
                        "{\"rssi\": %d, \"ssid\": \"%s\"}," 
                        "{\"rssi\": %d, \"ssid\": \"%s\"},"
                        "{\"rssi\": %d, \"ssid\": \"%s\"}"
                        "]}",
                        ap_ary_p[i].rssi, ap_ary_p[i].ssid,
                        ap_ary_p[i+1].rssi, ap_ary_p[i+1].ssid,
                        ap_ary_p[i+2].rssi, ap_ary_p[i+2].ssid);
        ble_send_data(resp, strlen(resp));
    }

    if (counter % 3 == 1) {
        memset(resp, 0, sizeof(resp));
        sprintf(resp, "{\"cmd\": 2, \"wfs\": ["
                        "{\"rssi\": %d, \"ssid\": \"%s\"}"
                        "]}",
                        ap_ary_p[i].rssi, ap_ary_p[i].ssid);
        ble_send_data(resp, strlen(resp));
    }
    else if (counter % 3 == 2) {
        memset(resp, 0, sizeof(resp));
        sprintf(resp, "{\"cmd\": 2, \"wfs\": ["
                        "{\"rssi\": %d, \"ssid\": \"%s\"},"
                        "{\"rssi\": %d, \"ssid\": \"%s\"}"
                        "]}",
                        ap_ary_p[i].rssi, ap_ary_p[i].ssid,
                        ap_ary_p[i+1].rssi, ap_ary_p[i+1].ssid);
        ble_send_data(resp, strlen(resp));
    }

    memset(resp, 0, sizeof(resp));
    sprintf(resp, "{\"cmd\": 2, \"wfs\": []}");
    ble_send_data(resp, strlen(resp));
}

void m_ble_config_step2(void)
{
    os_trace("\tm_ble_config_step1\r\n");

    /* show wifi around me */
    char resp[256];
    memset(resp, 0, sizeof(resp));
    sprintf(resp, "{\"cmd\": 4, \"result\": 1}");
    ble_send_data(resp, strlen(resp));
}

char* m_http_services(char* buf, uint16_t len)
{
    http_service_t services[MAX_NUM_SERVICES] = {wificonfig, apconfig};
    char* temp = NULL;

    for (int i = 0; i < MAX_NUM_SERVICES; i++)
    {
        temp = strnstr(buf, services[i].method, len);
        if (temp == &(buf[0]) && temp[strlen(services[i].method)] == ' ')
        {
            temp = strnstr(buf, services[i].uri, len);
            if (temp != NULL)
            {
                if ((temp[strlen(services[i].uri)] == '/')) 
                {
                    if (temp[strlen(services[i].uri) +1] == ' ')
                    {
                        continue;
                    }
                }
                else if (temp[strlen(services[i].uri)] != ' ')
                {
                    continue;
                }

                if (services[i].handler != NULL)
                {
                    return services[i].handler(buf);
                }
            }
        }
    }

    return NULL;
}
