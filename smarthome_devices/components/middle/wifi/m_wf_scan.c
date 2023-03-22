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
    // printf("\r\n=== m_cb_scan_item_parse: counter: %u ===\r\n", counter);

    /*scan overflow check*/
    if (counter >= WIFI_SCAN_MAX_AP)
    {
        /*XXX maybe we should warning here*/
        return;
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
            break;
        }
    }

    if (i < counter) {
        /* wifi ssid had been existed */
        // printf("Wifi SSID had been Existed: %s\r\n", item->ssid);
    }
    else {
        // printf("Scan get new wifi: %s\r\n", item->ssid);

        counter++;

        /*copy out scan data*/
        ap_ary_ptr->channel = item->channel;
        ap_ary_ptr->rssi = item->rssi;
        memcpy(ap_ary_ptr->bssid, item->bssid, sizeof(ap_ary_ptr->bssid));
        memcpy(ap_ary_ptr->ssid, item->ssid, sizeof(ap_ary_ptr->ssid)); // XXX boundary attack, copy too much data from wifi manager internal?
        ap_ary_ptr->ssid_tail[0] = '\0';
        ap_ary_ptr->ssid_len = strlen((char *)ap_ary_ptr->ssid);
        ap_ary_ptr->auth = item->auth;
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
