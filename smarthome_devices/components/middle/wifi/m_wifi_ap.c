#include "m_wifi_ap.h"
#include "m_ap_config.h"
#include "m_http_server.h"
#include "m_database.h"

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

/* if you want usb other gw ip, please change components/network/lwip_dhcpd/dhcp_server_raw.c：42   DHCPD_SERVER_IP 
for example  gw ip:"192.168.4.1" , change DHCPD_SERVER_IP to "192.168.4.1" */
#define IP_ADDR         "192.168.4.1"
#define GATEWAY_ADDR    IP_ADDR
#define NETMASK         "255.255.255.0"

static wifi_conf_t ap_conf = {
    .country_code = "VN",
};

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
        os_trace("[softAP]:SSID:%s@, IP addr:%s", m_db_get_instance()->_get_dev_id(), ip4addr_ntoa(netif_ip4_addr(ap_netif)));
    }
    else
        os_trace("no find netif ap1 ");

}

/**
 * @brief wifi_ap_start
 *
 */
static void wifi_ap_start()
{
    char ssid[64];
    char* passwd = "";

    memset(ssid, 0, sizeof(ssid));
    sprintf(ssid, "%s@", m_db_get_instance()->_get_dev_id());

    ap_interface = wifi_mgmr_ap_enable();
    wifi_mgmr_conf_max_sta(4);
    wifi_mgmr_ap_start(ap_interface, ssid, 0, passwd, 6);
    wifi_ap_ip_set(IP_ADDR, NETMASK, GATEWAY_ADDR);  
}

static void event_cb_wifi_event(input_event_t* event, void* private_data)
{
    switch (event->code) {
        case CODE_WIFI_ON_INIT_DONE:
            os_trace("<<<<<<<<<  init wifi done  <<<<<<<<<<");
            wifi_mgmr_start_background(&ap_conf);
            break;
        case CODE_WIFI_ON_MGMR_DONE:
            os_trace("<<<<<<<<< startting soft ap <<<<<<<<<<<");
            wifi_ap_start();
            break;
        case CODE_WIFI_ON_AP_STARTED:
            os_trace("<<<<<<<<< startt soft ap OK<<<<<<<<<<<");
            m_http_server_start();
            scan_ap_around();
            break;
        case CODE_WIFI_ON_AP_STOPPED:
            break;
        case CODE_WIFI_ON_AP_STA_ADD:
            os_trace("<<<<<<<<< station connent ap <<<<<<<<<<<");
            break;
        case CODE_WIFI_ON_AP_STA_DEL:
            os_trace("<<<<<<<<< station disconnet ap <<<<<<<<<<<");

            break;
        default:
            break;

    }
}

static void proc_main_entry(void* pvParameters)
{
    aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);
    hal_wifi_start_firmware_task();
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
    vTaskDelete(NULL);
}

void m_wifi_ap_start(void)
{
    // puts("[OS] Starting TCP/IP Stack...");
    // tcpip_init(NULL, NULL);
    ap_cfg_clear_state();
    xTaskCreate(proc_main_entry, (char*)"main_entry", 1024, NULL, 15, NULL);
}
