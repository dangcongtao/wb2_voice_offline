#include "m_wf_scan.h"
#include "cJSON.h"
#include "system_typedef.h"
#include "m_database.h"
#include "m_mqtt.h"

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/tcpip.h>
#include <hal_wifi.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <blog.h>

#include <bl_os_private.h>
#include <wifi_mgmr_ext.h>

static BL_Sem_t scan_semaphore = NULL;
uint32_t counter = 0;
wifi_mgmr_ap_item_t ap_ary_p[WIFI_SCAN_MAX_AP];

static void m_cb_scan_complete(void *data, void *param)
{
    // wifi_mgmr_ap_item_t *ap_ary_ptr = (wifi_mgmr_ap_item_t *)data;
    // int status = *((int *)param);
    // os_trace("scan complete status: %d, ssid_len = %lu\r\n", status, ap_ary_ptr->ssid_len);

    if (scan_semaphore != NULL)
    {
        bl_os_sem_give(scan_semaphore);
    }
}


static void m_cb_scan_item_parse(wifi_mgmr_ap_item_t *env, uint32_t *param1, wifi_mgmr_ap_item_t *item)
{
    /*recall the env*/
    uint32_t counter = (*param1);
    uint8_t is_over_queue = 0;
    os_trace("=== m_cb_scan_item_parse: counter: %u ===, [%s]:%d\r\n", counter, item->ssid, item->rssi);

    /*scan overflow check*/
    if (counter >= WIFI_SCAN_MAX_AP)
    {
        /*XXX maybe we should warning here*/
        is_over_queue = 1;
    }

    wifi_mgmr_ap_item_t *ap_ary_ptr = (wifi_mgmr_ap_item_t *)env + counter; // get and move the ptr to the next item

    /* check if wifi ssid existed */
    wifi_mgmr_ap_item_t *temp = (wifi_mgmr_ap_item_t *)env;
    uint32_t i;
    for (i = 0; i < counter; i++)
    {
        // printf("old_ssid/new_ssid: [%s:%s]\r\n", temp[i].ssid, item->ssid);
        if (!strcmp(temp[i].ssid, item->ssid))
        {
            if (item->rssi > temp[i].rssi)
            {
                printf("old_ssid/new_ssid: [%s:%s][%d:%d]\r\n", temp[i].ssid, item->ssid, temp[i].rssi, item->rssi);
                uint32_t j;
                for (j = i; j < counter - 1; j++)
                {
                    temp[j] = temp[j + 1];
                    temp[j].channel = temp[j + 1].channel;
                    temp[j].rssi = temp[j + 1].rssi;
                    memcpy(temp[j].bssid, temp[j + 1].bssid, sizeof(temp[j].bssid));
                    memcpy(temp[j].ssid, temp[j + 1].ssid, sizeof(temp[j].ssid));
                    temp[j].ssid_tail[0] = '\0';
                    temp[j].ssid_len = strlen((char *)temp[j].ssid);
                    temp[j].auth = temp[j + 1].auth;
                }
                counter--;
                i = counter;
            }
            break;
        }
    }

    if (i < counter)
    {
        /* wifi ssid had been existed */
        // printf("Wifi SSID had been Existed: %s\r\n", item->ssid);
    }
    else
    {
        uint8_t p = 0;

        for (i = 0; i < counter; i++)
        {
            if (item->rssi > temp[i].rssi)
            {
                p = i;
                break;
            }
            else
            {
                p = i + 1;
            }
        }

        uint32_t j;
        for (j = counter; j > p; j--)
        {
            temp[j] = temp[j - 1];
            temp[j].channel = temp[j - 1].channel;
            temp[j].rssi = temp[j - 1].rssi;
            memcpy(temp[j].bssid, temp[j - 1].bssid, sizeof(temp[j].bssid));
            memcpy(temp[j].ssid, temp[j - 1].ssid, sizeof(temp[j].ssid));
            temp[j].ssid_tail[0] = '\0';
            temp[j].ssid_len = strlen((char *)temp[j].ssid);
            temp[j].auth = temp[j - 1].auth;
        }

        temp[p].channel = item->channel;
        temp[p].rssi = item->rssi;
        memcpy(temp[p].bssid, item->bssid, sizeof(temp[p].bssid));
        memcpy(temp[p].ssid, item->ssid, sizeof(temp[p].ssid)); // XXX boundary attack, copy too much data from wifi manager internal?
        temp[p].ssid_tail[0] = '\0';
        temp[p].ssid_len = strlen((char *)temp[p].ssid);
        temp[p].auth = item->auth;

        if (!is_over_queue)
        {
            counter++;
        }
    }

    /*store back counter*/
    (*param1) = counter;
}

/*
@ overwrite of platform api
    int wifi_mgmr_all_ap_scan(wifi_mgmr_ap_item_t **ap_ary, uint32_t *num)
*/
void scan_ap_around(void)
{
    memset(ap_ary_p, 0, sizeof(wifi_mgmr_ap_item_t) * WIFI_SCAN_MAX_AP);
    counter = 0;

    scan_semaphore = bl_os_sem_create(0);
    if (NULL == scan_semaphore)
    {
        return;
    }

    wifi_mgmr_scan(ap_ary_p, m_cb_scan_complete); // trigger the scan

    if (0 == bl_os_sem_take(scan_semaphore, BL_OS_WAITING_FOREVER))
    {
        os_trace("wifi scan Done\r\n");
    }

    /*Start to scan the AP*/
    wifi_mgmr_scan_ap_all(ap_ary_p, &counter, m_cb_scan_item_parse);

    bl_os_sem_delete(scan_semaphore);

    // for (uint8_t i = 0; i < counter; i++)
    // {
    //     printf("rssi %3d, SSID %s\r\n", ap_ary_p[i].rssi, ap_ary_p[i].ssid);
    // }
}
