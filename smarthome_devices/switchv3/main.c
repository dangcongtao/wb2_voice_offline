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
#include "m_led_stt.h"
#include "m_database.h"
#include "m_wifi.h"
#include "m_http_server.h"
#include "process_data.h"
#include "app_fan.h"
#include "m_ble.h"
#include "event_driven.h"

#include <bl_wdt.h>
#include "IRrecv_test.h"
#include "IRrecv.h"
#include "m_fan.h"
#include "hw_uart.h"
#include "m_http_client.h"

extern uint32_t val_temperature;
void main_loop(void)
{
    const uint32_t delay_ms = 10;
    // uint32_t old = 0;

    while (true)
    {
        m_event_loop(delay_ms);
        http_client_loop();

        if (hw_temp_read())
        {
            app_fan_send_temperature(val_temperature);
        }

        // old += 1;
        // if (old > 20)
        // {
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
    bl_sys_init(); // if use ble ,must init
#if DEBUG_ENABLE == 0
    bl_sys_logall_disable();
#else
    bl_sys_logall_enable();
#endif
    bl_wdt_init(120000);

    hw_74hc595_init();

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
    m_led_stt_init();

    process_data_init();
    m_event_init();

    hw_uart_init();
    m_fan_init();

#if DEBUG_ENABLE == 1
    _tolerance = kTolerance;
    IRrecv_test_run();
#endif
    m_ir_init();

    /* start stack */

    xTaskCreate(main_loop,   /* Function that implements the task. */
                "main_loop", /* Text name for the task. */
                1024 * 3,    /* Stack size in words, not bytes. */
                NULL,        /* Parameter passed into the task. */
                10,          /* Priority at which the task is created. */
                NULL);
    vTaskDelete(NULL);
}
