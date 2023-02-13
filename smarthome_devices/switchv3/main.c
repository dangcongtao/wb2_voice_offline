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

void main_loop(void)
{
    uint32_t old = 0;

    while (true)
    {
        m_event_loop();

        // old += 1;
        // if (old > 20) {
        //     old = 0;
        //     os_trace("Free memory: %d\r\n", xPortGetFreeHeapSize());
        // }

        vTaskDelay(pdMS_TO_TICKS(100));
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
    hw_switch_cfg(0);

    easyflash_init();
    tcpip_init(NULL, NULL);
    blog_set_level_log_component(BLOG_LEVEL_NEVER, "axk_mqtt");

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
    ble_init();
    m_event_init();
    
    /* start stack */
    main_loop();
}
