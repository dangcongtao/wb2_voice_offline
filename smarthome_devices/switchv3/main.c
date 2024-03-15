#include <stdio.h>
#include <string.h>

#include <bl_gpio.h>
#include <bl_uart.h>
#include "bl_sys.h"
#include "easyflash_common.h"
#include "blog.h"

#include <FreeRTOS.h>
#include <task.h>
#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include <lwip/inet.h>

#include "hardware.h"
#include "m_button.h"
#include "m_switch.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_wifi.h"
#include "m_http_server.h"
#include "process_data.h"
#include "m_ble.h"
#include "event_driven.h"

#include <bl_wdt.h>

void main_loop(void)
{
    uint32_t delay_ms = 100, old = 0;

    while (true)
    {
        m_event_loop(delay_ms);

        // old += 1;
        // if (old > 20) {
        //     old = 0;
        //     os_trace("Free memory: %d\r\n", xPortGetFreeHeapSize());
        // }

        bl_wdt_feed();

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/**
 * initialize some feature:
 * + gpio
 * + database
 * + ble
 * + wifi
 * + mqtt instances
 */

void main(void)
{
    #if DEBUG_ENABLE == 0
    bl_sys_logall_disable();
    #endif
    bl_sys_init(); // if use ble ,must init
    bl_wdt_init(20000);
    // hw_switch_cfg(0);

    easyflash_init();
    tcpip_init(NULL, NULL);
    blog_set_level_log_component(BLOG_LEVEL_INFO, "hardware");

    /* have to init wifi and mqtt first. because database was built from them */
    m_wifi_init();
    m_mqtt_init();
    m_database_init();

    /* have to init gpio after init database, because gpio init need number_channel
    which initialized by database */
    m_button_init();
    m_switch_init();
    m_led_stt_init();

    process_data_init();
    // ble_init();
    // ble_init_first();
    m_event_init();
    
    /* start stack */
    xTaskCreate(main_loop,  /* Function that implements the task. */
                "main_loop", /* Text name for the task. */
                1024 * 2,          /* Stack size in words, not bytes. */
                NULL,              /* Parameter passed into the task. */
                4,                 /* Priority at which the task is created. */
                NULL);
    // main_loop();
}
