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

char* m_apconfig_step1(char* request_buf);
char* m_apconfig_step2(char* request_buf);

extern uint32_t counter;
extern wifi_mgmr_ap_item_t ap_ary_p[WIFI_SCAN_MAX_AP];
extern void ble_send_data(char *data, uint32_t length);

http_service_t get_info = {
    .uri       = "/get_info",
    .method    = "GET",
    .handler   = m_apconfig_step1,
};

http_service_t set_info = {
    .uri       = "/set_info",
    .method    = "POST",
    .handler   = m_apconfig_step2,
};

char* m_apconfig_step1(char* buf)
{
    int n = 0;

    os_trace("\tAPCONFIG STEP 1\r\n");

    /* show wifi around me */
    memset(buf, 0, MAX_HTTP_RESPONSE_LEN);

    n = sprintf(buf, "{\"wifis\": [");
    for (uint8_t i = 0; i < counter; i++)
    {
        n += sprintf(&buf[n], "{\"id\": %d, \"rssi\": %d, \"ssid\": \"%s\"},",
                     i, ap_ary_p[i].rssi, ap_ary_p[i].ssid);
    }
    n += sprintf(&buf[n - 1], "], \"root_id\": \"%s\", \"root_type\": \"%s\", \"result\": 1}",
                 m_db_get_instance()->_get_dev_id(), gs_dev_type[device_type]);

    os_trace("APCONFIG STEP 1 DONE\r\n");

    return buf;
}

char* m_apconfig_step2(char* buf)
{
    os_trace("APCONFIG STEP 2\r\n");

    uint8_t result = 1;

    /* parser data to get info of wifi & mqtt server */
    if (apconfig_parse_wifi_info(buf) == false)
    {
        os_trace("ap_config parse data fail\r\n");
        result = 0;
    }

    /* Send response */
    memset(buf, 0, MAX_HTTP_RESPONSE_LEN);
    sprintf(buf, "{\"result\": %d}", result);

    return buf;
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
    os_trace("\tm_ble_config_step2\r\n");

    char resp[256];
    memset(resp, 0, sizeof(resp));
    sprintf(resp, "{\"cmd\": 4, \"result\": 1}");
    ble_send_data(resp, strlen(resp));
}

char* m_http_services(char* buf, uint16_t len)
{
    http_service_t services[MAX_NUM_SERVICES] = {set_info, get_info};
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
