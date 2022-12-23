#include "m_wifi_station.h"
#include "m_http_server.h"
#include "system_typedef.h"

#define STA_SSID "RD HUNONIC 1"
#define STA_PASSWORD "66668888"

extern int g_wifi_mode;
static int g_wifi_sta_is_connected = 0;
static wifi_interface_t g_wifi_sta_interface = NULL;
wifi_sta_reconnect_t sta_reconn_params = {15, 10}; // set connection interval = 15, reconntction times = 10

int wifi_sta_connect(char *ssid, char *pass)
{
    g_wifi_mode = WIFI_STATION;

    g_wifi_sta_interface = wifi_mgmr_sta_enable();
    if (g_wifi_sta_is_connected == 1)
    {
        os_trace("sta has connect\r\n");
        return 0;
    }
    else
    {
        wifi_mgmr_sta_autoconnect_enable();
        return wifi_mgmr_sta_connect(g_wifi_sta_interface, ssid, pass, NULL, NULL, 0, 0);
    }
}

void m_wifi_sta_event_cb(input_event_t *event, void *private_data)
{
    static char *ssid;
    static char *password;

    os_trace("[APP] [EVT] event->code %d\r\n", event->code);
    os_trace("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());

    switch (event->code)
    {

    case CODE_WIFI_ON_INIT_DONE:
        os_trace("[APP] [EVT] INIT DONE\r\n");
        // wifi_mgmr_start_background(&conf);
        // wifi_sta_connect();
        break;
    case CODE_WIFI_ON_MGMR_DONE:
        os_trace("[APP] [EVT] MGMR DONE\r\n");
        break;
    case CODE_WIFI_ON_SCAN_DONE:
        os_trace("[APP] [EVT] SCAN Done\r\n");
        // wifi_mgmr_cli_scanlist();
        break;
    case CODE_WIFI_ON_CONNECTING:
        os_trace("[APP] [EVT] Connecting\r\n");
        break;
    case CODE_WIFI_CMD_RECONNECT:
        os_trace("[APP] [EVT] Reconnect\r\n");
        break;
    case CODE_WIFI_ON_CONNECTED:
        os_trace("wifi sta connected\r\n");
        os_trace("[APP] [EVT] connected\r\n");
        break;
    case CODE_WIFI_ON_DISCONNECT:
        g_wifi_sta_is_connected = 0;
        os_trace("wifi sta disconnected\r\n");
        os_trace("[APP] [EVT] disconnect\r\n");
        break;
    case CODE_WIFI_ON_PRE_GOT_IP:
        os_trace("[APP] [EVT] connected\r\n");
        break;
    case CODE_WIFI_ON_GOT_IP:
        os_trace("WIFI STA GOT IP\r\n");
        os_trace("[APP] [EVT] GOT IP\r\n");
        g_wifi_sta_is_connected = 1;
        break;
    case CODE_WIFI_ON_PROV_SSID:
        os_trace("[APP] [EVT] [PROV] [SSID] %lld: %s\r\n",
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
        os_trace("[APP] [EVT] [PROV] [BSSID] %lld: %s\r\n",
                 aos_now_ms(),
                 event->value ? (const char *)event->value : "UNKNOWN");
        if (event->value)
        {
            vPortFree((void *)event->value);
        }
        break;
    case CODE_WIFI_ON_PROV_PASSWD:
        os_trace("[APP] [EVT] [PROV] [PASSWD] %lld: %s\r\n", aos_now_ms(),
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
    case CODE_WIFI_ON_PROV_DISCONNECT:
        os_trace("[APP] [EVT] [PROV] [DISCONNECT]\r\n");
        break;
    case CODE_WIFI_ON_AP_STARTED:
        os_trace("[APP] [EVT] AP INIT DONE\r\n");
        break;

    case CODE_WIFI_ON_AP_STOPPED:
        os_trace("[APP] [EVT] AP STOP DONE\r\n");
        break;

    default:
        os_trace("%*s %u, %lld\r\n", 100, "[APP] [EVT] Unknown code", event->code, aos_now_ms());
    }
}

void wifi_execute(void *pvParameters)
{
    // aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
    vTaskDelete(NULL);
}

void m_wifi_sta_start(void)
{
    xTaskCreate(wifi_execute, (char *)"wifi execute", 1024 * 3, NULL, 15, NULL);
}