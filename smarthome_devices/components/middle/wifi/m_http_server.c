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

static void web_http_server(struct netconn *conn)
{
    struct netbuf *inputbuf;
    char *buf;
    char* resond;
    u16_t buflen;
    err_t err;

    err = netconn_recv(conn, &inputbuf);
    if (err == ERR_OK)
    {
        netbuf_data(inputbuf, (void **)&buf, &buflen);
        printf("the received data:\r\n%s\r\n", buf);

        /* Judge if this is an HTTP GET command */
        if (buflen > 0)
        {
            resond = m_http_services(buf, buflen);
            if (resond != NULL) 
            {
                netconn_write(conn, resond, sizeof(resond) - 1, NETCONN_NOCOPY);
            } else {
                os_trace("not found any services\r\n");
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
    if (conn == NULL) {
        os_trace("Fail to allocate netconn_new\r\n");
        return;
    }

    if (netconn_bind(conn, NULL, SERVER_PORT) != ERR_OK) {
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