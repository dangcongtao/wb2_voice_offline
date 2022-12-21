#include "m_http_services.h"
#include <string.h>
#include <stdint.h>

#include "app_config.h"
#include "system_typedef.h"
#include "m_database.h"
#include "m_ap_config.h"
#include "m_smartconfig.h"
#include <wifi_mgmr_ext.h>

#define MAX_NUM_SERVICES    4
#define MAX_RESP_LEN        1024

char* m_apconfig_step1(char* req);
char* m_apconfig_step2(char* req);
char* m_smartconfig_step1(char *req);
char* m_smartconfig_step2(char *req);

extern uint32_t counter;
extern wifi_mgmr_ap_item_t ap_ary_p[WIFI_SCAN_MAX_AP];

http_service_t wificonfig = {
    .uri       = "/wificonfig",
    .method    = "POST",
    .handler   = m_apconfig_step1,
};

char* m_apconfig_step1(char* req)
{
    os_trace("APCONFIG STEP 1\r\n");

    /* parser data to get info of wifi & mqtt server */
    os_trace("m_http_server receive: [%s]\r\n", req);
    apconfig_parse_wifi_info(req);

    /* Send response */
    char* resp = malloc(sizeof(char) * MAX_RESP_LEN);
    memset(resp, 0, sizeof(resp));
    sprintf(resp, "{\"result\": 1}");

    return resp;
}

http_service_t apconfig = {
    .uri       = "/apconfig",
    .method    = "GET",
    .handler   = m_apconfig_step2,
};

char* m_apconfig_step2(char *req)
{
    int n = 0;

    os_trace("\tAPCONFIG STEP 2\r\n");

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


http_service_t root = {
    .uri       = "/",
    .method    = "GET",
    .handler   = m_smartconfig_step1,
};

char* m_smartconfig_step1(char *req)
{
    os_trace("SMARTCONFIG STEP 1\r\n");

    char* resp = malloc(sizeof(char) * MAX_RESP_LEN);
    memset(resp, 0, sizeof(resp));
    sprintf(resp, "{\"id\":\"%s\",\"version\":\"%s\"}", m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION);

    return resp;
}


http_service_t setting = {
    .uri       = "/setting",
    .method    = "POST",
    .handler   = m_smartconfig_step2,
};

char* m_smartconfig_step2(char *req)
{
    // os_trace("\nSmartconfig http step 2 \n");
    char* resp = malloc(sizeof(char) * MAX_RESP_LEN);

    // os_trace("m_http_server receive: [%s]", req);
    // if (m_smartconfig_parse_data(req) != true)
    // {
    //     os_trace("Smartconfig - http parse wifi fail\n");
    // }

    // memset(resp, 0, sizeof(resp));
    // sprintf(resp, "{\"result\": 1}");

    return resp;
}

char* m_http_services(char* buf, uint16_t len)
{
    http_service_t services[MAX_NUM_SERVICES] = {wificonfig, apconfig, root, setting};
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
