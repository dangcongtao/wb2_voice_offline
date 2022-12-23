#include <stdio.h>
#include <string.h>

#include <bl_gpio.h>
#include <bl_uart.h>
#include "bl_sys.h"
#include "easyflash_common.h"

#include <FreeRTOS.h>
#include <task.h>
#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include <lwip/inet.h>

#include "m_button.h"
#include "m_switch.h"
#include "m_led_stt.h"
#include "m_database.h"
#include "m_http_server.h"
#include "process_data.h"

void m_stack_start(void)
{

    // if (m_mqtt_get_instance()._mqtt_handler_msg == NULL)
    // {
    //     os_trace("m_mqtt can not start, Please call function "
    //              "m_mqtt_register_msg_handler() before start stack\n");
    //     exit(0);
    // }

    // if (m_mqtt_get_instance()._mqtt_pub_startup_msg == NULL)
    // {
    //     os_trace("m_mqtt can not start, Please call function "
    //              "m_mqtt_command_startup_register_handler() before start stack\n");
    //     exit(0);
    // }

    /* start stack */
    m_mqtt_init();
    m_http_server_start();
    m_wifi_init();
}

void main(void)
{
    bl_uart_init(0, 16, 7, 255, 255, 115200); // set uart baud 115200
    easyflash_init();
    os_trace("[OS] Starting TCP/IP Stack...");
    tcpip_init(NULL, NULL);

    m_database_init();
    m_button_init();
    m_switch_init();
    m_led_stt_init();

    app_handler_msg_init();

    m_stack_start();
}
