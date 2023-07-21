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

#define WEB_SERVER "api.hunonicpro.com"
#define WEB_PORT "80"
#define WEB_PATH "/infomqtt/?device_id="

#define HTTP_REQUEST_LEN    256
#define HTTP_RESP_LEN       512
#define MQTT_INFO_LEN       256

extern char s_device_id[64];
extern char *gs_dev_type[];
extern uint32_t device_type;

extern bool save_mqtt_info(char *json);

// static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
//                              "Host: " WEB_SERVER ":" WEB_PORT "\r\n"
//                              "User-Agent: aithinker wb2\r\n"
//                              "\r\n";

void http_get(char *url, char* output)
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
    if (err != 0 || res == NULL) {
        os_trace("DNS lookup failed err=%d res=%p\r\n", err, res);
        return;
    }

    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    os_trace("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0) {
        os_trace("... Failed to allocate socket.\r\n");
        freeaddrinfo(res);
        return;
    }
    os_trace("... allocated socket\r\n");

    if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        os_trace("... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        return;
    }
    os_trace("... connected\r\n");
    freeaddrinfo(res);

    if (write(s, request, strlen(request)) < 0) {
        os_trace("... socket send failed\r\n");
        close(s);
        return;
    }
    os_trace("... socket send success\r\n");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                   sizeof(receiving_timeout)) < 0) {
        os_trace("... failed to set socket receiving timeout\r\n");
        close(s);
        return;
    }
    os_trace("... set socket receiving timeout success\r\n");

    /* Read HTTP response */
	char recv_buf[HTTP_RESP_LEN];
	bzero(recv_buf, sizeof(recv_buf));
    do {
        r = read(s, recv_buf, sizeof(recv_buf) - 1);
    } while (r > 0);
	for (int i = 0; i < strlen(recv_buf); i++) {
		putchar(recv_buf[i]);
	}

	/* get raw data */
	char *raw_data = strstr(recv_buf, "\r\n\r\n");
	if (raw_data == NULL) {
		os_trace("raw data not found\r\n");
		return;
	}
	raw_data += 4;
	memcpy(output, raw_data, strlen(raw_data));

	printf("\n\r\n");
    close(s);
}

// http://api.hunonicpro.com/infomqtt/?device_id={root_id}&type={root_type}
void get_mqtt_info(void)
{
    char url[HTTP_REQUEST_LEN];
    memset(url, 0, sizeof(url));
    sprintf(url, "http://api.hunonicpro.com/infomqtt/?device_id=%s&type=%s", 
            s_device_id, gs_dev_type[device_type]);

    char output[MQTT_INFO_LEN];
    memset(output, 0, sizeof(output));
    http_get(url, output);
    printf("output \n[%s]\n", output);

    cJSON *js_Root;
    js_Root = cJSON_Parse(output);
    if (!js_Root) {
        bl_sys_reset_por();
        return false;
    }

    cJSON *jsGetObject = cJSON_GetObjectItem(js_Root, "data");
    if (!jsGetObject)
        goto json_parse_fail;
    
    char mqtt_info[MQTT_INFO_LEN];
    memset(mqtt_info, 0, sizeof(mqtt_info));
    snprintf(mqtt_info, sizeof(mqtt_info), "%s", jsGetObject->valuestring);

    /* Decode base 64 */
    uint16_t base64_len;
    uint8_t base64_data[MQTT_INFO_LEN];
    memset(base64_data, 0, sizeof(base64_data));
    base64_len = b64_decode((uint8_t *)mqtt_info, strlen(mqtt_info), base64_data);

    /* Decode AES */
    char real_msg[MQTT_INFO_LEN];
    size_t real_msg_len = 0;
    memset(real_msg, 0, sizeof(real_msg));
    aes_cbc128_init((uint8_t*)DEF_AES_KEY, (uint8_t*)DEF_AES_IV);
    if (aes_cbc128_decrypt((uint8_t *)base64_data, base64_len, (uint8_t *)real_msg, &real_msg_len) != AES_SUCCESS) {
        goto json_parse_fail;
    }

    /* {"server":"mqtt2.hunonicpro.com","port":"1883","user":"bestbug","pass":"bigbugdmm"} */
    save_mqtt_info(real_msg);

json_parse_fail:
    cJSON_Delete(js_Root);
}