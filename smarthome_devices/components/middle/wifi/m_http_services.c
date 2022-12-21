#include "m_http_services.h"
#include <string.h>
#include <stdint.h>

#include "app_config.h"
#include "system_typedef.h"
#include "m_database.h"
#include "m_ap_config.h"
#include "m_smartconfig.h"

#define MAX_NUM_SERVICES 4

char* m_apconfig_step1(char* req);
char* m_apconfig_step2(char* req);
char* m_smartconfig_step1(char *req);
char* m_smartconfig_step2(char *req);

char resp[1024];

http_service_t wificonfig = {
    .uri       = "/wificonfig",
    .method    = "POST",
    .handler   = m_apconfig_step1,
};

char* m_apconfig_step1(char* req)
{
    os_trace("\tApconfig step 1\n");

    /* parser data to get info of wifi & mqtt server */
    os_trace("m_http_server receive: [%s]", req);
    apconfig_parse_wifi_info(req);

    /* Send response */
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

    os_trace("\tApconfig step 2\n");

    /* show wifi around me */
    // memset(resp, 0, sizeof(resp));
    // n = sprintf(resp, "{\"APs\": [");
    // for (uint8_t i = 0; i < MIN(access_point_around.num_ap_available, MAX_SSID_SCAN); i++)
    // {
    //     n += sprintf(&resp[n], "{\"id\": %d, \"rssi\": %d, \"ssid\": \"%s\"},",
    //                  i, access_point_around.rssi[i], access_point_around.ssid[i]);
    // }
    // n += sprintf(&resp[n - 1], "], \"device_id\": \"%s\", \"extra\": %d, \"type\": \"%s\"}",
    //              m_db_get_instance()->_get_dev_id(), EXTRAS, gs_dev_type[device_type]);
    // os_trace("%s\n", resp);

    return resp;
}


http_service_t root = {
    .uri       = "/",
    .method    = "GET",
    .handler   = m_smartconfig_step1,
};

char* m_smartconfig_step1(char *req)
{
    os_trace("\nSmartconfig http step 1 \n");

    memset(resp, 0, sizeof(resp));
    sprintf(resp, "{\"id\":\"%s\",\"version\":\"%s\"}", m_db_get_instance()->_get_dev_id(), FIRMWARE_VERSION);
    os_trace("%s\n", resp);

    return resp;
}


http_service_t setting = {
    .uri       = "/setting/",
    .method    = "POST",
    .handler   = m_smartconfig_step2,
};

char* m_smartconfig_step2(char *req)
{
    // os_trace("\nSmartconfig http step 2 \n");

    // os_trace("m_http_server receive: [%s]", req);
    // if (m_smartconfig_parse_data(req) != true)
    // {
    //     os_trace("Smartconfig - http parse wifi fail\n");
    // }

    // memset(resp, 0, sizeof(resp));
    // sprintf(resp, "{\"result\": 1}");

    return resp;
}

char* m_http_services(char* buf)
{
    http_service_t services[MAX_NUM_SERVICES] = {wificonfig, apconfig, root, setting};
    int i = 0;

    for ( i = 0; i < MAX_NUM_SERVICES; i++)
    {
        if (strstr(buf, services[i].method) != NULL)
        {
            if (services[i].handler != NULL)
            {
                return services[i].handler(buf);
            }
        }
    }

    return NULL;
}
