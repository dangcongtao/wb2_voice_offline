#include "m_http_server.h"
#include "m_http_services.h"
#include "system_typedef.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/sockets.h"

#include <FreeRTOS.h>
#include "portmacro.h"
#include <task.h>
#include <timers.h>
#include "event_groups.h"
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <easyflash.h>

#include <hal_sys.h>
#include <bl_wifi.h>
#include <hal_wifi.h>
#include <bl_timer.h>
#include <bl_sys_time.h>
#include <bl_sys.h>

#include <wifi_mgmr_ext.h>

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <blog.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/sockets.h>
#include <lwip/tcpip.h>
#include <wifi_mgmr_ext.h>
#include <cli.h>
#include <hal_wifi.h>
#include <lwip/init.h>
#include "tcp_server.h"

#define SERVER_PORT 80

extern m_state_t current_state;

static uint8_t stop_http_server = 0;

const static char http_html_hdr[] =
    "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_hml[] = "<!DOCTYPE html>"
                                     "<html>\n"
                                     "<head>\n"
                                     "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                                     "  <style type=\"text/css\">\n"
                                     "    html, body, iframe { margin: 0; padding: 0; height: 100%; }\n"
                                     "    iframe { display: block; width: 100%; border: none; }\n"
                                     "  </style>\n"
                                     "<title>HUNONIC DEVICES</title>\n"
                                     "</head>\n"
                                     "<body>\n"
                                     "<h1>Hunonic Wifi Devices - Author: Dungnt98</h1>\n"
                                     "</body>\n"
                                     "</html>\n";

/**
 * @brief tcp_accpet_handle_cb
 *
 * @param arg
 */
static void tcp_accept_handle_cb(void *arg)
{
    tcp_client_msg_t *tcp_client_msg = (tcp_client_msg_t *)arg;
    struct sockaddr_in *socket_addr = (struct sockaddr_in *)tcp_client_msg->ip_addr;
    int ret = 0;
    char data[MAX_HTTP_REQUEST_LEN] = {0};
    char *respond;
    int timeout_retry_ms = 3000;
    do 
    {
        memset(data, 0, sizeof(data));
        ret = tcp_server_receive(tcp_client_msg->socket_id, data);
        if (ret > 0)
        {
            printf("IP: %s\r\n", inet_ntoa(socket_addr->sin_addr.s_addr));

            printf("data:[");
            for (uint32_t i = 0; i < strlen(data); i++)
                printf("%c", data[i]);
            printf("]\r\n\r\n");

            /* Parse the incoming request */
            char method[8];
            char url[128];
            sscanf(data, "%s %s", method, url);
            printf("url: %s\r\n", url);
            printf("method: %s\r\n", method);

            respond = m_http_services(data, strlen(data));
            if (respond != NULL)
            {
                printf("respond:[");
                for (uint32_t i = 0; i < strlen(respond); i++)
                    printf("%c", respond[i]);
                printf("]\r\n\r\n");

                tcp_server_send(tcp_client_msg->socket_id, http_html_hdr);
                tcp_server_send(tcp_client_msg->socket_id, respond);
                
                os_trace("send respond done\r\n");
            }
            else
            {
                os_trace("not found any services\r\n");
                tcp_server_send(tcp_client_msg->socket_id, http_html_hdr);
                tcp_server_send(tcp_client_msg->socket_id, http_index_hml);
            }
            break;
        }
        else {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    } while (timeout_retry_ms);

    tcp_server_close(tcp_client_msg->socket_id);
    os_trace("tcp_accept_handle_cb delete task\r\n");
    // vTaskDelete(NULL);
}

void tcp_server_task(void *pvParameters)
{
    int socket_fd;
    socket_fd = tcp_server_init(NULL, SERVER_PORT);
    if (socket_fd < 0) {
        os_trace("tcp server create socker fail\r\n");
        vTaskDelete(NULL);
        return;
    }
    os_trace("http_server_task\r\n");

    while (1) {
        if (stop_http_server) {
            os_trace("stop http server\r\n");
            tcp_server_deinit();
            stop_http_server = 0;
            break;
        }

        if (current_state == M_WIFI_APCONFIG) {
            if (tcp_server_listen(socket_fd) == 0) {
                tcp_server_accept(socket_fd, tcp_accept_handle_cb);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    vTaskDelete(NULL);
}

void m_http_server_start(void)
{
    os_trace("m_http_server_start\r\n");
    xTaskCreate(tcp_server_task, (char *)"http server", 1024*3, NULL, 15, NULL);
}

void m_http_stop(void)
{
    stop_http_server = 1;
}