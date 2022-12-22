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

#define SERVER_PORT 80
#define MAX_LEN_HTTP_REQUEST 1024

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

static void web_http_server(struct netconn *conn)
{
    err_t err;
    struct netbuf *inputbuf;
    char *buf;
    char *resond;
    u16_t buflen;

    struct netbuf *inputbuf2;
    char *buf2;
    u16_t buflen2;

    char request[MAX_LEN_HTTP_REQUEST];
    uint32_t request_len = 0;

    err = netconn_recv(conn, &inputbuf);
    if (err == ERR_OK)
    {
        if (netbuf_data(inputbuf, (void **)&buf, &buflen) != ERR_OK)
            return;

        memset(request, 0, sizeof(request));
        memcpy(request, buf, buflen);
        request_len = buflen;

        /* HTTP POST was divided into 2 requests, need read here to get body content */
        if (strstr(buf, "POST"))
        {
            err = netconn_recv(conn, &inputbuf2);
            if (err == ERR_OK)
            {
                err = netbuf_data(inputbuf2, (void **)&buf2, &buflen2);
                if (err != ERR_OK)
                {
                    os_trace("read http post body content fail\r\n");
                    netbuf_delete(inputbuf2);
                    goto close_connection;
                }

                if (buflen2 + request_len < MAX_LEN_HTTP_REQUEST)
                {
                    memcpy(&request[request_len], buf2, buflen2);
                    request_len += buflen2;
                }
                else
                {
                    os_trace("err: request len too long\r\n");
                    netbuf_delete(inputbuf2);
                    goto close_connection;
                }
            }
            else
            {
                os_trace("netconn_recv inputbuf2 fail\r\n");
                netbuf_delete(inputbuf2);
                goto close_connection;
            }

            netbuf_delete(inputbuf2);
            // os_trace("request [%ld]:\r\n[%s]\r\n", request_len, request);
        }
        else
        {
            // os_trace("request [%ld]:\r\n[%s]\r\n", request_len, request);
        }

        /* Judge if this is an HTTP GET command */
        if (buflen > 0)
        {
            resond = m_http_services(request, request_len);
            if (resond != NULL)
            {
                // os_trace("%s\r\n", resond);
                netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                netconn_write(conn, resond, strlen(resond), NETCONN_COPY);
                free(resond);
                resond = NULL;
            }
            else
            {
                os_trace("not found any services\r\n");
                netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                netconn_write(conn, http_index_hml, sizeof(http_index_hml) - 1, NETCONN_NOCOPY);
            }
        }
    }

close_connection:
    netconn_close(conn);
    netbuf_delete(inputbuf);
}

void http_server_start(void *pvParameters)
{
    struct netconn *conn, *newconn;
    err_t err;
    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL)
    {
        os_trace("Fail to allocate netconn_new\r\n");
        return;
    }

    if (netconn_bind(conn, NULL, SERVER_PORT) != ERR_OK)
    {
        os_trace("bind server port fail\r\n");
    }

    netconn_listen(conn);

    os_trace("http_server_start\r\n");

    while (1)
    {
        os_trace("test1\r\n");
        err = netconn_accept(conn, &newconn);
        os_trace("test2\r\n");
        if (err == ERR_OK)
        {
            os_trace("get new connection\r\n");
            web_http_server(newconn);
            netconn_delete(newconn);
        }
        else
        {
            netconn_close(conn);
            netconn_delete(conn);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void m_http_server_start(void)
{
    os_trace("m_http_init\r\n");
    xTaskCreate(http_server_start, (char *)"http server", 1024 * 4, NULL, 15, NULL);
}