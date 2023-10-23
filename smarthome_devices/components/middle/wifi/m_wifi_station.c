#include "m_wifi_station.h"
#include "m_wifi.h"

#include "m_http_server.h"
#include "system_typedef.h"
#include "mqtt_client.h"

#include <lwip/tcpip.h>
#include <hal_wifi.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <wifi_mgmr_ext.h>
#include "m_led_stt.h"
#include "event_driven.h"

extern m_wifi_t g_wifi_obj;
extern int g_wifi_mode;
static int g_wifi_sta_is_connected = 0;
static wifi_interface_t g_wifi_sta_interface = NULL;

extern void signal_post(m_signal_t sig);

int is_wifi_connected(void)
{
    return g_wifi_sta_is_connected;
}

void m_wifi_sta_event_cb(input_event_t *event, void *private_data)
{
    static char *ssid;
    static char *password;
    char my_ip[16];
    ip4_addr_t ip, gw, mask, dns1, dns2;

    os_trace("wifi sta event->code %d\r\n", event->code);
    os_trace("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());

    switch (event->code)
    {
    case CODE_WIFI_ON_CONNECTING:
        os_trace("wifi sta Connecting\r\n");
        break;

    case CODE_WIFI_CMD_RECONNECT:
        os_trace("wifi sta Reconnect\r\n");
        break;

    case CODE_WIFI_ON_CONNECTED:
        os_trace("wifi sta connected\r\n");
        break;

    case CODE_WIFI_ON_DISCONNECT:
        g_wifi_sta_is_connected = 0;
        os_trace("wifi sta disconnected\r\n");
        // g_wifi_obj.wifi_set_state_cb(M_WIFI_DISCONNECT);
        // if (!dev_is_doing_config()) {
        //     m_led_stt_get_instance()._set_led_stt_wifi_not_connect_cb();
        // }
        signal_post(SIG_WIFI_DISCONNECT);
        break;

    case CODE_WIFI_ON_GOT_IP:
        wifi_mgmr_sta_ip_get(&ip.addr, &gw.addr, &mask.addr);
        memset(my_ip, 0, sizeof(my_ip));
        sprintf(my_ip, "%s", ip4addr_ntoa(&ip.addr));
        g_wifi_obj.wifi_save_ip(my_ip, strlen(my_ip));
        os_trace("GOT IP  :   %s \r\n", my_ip);

        g_wifi_sta_is_connected = 1;
        
        // Start the connection of MQTT
        // g_wifi_obj.wifi_set_state_cb(M_WIFI_CONNECTED);
        signal_post(SIG_WIFI_CONNECTED);

        break;

    case CODE_WIFI_ON_PRE_GOT_IP:
        os_trace("wifi sta connected\r\n");
        break;

    case CODE_WIFI_ON_PROV_SSID:
        os_trace("wifi sta [PROV] [SSID] %lld: %s\r\n",
                 aos_now_ms(),
                 event->value ? (const char *)event->value : "UNKNOWN");
        if (ssid)
        {
            vPortFree(ssid);
            ssid = NULL;
        }
        ssid = (char *)event->value;
        break;

    case CODE_WIFI_ON_PROV_BSSID:
        os_trace("wifi sta [PROV] [BSSID] %lld: %s\r\n",
                 aos_now_ms(),
                 event->value ? (const char *)event->value : "UNKNOWN");
        if (event->value)
        {
            vPortFree((void *)event->value);
        }
        break;

    case CODE_WIFI_ON_PROV_PASSWD:
        os_trace("wifi sta [PROV] [PASSWD] %lld: %s\r\n", aos_now_ms(),
                 event->value ? (const char *)event->value : "UNKNOWN");
        if (password)
        {
            vPortFree(password);
            password = NULL;
        }
        password = (char *)event->value;
        break;

    case CODE_WIFI_ON_PROV_CONNECT:
        os_trace("connecting to %s:%s...\r\n", ssid, password);
        break;

    case CODE_WIFI_ON_INIT_DONE:
        os_trace("wifi sta INIT DONE\r\n");
        break;

    case CODE_WIFI_ON_MGMR_DONE:
        os_trace("wifi sta MGMR DONE\r\n");
        break;

    case CODE_WIFI_ON_SCAN_DONE:
        os_trace("wifi sta SCAN Done\r\n");
        break;

    case CODE_WIFI_ON_PROV_DISCONNECT:
        os_trace("wifi sta [PROV] [DISCONNECT]\r\n");
        break;

    case CODE_WIFI_ON_AP_STARTED:
        os_trace("wifi sta AP INIT DONE\r\n");
        break;

    case CODE_WIFI_ON_AP_STOPPED:
        os_trace("wifi sta AP STOP DONE\r\n");
        break;

    default:
        os_trace("%*s %u, %lld\r\n", 100, "wifi sta Unknown code", event->code, aos_now_ms());
    }
}

static int wifi_sta_connect(char *ssid, char *pass)
{
    g_wifi_mode = WIFI_STATION;

    g_wifi_sta_interface = wifi_mgmr_sta_enable();
    if (g_wifi_sta_is_connected == 1)
    {
        os_trace("sta has been connected\r\n");
        wifi_mgmr_sta_disconnect();
        wifi_mgmr_sta_autoconnect_set(WIFI_RECONN_INTERVAL_SEC, WIFI_RECONN_REPEAT_TIMES);
        wifi_mgmr_sta_autoconnect_enable();
        return wifi_mgmr_sta_connect(g_wifi_sta_interface, ssid, pass, NULL, NULL, 0, 0);
    }
    else
    {
        wifi_mgmr_sta_autoconnect_set(WIFI_RECONN_INTERVAL_SEC, WIFI_RECONN_REPEAT_TIMES);
        wifi_mgmr_sta_autoconnect_enable();
        return wifi_mgmr_sta_connect(g_wifi_sta_interface, ssid, pass, NULL, NULL, 0, 0);
    }
}

void m_wifi_sta_start(char *ssid, char *pass)
{
    os_trace("m_wifi_sta_start\r\n");
    wifi_sta_connect(ssid, pass);
}

void m_wifi_sta_stop(void)
{
    os_trace("m_wifi_sta_stop\r\n");
    wifi_mgmr_sta_autoconnect_disable();
    wifi_mgmr_sta_disconnect();
    while(g_wifi_sta_is_connected)
        vTaskDelay(100);

    vTaskDelay(1000);
    wifi_mgmr_sta_disable(NULL);
    vTaskDelay(1000);
    os_trace("wifi_mgmr_sta_disable\r\n");
}