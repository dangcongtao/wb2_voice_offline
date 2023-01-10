#include "m_ble.h"

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vfs.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <cli.h>

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <lwip/err.h>
#include <netutils/netutils.h>

#include <bl602_glb.h>
#include <bl602_hbn.h>

#include <bl_uart.h>
#include <bl_chip.h>
#include <bl_sec.h>
#include <bl_cks.h>
#include <bl_irq.h>
#include <bl_dma.h>
#include <bl_adc.h>
#include <bl_timer.h>
#include <bl_gpio_cli.h>
#include <bl_wdt_cli.h>
//#include <hal_uart.h>
#include <hal_sys.h>
#include <hal_gpio.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <looprt.h>
#include <loopset.h>
#include <bl_sys_time.h>
#include <bl_sys_ota.h>
#include <bl_romfs.h>
#include <fdt.h>
#include <bl_sys.h>
#include <bl_timer.h>
#include <easyflash.h>
#include <bl60x_fw_api.h>
#include <utils_log.h>
#include <libfdt.h>
#include <blog.h>
//#include <ble_cli_cmds.h>
#include <hosal_uart.h>
#include "my_ble.h"

#include "system_typedef.h"

static struct bt_gatt_exchange_params exchange_params;
extern unsigned char bleuart_connect_status;

static void exchange_func(struct bt_conn *conn, u8_t err, struct bt_gatt_exchange_params *params)
{
    if (conn) {
        printf("[BLE] Exchange %s MTU Size =%d \r\n", err == 0U ? "successful" : "failed",bt_gatt_get_mtu(conn));
    }
}

static void ble_connect_status_cb(uint8_t status, char *addr)
{                    
    if(status == MY_BLE_DEVICE_CONNECT)
    {
        os_trace("+BLE_CONNECT\r\n");
        bleuart_connect_status = 1;
        exchange_params.func = exchange_func;
        bt_gatt_exchange_mtu(pconn, &exchange_params);
    }
    else if(status == MY_BLE_DEVICE_DISCONNECT)
    {                           
        os_trace("+BLE_DISCONNECT\r\n");
        bleuart_connect_status = 0;
        my_ble_start_advertise();  //蓝牙断开，开始广播
    }
}

static void ble_recv_data_cb(struct bt_conn *conn, const char *buf, u16_t len)
{    
    os_trace("ble receive[%d]: %s\r\n", len, buf);
    memset(buf, 0, sizeof(buf));
}

static void ble_user_init(void)
{
    my_ble_init();
    my_ble_recv_register(ble_recv_data_cb); 
    my_ble_connect_register(ble_connect_status_cb);
    bleuart_connect_status = 0;
}

static void ble_task(void *pvParameters)
{
    int ret;
    bl_sys_init();//if use ble ,must init
    ble_user_init();

    while (1)
    {
        // /* Uart send poll */
        // if(bleuart_connect_status == 1)
        //     my_ble_notify(pconn, data, ret); 
        vTaskDelay(100);
    }
    vTaskDelete(NULL);
}

void ble_init(void)
{
    os_trace("MY BLE SLAVE\r\n");
    xTaskCreate(ble_task, "ble slave", 1024, NULL, 15, NULL);
}


