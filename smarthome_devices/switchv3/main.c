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
#include "IRrecv.h"
#include "m_fan.h"
#include "hw_uart.h"
#include "aiio_ci13162_uart.h"
#include "m_http_client.h"
#include "m_ci13162_ota_boot.h"
#include "system_typedef.h"

extern uint32_t val_temperature;
extern uint8_t is_ota;
extern uint8_t bat_percent;
extern uint8_t bat_charging_status;

void main_loop(void)
{
    const uint32_t delay_ms = 10;
    // uint32_t old = 0;

    while (true)
    {
        m_event_loop(delay_ms);
        http_client_loop();

        if (!is_ota)
        {
            if (hw_temp_read())
            {
                app_fan_send_temperature(val_temperature);
            }
            else if (hw_bat_read())
            {
                app_fan_send_battery(bat_percent, bat_charging_status);
            }
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
    int32_t ci13162_uart_result;
    m_wifi_info_t wifi_info;

    aiio_ci13162_uart_hold_tx_idle();
    bl_sys_init(); // if use ble ,must init
#if DEBUG_ENABLE == 0
    bl_sys_logall_disable();
#else
    bl_sys_logall_enable();
#endif

    if (bl_wdt_init(AIIO_WDT_TIMEOUT_MS) != 0)
    {
        printf("Watchdog initialization failed\r\n");
    }
    ci13162_uart_result = aiio_ci13162_uart_init();
    if (ci13162_uart_result != 0)
    {
        printf("CI13162 UART initialization failed\r\n");
    }

    easyflash_init();
    m_db_read_device_id();
    if (hw_device_support_voice())
    {
        aiio_ci13162_power_init();
    }
    if (m_ci13162_ota_boot_prepare(&wifi_info))
    {
        m_wifi_t wifi;
        uint32_t led_elapsed_ms = 500U;

        if (ci13162_uart_result != 0)
        {
            bl_wdt_feed();
            vTaskDelay(pdMS_TO_TICKS(100U));
            bl_sys_reset_por();
            vTaskDelete(NULL);
            return;
        }

        hw_led_stt_cfg();

        tcpip_init(NULL, NULL);
        m_wifi_init();
        wifi = m_wifi_get_instance();
        wifi.wifi_sta_start_cb(wifi_info._ssid, wifi_info._passwd);

        os_trace("[CI13162][BOOT] Product features disabled, free heap: %u bytes\r\n",
                 (unsigned int)xPortGetFreeHeapSize());
        while (true)
        {
            uint32_t led_period_ms;

            bl_wdt_feed();
            /* Match button_scan_task: 500 ms while updating, 100 ms after success. */
            led_period_ms = (is_ota == STATE_OTA_SUCCESS) ? 100U : 500U;
            if (led_elapsed_ms >= led_period_ms)
            {
                led_elapsed_ms = 0U;
                hw_toggle_led_stt();
            }
            led_elapsed_ms += 10U;
            if (m_ci13162_ota_boot_poll())
            {
                vTaskDelay(pdMS_TO_TICKS(100U));
                bl_sys_reset_por();
                vTaskDelete(NULL);
                return;
            }
            vTaskDelay(pdMS_TO_TICKS(10U));
        }
    }

    hw_74hc595_init();
    tcpip_init(NULL, NULL);
    blog_set_level_log_component(BLOG_LEVEL_INFO, "hardware");

    /* have to init wifi and mqtt first. because database was built from them */
    m_wifi_init();
    m_mqtt_init();
    m_database_init();

    /* have to init gpio after init database, because gpio init need number_channel
    which initialized by database */
    hw_uart_init();
    m_button_init();
    m_led_stt_init();

    process_data_init();
    m_event_init();

    m_fan_init();

#if DEBUG_ENABLE == 1
    _tolerance = kTolerance;
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
