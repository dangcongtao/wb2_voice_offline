#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <lwip/err.h>
#include <http_client.h>
#include <cli.h>
#include <blog.h>
#include "errno.h"

#include "cJSON.h"
#include "system_typedef.h"
#include "aes_cbc128.h"
#include "base64.h"
#include "bl_sys.h"
#include "event_driven.h"
#include "hardware.h"
#include "m_mqtt.h"

#define WEB_SERVER "infoserver.hunonicpro.com"
#define WEB_PORT "80"
#define WEB_PATH "/infomqtt/?device_id="

#define HTTP_REQUEST_LEN 512
#define HTTP_RESP_LEN 700
#define MQTT_INFO_LEN 512

static uint8_t g_req_get_mqtt_info = 0;
static uint32_t s_time_get = 0;
extern char s_device_id[64];
extern char *gs_dev_type[];
extern uint32_t device_type;
extern char key[16], iv[16];

extern bool save_mqtt_info(char *json);
extern void signal_post(m_signal_t sig);

// static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
//                              "Host: " WEB_SERVER ":" WEB_PORT "\r\n"
//                              "User-Agent: aithinker wb2\r\n"
//                              "\r\n";

void http_get(char *url, char *output)
{
    char request[HTTP_REQUEST_LEN];
    memset(request, 0, sizeof(request));
    sprintf(request,
            "GET %s HTTP/1.0\r\n"
            "Host: %s \r\n"
            "User-Agent: hunonic wb2\r\n"
            "\r\n",
            url, WEB_SERVER);

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;

    int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);
    if (err != 0 || res == NULL)
    {
        os_trace("DNS lookup failed err=%d res=%p\r\n", err, res);
        return;
    }

    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    os_trace("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0)
    {
        os_trace("... Failed to allocate socket.\r\n");
        freeaddrinfo(res);
        return;
    }
    os_trace("... allocated socket\r\n");

    if (connect(s, res->ai_addr, res->ai_addrlen) != 0)
    {
        os_trace("... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        return;
    }
    os_trace("... connected\r\n");
    freeaddrinfo(res);

    if (write(s, request, strlen(request)) < 0)
    {
        os_trace("... socket send failed\r\n");
        close(s);
        return;
    }
    os_trace("... socket send success\r\n");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                   sizeof(receiving_timeout)) < 0)
    {
        os_trace("... failed to set socket receiving timeout\r\n");
        close(s);
        return;
    }
    os_trace("... set socket receiving timeout success\r\n");

    /* Read HTTP response */
    char recv_buf[HTTP_RESP_LEN];
    bzero(recv_buf, sizeof(recv_buf));
    do
    {
        r = read(s, recv_buf, sizeof(recv_buf) - 1);
    } while (r > 0);
    // for (int i = 0; i < strlen(recv_buf); i++) {
    // 	putchar(recv_buf[i]);
    // }

    /* get raw data */
    char *raw_data = strstr(recv_buf, "\r\n\r\n");
    if (raw_data == NULL)
    {
        os_trace("raw data not found\r\n");
        close(s);
        return;
    }
    raw_data += 4;
    if (strlen(raw_data) >= HTTP_REQUEST_LEN)
    {
        os_trace("--------> %s response is limit[%d]\r\n", __func__, strlen(raw_data));
        close(s);
        return;
    }
    memcpy(output, raw_data, strlen(raw_data));

    // printf("\n\r\n");
    close(s);
}

char temp1[HTTP_REQUEST_LEN];
char temp2[HTTP_REQUEST_LEN];
// http://api.hunonicpro.com/infomqtt/?device_id={root_id}&type={root_type}
uint8_t get_mqtt_info(void)
{
    uint8_t result = 0;

    memset(temp1, 0, sizeof(temp1));
    sprintf(temp1, "/HardwareAPI/getInfoMqtt.php?device_id=%s&type=%s&dev=%d",
            s_device_id, gs_dev_type[device_type], BUILD_TEST);

    memset(temp2, 0, sizeof(temp2));
    http_get(temp1, temp2);

#if DEBUG_ENABLE == 1
    os_trace("\r\noutput[%d][\r\n", strlen(temp2));
    for (int i = 0; i < strlen(temp2); i++)
    {
        os_trace("%c", temp2[i]);
        if (i == 300)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    os_trace("]\r\n");
    // os_trace("\r\noutput[%d][%s]\r\n", strlen(temp2), temp2);
#endif

    if (strlen(temp2) >= HTTP_REQUEST_LEN)
    {
        os_trace("%s output http get excceed length\r\n", __func__);
        return 0;
    }

    // os_trace("output \n[%s]\n", temp2);

    cJSON *js_Root;
    js_Root = cJSON_Parse(temp2);
    if (!js_Root)
    {
        return 0;
    }

    cJSON *jsGetObject = cJSON_GetObjectItem(js_Root, "data");
    if (!jsGetObject)
        goto json_parse_fail;

    memset(temp1, 0, sizeof(temp1));
    snprintf(temp1, sizeof(temp1), "%s", jsGetObject->valuestring);

    /* Decode base 64 */
    uint16_t base64_len;
    memset(temp2, 0, sizeof(temp2));
    base64_len = b64_decode((uint8_t *)temp1, strlen(temp1), (uint8_t *)temp2);

    /* Decode AES */
    size_t real_msg_len = 0;
    memset(temp1, 0, sizeof(temp1));
    aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
    if (aes_cbc128_decrypt((uint8_t *)temp2, base64_len, (uint8_t *)temp1, &real_msg_len) != AES_SUCCESS)
    {
        goto json_parse_fail;
    }

    /* {"server":"mqtt2.hunonicpro.com","port":"1883","user":"bestbug","pass":"bigbugdmm"} */
    save_mqtt_info(temp1);
    result = 1;

json_parse_fail:
    cJSON_Delete(js_Root);
    return result;
}

void request_get_mqtt_info(void)
{
    os_trace("%s\r\n", __func__);
    g_req_get_mqtt_info = 1;
}

void http_client_loop(void)
{
    if (g_req_get_mqtt_info)
    {
        if (sys_get_tick_ms() - s_time_get < 3000)
        {
            return;
        }

        g_req_get_mqtt_info = 0;
        if (get_mqtt_info() == 0)
        {
            os_trace("%s:%d: Get MQTT Info Fail \r\n", __func__, __LINE__);
            signal_post(SIG_MQTT_GET_INFO_FAIL);
            s_time_get = sys_get_tick_ms();
        }
        else
        {
            os_trace("%s:%d: sig_mqtt_got_info success \r\n", __func__, __LINE__);
            m_mqtt_get_instance()._mqtt_stop();
            signal_post(SIG_MQTT_GOT_INFO);
            s_time_get = 0;
        }
    }
}