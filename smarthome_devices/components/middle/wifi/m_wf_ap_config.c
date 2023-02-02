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

    char *temp = NULL;
    char data[128];
    char *data_begin = NULL;
    char *data_end = NULL;

    uint16_t base64_len;
    uint8_t base64_data[256];
    char real_msg[256];
    size_t real_msg_len = 0;

    /* pub */
    temp = strstr(buf, "name=\"pub\"");
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
    /* base64 */
    memset(base64_data, 0, sizeof(base64_data));
    base64_len = b64_decode((uint8_t *)data, strlen(data), base64_data);
    /* AES */
    memset(real_msg, 0, sizeof(real_msg));
    aes_cbc128_init((uint8_t*)DEF_AES_KEY, (uint8_t*)DEF_AES_IV);
    if (aes_cbc128_decrypt((uint8_t *)base64_data, base64_len, (uint8_t *)real_msg, &real_msg_len) != AES_SUCCESS)
    {
        os_trace("Pub decode fail\r\n");
        return false;
    }
    /* save */
    os_trace("set_mqtt_topic_pub[%d]: %s\r\n", real_msg_len, real_msg);
    m_mqtt_get_instance().set_mqtt_topic_pub(real_msg, real_msg_len);

    /* sub */
    temp = strstr(buf, "name=\"sub\"");
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
    /* copy */
    memset(data, 0, sizeof(data));
    memcpy(data, data_begin, data_end - data_begin);
    /* base64 */
    memset(base64_data, 0, sizeof(base64_data));
    base64_len = b64_decode((uint8_t *)data, strlen(data), base64_data);
    /* AES */
    memset(real_msg, 0, sizeof(real_msg));
    aes_cbc128_init((uint8_t*)DEF_AES_KEY, (uint8_t*)DEF_AES_IV);
    if (aes_cbc128_decrypt((uint8_t *)base64_data, base64_len, (uint8_t *)real_msg, &real_msg_len) != AES_SUCCESS)
    {
        os_trace("Pub decode fail\r\n");
        return false;
    }
    /* save */
    os_trace("set_mqtt_topic_sub[%d]: %s\r\n", real_msg_len, real_msg);
    m_mqtt_get_instance().set_mqtt_topic_sub(real_msg, real_msg_len);

    /* ssid */
    temp = strstr(buf, "name=\"ssid\"");
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
    /* copy */
    memset(data, 0, sizeof(data));
    memcpy(data, data_begin, data_end - data_begin);
    m_wifi_get_instance().wifi_set_ssid_cb(data, strlen(data));
    os_trace("set wifi ssid %s\r\n", data);

    /* pass */
    temp = strstr(buf, "name=\"pass\"");
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
    /* copy */
    memset(data, 0, sizeof(data));
    memcpy(data, data_begin, data_end - data_begin);
    m_wifi_get_instance().wifi_set_passwd_cb(data, strlen(data));
    os_trace("set wifi password %s\r\n", data);

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
