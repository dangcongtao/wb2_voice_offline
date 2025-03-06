#ifndef _HW_UART_H_
#define _HW_UART_H_

#include <stdint.h>
#include <hosal_uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include "system_typedef.h"

#define MAX_QUEUE_UART 5
#define MAX_LEN_RX  32

typedef struct
{
    char data[MAX_LEN_RX];
    uint32_t length;
    uint8_t method_trasmit;
} m_data_uart_t;

typedef struct
{
    uint32_t rx_count;
    char rx_bytes[MAX_LEN_RX];
    uint32_t last_time_rx;
    hosal_uart_dev_t hosal_uart;
    QueueHandle_t tx_queue;
    QueueHandle_t rx_queue;

    uint32_t (*get_data)(m_data_t *msg);
    void (*reset_rx_buffer)(void);
    void (*send_bytes)(char *data, uint32_t len);
} hw_uart_t;

hw_uart_t *hw_uart_get_instance(void);
void hw_uart_init(void);
void hw_uart_loop(void);

#endif // _HW_UART_H_