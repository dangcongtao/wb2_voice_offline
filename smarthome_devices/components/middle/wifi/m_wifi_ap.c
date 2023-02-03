#include "m_wifi_ap.h"
#include "m_wf_ap_config.h"
#include "m_http_server.h"
#include "m_database.h"
#include "system_typedef.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/tcpip.h>
#include <wifi_mgmr_ext.h>
#include <hal_wifi.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <blog.h>

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
#include "FreeRTOS.h"
#include "portmacro.h"
#include "event_groups.h"
#include "lwip/err.h"
#include "string.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "web_server.h"

#define WIFI_RECONN_INTERVAL_SEC 15
#define WIFI_RECONN_REPEAT_TIMES 10

#define IP_SET_ADDR(a, b, c, d) (((uint32_t)((a)&0xff)) |       \
                                 ((uint32_t)((b)&0xff) << 8) |  \
                                 ((uint32_t)((c)&0xff) << 16) | \
                                 ((uint32_t)((d)&0xff) << 24))

/* if you want usb other gw ip, please change components/network/lwip_dhcpd/dhcp_server_raw.c：42   DHCPD_SERVER_IP 
for example  gw ip:"192.168.4.1" , change DHCPD_SERVER_IP to "192.168.4.1" */
#define IP_ADDR         "192.168.4.1"
#define GATEWAY_ADDR    IP_ADDR
#define NETMASK         "255.255.255.0"

extern int g_wifi_mode;
static wifi_interface_t ap_interface;

/**
 * @brief wifi_ap_ip_set
 *      Set the IP address of soft AP
 * @param ip_addr IPV4 addr
 * @param netmask netmask
 * @param gw DNS
 */
static void wifi_ap_ip_set(char* ip_addr, char* netmask, char* gw)
{
    struct netif* ap_netif = netif_find("ap1");
    int i = 0;
    int ap_ipaddr[4] = { 0 };
    int ap_netmask[4] = { 255,255,255,0 };
    int ap_gw_arry[4] = { 0,0,0,0 };

    ap_ipaddr[0] = atoi(strtok(ip_addr, "."));

    for (i = 1;i<4;i++) {
        ap_ipaddr[i] = atoi(strtok(NULL, "."));
    }
    if (netmask) {
        ap_netmask[0] = atoi(strtok(netmask, "."));
        for (i = 1;i<4;i++)
            ap_netmask[i] = atoi(strtok(NULL, "."));
    }
    if (gw) {
        ap_gw_arry[0] = atoi(strtok(gw, "."));
        for (i = 1;i<4;i++)
            ap_gw_arry[i] = atoi(strtok(NULL, "."));
    }

    if (ap_netif) {
        ip_addr_t ap_ip;
        ip_addr_t ap_mask;
        ip_addr_t ap_gw;
        IP4_ADDR(&ap_ip, ap_ipaddr[0], ap_ipaddr[1], ap_ipaddr[2], ap_ipaddr[3]);
        IP4_ADDR(&ap_mask, ap_netmask[0], ap_netmask[1], ap_netmask[2], ap_netmask[3]);
        IP4_ADDR(&ap_gw, ap_gw_arry[0], ap_gw_arry[1], ap_gw_arry[2], ap_gw_arry[3]);

        netif_set_down(ap_netif);
        netif_set_ipaddr(ap_netif, &ap_ip);
        netif_set_netmask(ap_netif, &ap_mask);
        netif_set_gw(ap_netif, &ap_gw);
        netif_set_up(ap_netif);
        os_trace("[softAP]: SSID:%s@, IP addr:%s\r\n", 
                m_db_get_instance()->_get_dev_id(),
                ip4addr_ntoa(netif_ip4_addr(ap_netif)));
    }
    else
        os_trace("no find netif ap1 ");

}

/**
 * @brief wifi_ap_start
 *
 */
static void wifi_ap_start(void)
{
    char ssid[64];
    char* passwd = "";

    memset(ssid, 0, sizeof(ssid));
    sprintf(ssid, "%s*", m_db_get_instance()->_get_dev_id());

    /* start soft0-ap */
    ap_interface = wifi_mgmr_ap_enable();
    wifi_mgmr_ap_start(ap_interface, ssid, 0, passwd, 6);

    /* some setting for soft-ap */
    wifi_mgmr_conf_max_sta(4);
    wifi_ap_ip_set(IP_ADDR, NETMASK, GATEWAY_ADDR);  
}

void m_wifi_ap_event_cb(input_event_t* event, void* private_data)
{
    switch (event->code) {
        case CODE_WIFI_ON_INIT_DONE:
            os_trace("<<<<<<<<<  init wifi done  <<<<<<<<<\r\n");
            // wifi_mgmr_start_background(&ap_conf);
            break;
        case CODE_WIFI_ON_MGMR_DONE:
            os_trace("<<<<<<<<< startting soft ap <<<<<<<<<<\r\n");
            // scan_ap_around();
            wifi_ap_start();
            break;
        case CODE_WIFI_ON_AP_STARTED:
            os_trace("<<<<<<<<< startt soft ap OK<<<<<<<<<<\r\n");
            break;
        case CODE_WIFI_ON_AP_STOPPED:
            break;
        case CODE_WIFI_ON_AP_STA_ADD:
            os_trace("<<<<<<<<< station connent ap <<<<<<<<<<\r\n");
            break;
        case CODE_WIFI_ON_AP_STA_DEL:
            os_trace("<<<<<<<<< station disconnet ap <<<<<<<<<<\r\n");

            break;
        default:
            break;

    }
}

static void proc_main_entry(void* pvParameters)
{
    os_trace("proc_main_entry\r\n");
    aos_post_event(EV_WIFI, CODE_WIFI_ON_MGMR_DONE, 0);
    vTaskDelete(NULL);
}

void m_wifi_ap_start(void)
{
    g_wifi_mode = WIFI_SOFT_AP;
    m_ap_cfg_clear_state();
    xTaskCreate(proc_main_entry, (char*)"main_entry", 1024, NULL, 15, NULL);
}

void m_wifi_ap_stop(void)
{
    os_trace("m_wifi_ap_stop\r\n");
    wifi_mgmr_ap_stop(NULL);
}
