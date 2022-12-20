#include "m_http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "web_server.h"

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

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include <wifi_mgmr_ext.h>

#define SERVER_PORT 80

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
                                     "<title>HELLO Ai-WB2 module</title>\n"
                                     "</head>\n"
                                     "<body>\n"
                                     "<h1>Hello World, from Ai-WB2 module</h1>\n"
                                     "</body>\n"
                                     "</html>\n";

static void web_http_server(struct netconn *conn)
{
    struct netbuf *inputbuf;
    char *buf;
    u16_t buflen;
    err_t err;

    err = netconn_recv(conn, &inputbuf);
    if (err == ERR_OK)
    {
        netbuf_data(inputbuf, (void **)&buf, &buflen);
        printf("the received data:\n%s\n", buf);
        
        /* Judge if this is an HTTP GET command */
        if (buflen >= 5 && buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T' && buf[3] == ' ' && buf[4] == '/')
        {
            netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);

            if (buf[5] == 'h')
            {
                netconn_write(conn, http_index_hml, sizeof(http_index_hml) - 1, NETCONN_NOCOPY);
            }
            else if (buf[5] == 'l')
            {
                netconn_write(conn, http_index_hml, sizeof(http_index_hml) - 1, NETCONN_NOCOPY);
            }
            else
            {
                netconn_write(conn, http_index_hml, sizeof(http_index_hml) - 1, NETCONN_NOCOPY);
            }
        }
    }
    netconn_close(conn);
    netbuf_delete(inputbuf);
}

void http_server_start(void *pvParameters)
{
    struct netconn *conn, *newconn;
    err_t err;
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, SERVER_PORT);
    netconn_listen(conn);
    while (1)
    {
        err = netconn_accept(conn, &newconn);
        if (err == ERR_OK)
        {
            web_http_server(newconn);
            netconn_delete(newconn);
        }
        else
        {
            netconn_close(conn);
            netconn_delete(conn);
            break;
        }
    }
}

void m_http_init(void)
{
   xTaskCreate(http_server_start, (char *)"http server", 1024 * 4, NULL, 15, NULL);
}