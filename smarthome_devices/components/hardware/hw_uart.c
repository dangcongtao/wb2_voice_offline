#include "hw_uart.h"
#include "hardware.h"
#include "m_ft62f28.h"

static hw_uart_t this;

static int __uart_rx_irq(void *p_arg)
{
    uint8_t tmp_buf[MAX_LEN_RX];
    int len = 0;

    memset(tmp_buf, 0, sizeof(tmp_buf));
    len = hosal_uart_receive(&this.hosal_uart, tmp_buf, MAX_LEN_RX - 1);
    if (len > 0)
    {
        if (this.rx_count + len <= MAX_LEN_RX - 1)
        {
            memcpy(&this.rx_bytes[this.rx_count], tmp_buf, len);
            this.rx_count += len;
            this.last_time_rx = sys_get_tick_ms();
        }
    }
    return 0;
}

static void send_bytes(char *data, uint32_t len)
{
    m_data_uart_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.length = len;
    memcpy(msg.data, data, len);

    if (this.tx_queue != NULL)
    {
        xQueueSend(this.tx_queue, &msg, pdMS_TO_TICKS(5));
    }
}

static uint32_t get_data(m_data_uart_t *msg)
{
    if (msg == NULL)
    {
        return 0;
    }

    if (this.rx_queue != NULL)
    {
        memset(msg, 0, sizeof(m_data_uart_t));
        if (xQueueReceive(this.rx_queue, msg, pdMS_TO_TICKS(5)) != pdPASS)
        {
            return 0;
        }
    }

    return msg->length;
}

static void reset_rx_buffer(void)
{
    memset(this.rx_bytes, 0, sizeof(this.rx_bytes));
    this.rx_count = 0;

    xQueueReset(this.rx_queue);
}

void hw_uart_loop(void)
{
    m_data_uart_t msg;
    if (this.last_time_rx && (sys_get_tick_ms() - this.last_time_rx) > 10)
    {
        this.last_time_rx = 0;

        if (this.rx_count > 0)
        {
                os_trace("uart rx: %lu bytes\r\n", this.rx_count);
                for (size_t i = 0; i < this.rx_count; i++)
                {
                    os_trace("%02X-", this.rx_bytes[i]);
                }
                os_trace("\r\n");
    
            memset(&msg, 0, sizeof(msg));
            msg.length = this.rx_count;
            memcpy(msg.data, this.rx_bytes, this.rx_count);
            m_ft62f28_receive_data((uint8_t *)msg.data, msg.length);

            this.reset_rx_buffer();

            // if (this.rx_queue != NULL)
            // {
            //     xQueueSend(this.rx_queue, &msg, pdMS_TO_TICKS(5));
            // }
        }
    }

    /* tx */
    if (this.tx_queue != NULL)
    {
        memset(&msg, 0, sizeof(m_data_uart_t));
        if (xQueueReceive(this.tx_queue, &msg, pdMS_TO_TICKS(0)) == pdPASS)
        {
            hosal_uart_send(&this.hosal_uart, msg.data, msg.length);
        }
    }

    m_ft62f28_check_send_change();
}

static void hw_uart_task(void *param)
{
    os_trace("%s running\r\n", __func__);

    m_data_uart_t msg;

    while (1)
    {
        /* rx */
        if (((sys_get_tick_ms() - this.last_time_rx) > 100 && this.last_time_rx > 0) || this.rx_count > MAX_LEN_RX)
        {
            this.last_time_rx = 0;

            if (this.rx_count > 0)
            {
                os_trace("uart rx: %lu bytes\r\n", this.rx_count);
                memset(&msg, 0, sizeof(msg));
                msg.length = this.rx_count;
                memcpy(msg.data, this.rx_bytes, this.rx_count);
                this.reset_rx_buffer();
                if (this.rx_queue != NULL)
                {
                    xQueueSend(this.rx_queue, &msg, pdMS_TO_TICKS(5));
                }
            }
        }

        /* tx */
        if (this.tx_queue != NULL)
        {
            memset(&msg, 0, sizeof(m_data_uart_t));
            if (xQueueReceive(this.tx_queue, &msg, pdMS_TO_TICKS(5)) == pdPASS)
            {
                os_trace("%s tx_queue\r\n", __func__);
                hosal_uart_send(&this.hosal_uart, msg.data, msg.length);
            }
        }

        m_ft62f28_check_send_change();

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void hw_uart_init(void)
{
    memset(&this, 0, sizeof(this));

    this.send_bytes = send_bytes;
    this.get_data = get_data;
    this.reset_rx_buffer = reset_rx_buffer;

    this.rx_queue = xQueueCreate(MAX_QUEUE_UART, sizeof(m_data_uart_t));
    if (this.rx_queue == NULL)
    {
        os_trace("uart create rx_queue fail\r\n");
        exit(-1);
    }

    this.tx_queue = xQueueCreate(MAX_QUEUE_UART, sizeof(m_data_uart_t));
    if (this.tx_queue == NULL)
    {
        os_trace("uart create tx_queue fail\r\n");
        exit(-1);
    }

    hosal_uart_config_t uart_config = {
        .uart_id = 1,
        .tx_pin = 4, // TXD GPIO
        .rx_pin = 3, // RXD GPIO
        .cts_pin = 255,
        .rts_pin = 255,
        .baud_rate = 9600,
        .data_width = HOSAL_DATA_WIDTH_8BIT,
        .parity = HOSAL_NO_PARITY,
        .stop_bits = HOSAL_STOP_BITS_1,
        .mode = HOSAL_UART_MODE_INT,
    };
    this.hosal_uart.config = uart_config;
    this.hosal_uart.rx_cb = __uart_rx_irq;
    this.hosal_uart.p_rxarg = NULL;
    hosal_uart_init(&this.hosal_uart);

    // xTaskCreate(hw_uart_task, "hw_uart_task", 1024 * 1, NULL, 5, NULL);
}

hw_uart_t *hw_uart_get_instance(void)
{
    return &this;
}
