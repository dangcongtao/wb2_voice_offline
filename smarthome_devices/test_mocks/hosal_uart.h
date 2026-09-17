#ifndef _MOCK_HOSAL_UART_H_
#define _MOCK_HOSAL_UART_H_

#include <stdint.h>
#include <stddef.h>

#define HOSAL_UART_BAUD_SET 1
#define HOSAL_UART_FLUSH 2
#define HOSAL_UART_MODE_INT 1
#define HOSAL_UART_TX_CALLBACK 1
#define HOSAL_UART_RX_CALLBACK 2
#define HOSAL_UART_TX_FIFO_REQ_CALLBACK 3
#define HOSAL_UART_RX_FIFO_REQ_CALLBACK 4

#define HOSAL_DATA_WIDTH_8BIT 8
#define HOSAL_NO_PARITY 0
#define HOSAL_STOP_BITS_1 1
#define HOSAL_FLOW_CONTROL_DISABLED 0
#define HOSAL_UART_MODE_POLL 2

typedef struct {
    struct {
        uint8_t uart_id;
        uint8_t tx_pin;
        uint8_t rx_pin;
        uint8_t cts_pin;
        uint8_t rts_pin;
        uint32_t baud_rate;
        uint8_t data_width;
        uint8_t parity;
        uint8_t stop_bits;
        uint8_t flow_control;
        int mode;
    } config;
    int (*rx_cb)(void *);
    void *p_rxarg;
} hosal_uart_dev_t;

int hosal_uart_init(hosal_uart_dev_t *dev);
int hosal_uart_init_only_tx(hosal_uart_dev_t *dev);
int hosal_uart_send(hosal_uart_dev_t *dev, const void *data, uint32_t length);
int hosal_uart_receive(hosal_uart_dev_t *dev, void *data, uint32_t length);
int hosal_uart_ioctl(hosal_uart_dev_t *dev, int ctl, void *arg);
int hosal_uart_finalize(hosal_uart_dev_t *dev);
int hosal_uart_callback_set(hosal_uart_dev_t *dev, int callback_type, int (*pfn_callback)(void *), void *arg);

#endif
