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
// #include <hal_uart.h>
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
// #include <ble_cli_cmds.h>
#include <hosal_uart.h>

#include "my_ble.h"
#include "system_typedef.h"
#include "aes_cbc128.h"
#include "m_beacon.h"
#include "hardware.h"

#define BLE_MAX_QUEUE_ITEM 10

extern char key[16], iv[16];

extern unsigned char bleuart_connect_status;
static struct bt_gatt_exchange_params exchange_params;
static QueueHandle_t ble_tx_queue;
static TaskHandle_t xHandle = NULL;

extern void push_data_to_pqueue(m_data_t *message);

static void exchange_func(struct bt_conn *conn, u8_t err, struct bt_gatt_exchange_params *params)
{
    if (conn)
    {
        printf("[BLE] Exchange %s MTU Size =%d \r\n", err == 0U ? "successful" : "failed", bt_gatt_get_mtu(conn));
    }
}

static void ble_connect_status_cb(uint8_t status, char *addr)
{
    if (status == MY_BLE_DEVICE_CONNECT)
    {
        os_trace("+BLE_CONNECT\r\n");
        bleuart_connect_status = 1;
        exchange_params.func = exchange_func;
        bt_gatt_exchange_mtu(pconn, &exchange_params);
    }
    else if (status == MY_BLE_DEVICE_DISCONNECT)
    {
        os_trace("+BLE_DISCONNECT\r\n");
        bleuart_connect_status = 0;
        my_ble_start_advertise(); // 蓝牙断开，开始广播
    }
}

static void ble_recv_data_cb(struct bt_conn *conn, const char *buf, u16_t len)
{

    if (len >= BLE_DATA_MAX_LEN)
    {
        os_trace("buf len overflow\r\n");
        return;
    }

    m_data_t message;
    memset(&message, 0, sizeof(message));
    memcpy(message.data, buf, len);
    message.length = len;
    message.method_trasmit = BLE_MSG;

    os_trace("ble receive[%d] %s\r\n", len, message.data);
    push_data_to_pqueue(&message);

    memset((void *)buf, 0, sizeof(buf));
}

static void ble_user_init(void)
{
    my_ble_init();
    my_ble_recv_register(ble_recv_data_cb);
    my_ble_connect_register(ble_connect_status_cb);
    bleuart_connect_status = 0;
}

static bool ble_init_queue(void)
{
    if (ble_tx_queue == NULL)
    {
        ble_tx_queue = xQueueCreate(BLE_MAX_QUEUE_ITEM, sizeof(m_data_t));
        if (ble_tx_queue == NULL)
        {
            os_trace("%s fail\r\n", __func__);
            while (1)
                ;
        }
    }

    os_trace("BLE create queue success\r\n");

    return true;
}

static void ble_task(void *pvParameters)
{
    char msg_aes[DATA_MAX_LEN];
    uint32_t len_aes = DATA_MAX_LEN;
    uint32_t aes_encrypt_result = 0;
    m_ble_data_t message;

    ble_user_init();

    memset(&message, 0, sizeof(message));
    memset(msg_aes, 0, sizeof(msg_aes));

    ble_master_scan();

    while (1)
    {
        if (xQueueReceive(ble_tx_queue, &message, 1) == pdPASS)
        {
            printf("[%s]:\r\n", message.data);

            /* Encode AES */
            len_aes = DATA_MAX_LEN;
            memset(msg_aes, 0, len_aes);

            aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
            aes_encrypt_result = aes_cbc128_encrypt((uint8_t *)message.data, message.length,
                                                    (uint8_t *)msg_aes, (size_t *)&len_aes);
            if (aes_encrypt_result != AES_SUCCESS)
            {
                os_trace("Encode fail,error-code: %ld\r\n", aes_encrypt_result);
                break;
            }

            /* call platform mqtt-publish api */
            my_ble_notify(pconn, msg_aes, len_aes);

            memset(&message, 0, sizeof(message));
        }

        ble_config_loop();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ble_send_data(char *data, uint32_t length)
{
    if (bleuart_connect_status == 0)
        return;

    os_trace("ble_send: %s\r\n", data);

    m_ble_data_t message;
    memset(&message, 0, sizeof(message));

    if (length > DATA_MAX_LEN)
    {
        os_trace("data ble too long to send %u\r\n", length);
        return;
    }

    memcpy(message.data, data, length);
    message.length = length;

    if (ble_tx_queue == NULL)
    {
        os_trace("ble_tx_queue NULL\r\n");
        return;
    }

    if (xQueueSend(ble_tx_queue, &message, pdMS_TO_TICKS(10)) != pdPASS)
    {
        os_trace("Push to ble_tx_queue fail\r\n");
        os_trace("\r\n");
        return;
    }

    os_trace("Push to ble_tx_queue success\r\n");
    os_trace("\r\n");
}

void ble_init(void)
{
    os_trace("MY BLE SLAVE ble_init\r\n");

    if (xHandle != NULL)
        return;

    ble_init_queue();
    xTaskCreate(ble_task, "ble slave", 1024 * 3, NULL, 15, &xHandle);
}

void ble_deinit(void)
{
    os_trace("MY BLE SLAVE ble_deinit\r\n");

    if (xHandle == NULL)
        return;

    m_ble_data_t message;
    if (ble_tx_queue != NULL) {
        while (xQueuePeek(ble_tx_queue, &message, (TickType_t)10) != pdFALSE)
        {
        }
    }

    my_ble_deinit();

    vTaskDelete(xHandle);
    xHandle = NULL;

    if (ble_tx_queue != NULL) {
        vQueueDelete(ble_tx_queue);
        ble_tx_queue = NULL;
    }

    vTaskDelay(500);
}


void ble_set_name(char* name, uint8_t restart_adv)
{
    os_trace("ble_set_name: %s\r\n", name);
    my_ble_set_name(name, restart_adv);
}