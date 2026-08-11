#include "aiio_uart0_rx_log.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <stream_buffer.h>
#include <task.h>
#include <hosal_uart.h>
#include <bl602_glb.h>
#include <bl602_hbn.h>
#include <bl602_uart.h>
#include <bl_gpio.h>

#include "app_config.h"

#if AIIO_UART0_RX_LOG_ENABLE == 1

#define AIIO_UART0_ID                    0U
#define AIIO_UART0_TX_PIN                16U
#define AIIO_UART0_INVALID_PIN           255U
#define AIIO_UART0_ISR_BUFFER_SIZE       64U
#define AIIO_UART0_FRAME_BUFFER_SIZE     128U
#define AIIO_UART0_PRINT_BUFFER_SIZE     96U
#define AIIO_UART0_RX_TASK_STACK_DEPTH   768U
#define AIIO_UART0_RX_TASK_PRIORITY      11U
#define AIIO_UART0_RX_TRIGGER_LEVEL      1U
#define AIIO_UART0_RX_DIAG_PERIOD_MS     1000U
#define AIIO_UART0_RX_GPIO_FUNCTION      7U
#define AIIO_UART0_RX_DIAG_BUFFER_SIZE   384U
#define AIIO_UART0_HBN_IRQ_MODE_ADDRESS  (HBN_BASE + HBN_IRQ_MODE_OFFSET)
#define AIIO_UART0_GLB_GPIO7_CFG_ADDRESS (GLB_BASE + GLB_GPIO_CFGCTL3_OFFSET)
#define AIIO_UART0_GLB_PARM_ADDRESS      (GLB_BASE + GLB_PARM_OFFSET)
#define AIIO_UART0_GLB_SIG_SEL_ADDRESS   (GLB_BASE + GLB_UART_SIG_SEL_0_OFFSET)
#define AIIO_UART0_REG32(address)        (*(volatile uint32_t *)(uintptr_t)(address))

extern hosal_uart_dev_t uart_stdio;

static StreamBufferHandle_t s_aiio_uart0_rx_stream;
static StaticStreamBuffer_t s_aiio_uart0_rx_stream_control;
static uint8_t s_aiio_uart0_rx_stream_storage[AIIO_UART0_RX_STREAM_SIZE];
static StaticTask_t s_aiio_uart0_rx_task_control;
static StackType_t s_aiio_uart0_rx_task_stack[AIIO_UART0_RX_TASK_STACK_DEPTH];
static volatile uint32_t s_aiio_uart0_rx_callback_count;
static volatile uint32_t s_aiio_uart0_rx_empty_callback_count;
static volatile uint32_t s_aiio_uart0_rx_received_bytes;
static volatile uint32_t s_aiio_uart0_rx_stream_bytes;
static volatile uint32_t s_aiio_uart0_rx_dropped_bytes;
static volatile uint32_t s_aiio_uart0_rx_total_dropped_bytes;

typedef struct {
    uint32_t callback_count;
    uint32_t empty_callback_count;
    uint32_t received_bytes;
    uint32_t stream_bytes;
    uint32_t dropped_bytes;
} aiio_uart0_rx_counter_snapshot_t;

// Restore the original UART0 TX-only console configuration.
static void aiio_uart0_restore_tx_only(void)
{
    uart_stdio.config.uart_id = AIIO_UART0_ID;
    uart_stdio.config.tx_pin = AIIO_UART0_TX_PIN;
    uart_stdio.config.rx_pin = AIIO_UART0_INVALID_PIN;
    uart_stdio.config.cts_pin = AIIO_UART0_INVALID_PIN;
    uart_stdio.config.rts_pin = AIIO_UART0_INVALID_PIN;
    uart_stdio.config.baud_rate = AIIO_UART0_RX_BAUD_RATE;
    uart_stdio.config.data_width = HOSAL_DATA_WIDTH_8BIT;
    uart_stdio.config.parity = HOSAL_NO_PARITY;
    uart_stdio.config.stop_bits = HOSAL_STOP_BITS_1;
    uart_stdio.config.flow_control = HOSAL_FLOW_CONTROL_DISABLED;
    uart_stdio.config.mode = HOSAL_UART_MODE_POLL;
    uart_stdio.rx_cb = NULL;
    uart_stdio.p_rxarg = NULL;
    hosal_uart_init_only_tx(&uart_stdio);
}

// Send diagnostic output directly through the existing UART0 console TX path.
static void aiio_uart0_send(const void *data, uint32_t length)
{
    if ((data == NULL) || (length == 0U)) {
        return;
    }

    hosal_uart_send(&uart_stdio, data, length);
}

// Send a null-terminated diagnostic string through UART0.
static void aiio_uart0_send_string(const char *text)
{
    if (text == NULL) {
        return;
    }

    aiio_uart0_send(text, (uint32_t)strlen(text));
}

// Copy the ISR counters atomically for task-context diagnostics.
static void aiio_uart0_rx_snapshot_counters(aiio_uart0_rx_counter_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    snapshot->callback_count = s_aiio_uart0_rx_callback_count;
    snapshot->empty_callback_count = s_aiio_uart0_rx_empty_callback_count;
    snapshot->received_bytes = s_aiio_uart0_rx_received_bytes;
    snapshot->stream_bytes = s_aiio_uart0_rx_stream_bytes;
    snapshot->dropped_bytes = s_aiio_uart0_rx_total_dropped_bytes;
    taskEXIT_CRITICAL();
}

// Report the GPIO7, UART0 IRQ, FIFO, callback, and stream-buffer state.
static void aiio_uart0_rx_report_diagnostics(void)
{
    static aiio_uart0_rx_counter_snapshot_t previous_snapshot;
    aiio_uart0_rx_counter_snapshot_t snapshot;
    char output[AIIO_UART0_RX_DIAG_BUFFER_SIZE];
    const char *classification;
    uint32_t callback_delta;
    uint32_t empty_callback_delta;
    uint32_t received_delta;
    uint32_t stream_delta;
    uint32_t dropped_delta;
    size_t stream_available;
    uint8_t gpio_function;
    uint8_t fifo_count;
    uint8_t irq_fifo;
    uint8_t irq_timeout;
    uint8_t irq_rx_end;
    uint8_t irq_rx_error;
    uint8_t overflow;
    uint8_t rx_busy;
    uint8_t aon_input_enable;
    uint8_t aon_pull_enable;
    uint8_t gpio_input_enable;
    uint8_t gpio_schmitt_enable;
    uint8_t uart_signal_function;
    uint8_t uart_swap;
    uint32_t baud_rate;
    uint32_t hbn_irq_mode;
    uint32_t gpio_config;
    uint32_t uart_signal_select;
    uint32_t glb_parameter;
    int gpio_level;
    int output_length;

    aiio_uart0_rx_snapshot_counters(&snapshot);
    callback_delta = snapshot.callback_count - previous_snapshot.callback_count;
    empty_callback_delta = snapshot.empty_callback_count - previous_snapshot.empty_callback_count;
    received_delta = snapshot.received_bytes - previous_snapshot.received_bytes;
    stream_delta = snapshot.stream_bytes - previous_snapshot.stream_bytes;
    dropped_delta = snapshot.dropped_bytes - previous_snapshot.dropped_bytes;
    previous_snapshot = snapshot;

    hbn_irq_mode = AIIO_UART0_REG32(AIIO_UART0_HBN_IRQ_MODE_ADDRESS);
    gpio_config = AIIO_UART0_REG32(AIIO_UART0_GLB_GPIO7_CFG_ADDRESS);
    uart_signal_select = AIIO_UART0_REG32(AIIO_UART0_GLB_SIG_SEL_ADDRESS);
    glb_parameter = AIIO_UART0_REG32(AIIO_UART0_GLB_PARM_ADDRESS);
    aon_input_enable = (uint8_t)((hbn_irq_mode >> HBN_REG_AON_PAD_IE_SMT_POS) & 0x01U);
    aon_pull_enable = (uint8_t)((hbn_irq_mode >> HBN_REG_EN_HW_PU_PD_POS) & 0x01U);
    gpio_input_enable = (uint8_t)((gpio_config >> GLB_REG_GPIO_7_IE_POS) & 0x01U);
    gpio_schmitt_enable = (uint8_t)((gpio_config >> GLB_REG_GPIO_7_SMT_POS) & 0x01U);
    uart_signal_function = (uint8_t)((uart_signal_select >> GLB_UART_SIG_7_SEL_POS) & 0x0FU);
    uart_swap = (uint8_t)((glb_parameter >> GLB_UART_SWAP_SET_POS) & 0x07U);
    gpio_function = GLB_GPIO_Get_Fun((GLB_GPIO_Type)AIIO_UART0_RX_PIN);
    gpio_level = bl_gpio_input_get_value((uint8_t)AIIO_UART0_RX_PIN);
    fifo_count = UART_GetRxFifoCount(UART0_ID);
    irq_fifo = (UART_GetIntStatus(UART0_ID, UART_INT_RX_FIFO_REQ) == SET) ? 1U : 0U;
    irq_timeout = (UART_GetIntStatus(UART0_ID, UART_INT_RTO) == SET) ? 1U : 0U;
    irq_rx_end = (UART_GetIntStatus(UART0_ID, UART_INT_RX_END) == SET) ? 1U : 0U;
    irq_rx_error = (UART_GetIntStatus(UART0_ID, UART_INT_RX_FER) == SET) ? 1U : 0U;
    overflow = (UART_GetOverflowStatus(UART0_ID, UART_RX_OVERFLOW) == SET) ? 1U : 0U;
    rx_busy = (UART_GetRxBusBusyStatus(UART0_ID) == SET) ? 1U : 0U;
    baud_rate = UART_GetBaudrate(UART0_ID);
    stream_available = xStreamBufferBytesAvailable(s_aiio_uart0_rx_stream);

    if ((aon_input_enable == 0U) ||
        (gpio_input_enable == 0U) ||
        (gpio_schmitt_enable == 0U)) {
        classification = "PAD_DISABLED";
    } else if ((gpio_function != AIIO_UART0_RX_GPIO_FUNCTION) ||
               (uart_signal_function != GLB_UART_SIG_FUN_UART0_RXD) ||
               (uart_swap != UART_SIG_SWAP_NONE)) {
        classification = "PINMUX";
    } else if ((fifo_count > 0U) && (callback_delta == 0U)) {
        classification = "FIFO_NO_IRQ";
    } else if ((callback_delta > 0U) &&
               (empty_callback_delta == callback_delta) &&
               (received_delta == 0U)) {
        classification = "IRQ_NO_DATA";
    } else if (received_delta > stream_delta) {
        classification = "STREAM_DROP";
    } else if ((received_delta > 0U) || (stream_delta > 0U)) {
        classification = "PATH_OK";
    } else if (stream_available > 0U) {
        classification = "TASK_BACKLOG";
    } else if ((overflow != 0U) || (irq_rx_error != 0U)) {
        classification = "RX_ERROR";
    } else {
        classification = "FIFO_EMPTY";
    }

    output_length = snprintf(
        output,
        sizeof(output),
        "[UART0 RX][DIAG] cls=%s fun=%u sig7=%u swap=%u pin=%u lvl=%d "
        "aon=%u hwpull=%u ie=%u smt=%u baud=%lu busy=%u fifo=%u "
        "irq=%u/%u/%u/%u cb=%lu(+%lu) empty=%lu(+%lu) rx=%lu(+%lu) "
        "stream_rx=%lu(+%lu) buf=%u/%u drop=%lu(+%lu) ovf=%u\r\n",
        classification,
        (unsigned int)gpio_function,
        (unsigned int)uart_signal_function,
        (unsigned int)uart_swap,
        (unsigned int)AIIO_UART0_RX_PIN,
        gpio_level,
        (unsigned int)aon_input_enable,
        (unsigned int)aon_pull_enable,
        (unsigned int)gpio_input_enable,
        (unsigned int)gpio_schmitt_enable,
        (unsigned long)baud_rate,
        (unsigned int)rx_busy,
        (unsigned int)fifo_count,
        (unsigned int)irq_fifo,
        (unsigned int)irq_timeout,
        (unsigned int)irq_rx_end,
        (unsigned int)irq_rx_error,
        (unsigned long)snapshot.callback_count,
        (unsigned long)callback_delta,
        (unsigned long)snapshot.empty_callback_count,
        (unsigned long)empty_callback_delta,
        (unsigned long)snapshot.received_bytes,
        (unsigned long)received_delta,
        (unsigned long)snapshot.stream_bytes,
        (unsigned long)stream_delta,
        (unsigned int)stream_available,
        (unsigned int)AIIO_UART0_RX_STREAM_SIZE,
        (unsigned long)snapshot.dropped_bytes,
        (unsigned long)dropped_delta,
        (unsigned int)overflow);
    if (output_length <= 0) {
        return;
    }
    if ((size_t)output_length >= sizeof(output)) {
        output_length = (int)(sizeof(output) - 1U);
    }

    aiio_uart0_send(output, (uint32_t)output_length);
}

// Print one received frame as safe ASCII text.
static void aiio_uart0_print_ascii(const uint8_t *data, size_t length)
{
    char output[AIIO_UART0_PRINT_BUFFER_SIZE];
    size_t output_length = 0U;

    if ((data == NULL) || (length == 0U)) {
        return;
    }

    aiio_uart0_send_string("[UART0 RX][ASCII] ");

    for (size_t i = 0U; i < length; i++) {
        uint8_t value = data[i];

        if ((value >= 0x20U) && (value <= 0x7EU)) {
            output[output_length++] = (char)value;
        } else if ((value == '\r') || (value == '\n') || (value == '\t')) {
            output[output_length++] = (char)value;
        } else {
            output[output_length++] = '.';
        }

        if (output_length == sizeof(output)) {
            aiio_uart0_send(output, (uint32_t)output_length);
            output_length = 0U;
        }
    }

    if (output_length > 0U) {
        aiio_uart0_send(output, (uint32_t)output_length);
    }
    aiio_uart0_send_string("\r\n");
}

// Print one received frame as hexadecimal bytes.
static void aiio_uart0_print_hex(const uint8_t *data, size_t length)
{
    static const char aiio_hex_digits[] = "0123456789ABCDEF";
    char output[AIIO_UART0_PRINT_BUFFER_SIZE];
    size_t output_length = 0U;

    if ((data == NULL) || (length == 0U)) {
        return;
    }

    aiio_uart0_send_string("[UART0 RX][HEX]   ");

    for (size_t i = 0U; i < length; i++) {
        uint8_t value = data[i];

        if ((output_length + 3U) > sizeof(output)) {
            aiio_uart0_send(output, (uint32_t)output_length);
            output_length = 0U;
        }

        output[output_length++] = aiio_hex_digits[value >> 4U];
        output[output_length++] = aiio_hex_digits[value & 0x0FU];
        output[output_length++] = ' ';
    }

    if (output_length > 0U) {
        aiio_uart0_send(output, (uint32_t)output_length);
    }
    aiio_uart0_send_string("\r\n");
}

// Report bytes discarded because the ISR stream buffer was full.
static void aiio_uart0_report_dropped_bytes(void)
{
    char output[64];
    uint32_t dropped_bytes;
    int output_length;

    taskENTER_CRITICAL();
    dropped_bytes = s_aiio_uart0_rx_dropped_bytes;
    s_aiio_uart0_rx_dropped_bytes = 0U;
    taskEXIT_CRITICAL();

    if (dropped_bytes == 0U) {
        return;
    }

    output_length = snprintf(output, sizeof(output),
                             "[UART0 RX][WARN] dropped %lu byte(s)\r\n",
                             (unsigned long)dropped_bytes);
    if (output_length <= 0) {
        return;
    }
    if ((size_t)output_length >= sizeof(output)) {
        output_length = (int)(sizeof(output) - 1U);
    }

    aiio_uart0_send(output, (uint32_t)output_length);
}

// Drain the hardware RX FIFO into the ISR-safe stream buffer.
static int aiio_uart0_rx_callback(void *argument)
{
    uint8_t input[AIIO_UART0_ISR_BUFFER_SIZE];
    BaseType_t higher_priority_task_woken = pdFALSE;
    int received_length;
    size_t stored_length;

    (void)argument;
    s_aiio_uart0_rx_callback_count++;

    received_length = hosal_uart_receive(&uart_stdio, input, sizeof(input));
    if ((received_length <= 0) || (s_aiio_uart0_rx_stream == NULL)) {
        s_aiio_uart0_rx_empty_callback_count++;
        return 0;
    }

    s_aiio_uart0_rx_received_bytes += (uint32_t)received_length;
    stored_length = xStreamBufferSendFromISR(s_aiio_uart0_rx_stream,
                                              input,
                                              (size_t)received_length,
                                              &higher_priority_task_woken);
    s_aiio_uart0_rx_stream_bytes += (uint32_t)stored_length;
    if (stored_length < (size_t)received_length) {
        uint32_t dropped_length;

        dropped_length = (uint32_t)((size_t)received_length - stored_length);
        s_aiio_uart0_rx_dropped_bytes += dropped_length;
        s_aiio_uart0_rx_total_dropped_bytes += dropped_length;
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
    return 0;
}

// Batch incoming bytes by idle time and print them outside interrupt context.
static void aiio_uart0_rx_task(void *argument)
{
    uint8_t frame[AIIO_UART0_FRAME_BUFFER_SIZE];

    (void)argument;

    while (1) {
        size_t frame_length;
        size_t received_length;

        frame_length = xStreamBufferReceive(
            s_aiio_uart0_rx_stream,
            frame,
            sizeof(frame),
            pdMS_TO_TICKS(AIIO_UART0_RX_DIAG_PERIOD_MS));
        if (frame_length == 0U) {
            aiio_uart0_rx_report_diagnostics();
            continue;
        }

        while (frame_length < sizeof(frame)) {
            received_length = xStreamBufferReceive(
                s_aiio_uart0_rx_stream,
                &frame[frame_length],
                sizeof(frame) - frame_length,
                pdMS_TO_TICKS(AIIO_UART0_RX_IDLE_TIMEOUT_MS));
            if (received_length == 0U) {
                break;
            }
            frame_length += received_length;
        }

        aiio_uart0_print_ascii(frame, frame_length);
        aiio_uart0_print_hex(frame, frame_length);
        aiio_uart0_report_dropped_bytes();
    }
}

#endif

int32_t aiio_uart0_rx_log_init(void)
{
#if AIIO_UART0_RX_LOG_ENABLE == 1
    TaskHandle_t task_handle;

    if ((AIIO_UART0_RX_PIN == AIIO_UART0_TX_PIN) ||
        (AIIO_UART0_RX_PIN == AIIO_UART0_INVALID_PIN) ||
        (AIIO_UART0_RX_BAUD_RATE == 0U) ||
        (AIIO_UART0_RX_STREAM_SIZE < 2U)) {
        return -1;
    }

    s_aiio_uart0_rx_stream = xStreamBufferCreateStatic(
        sizeof(s_aiio_uart0_rx_stream_storage),
        AIIO_UART0_RX_TRIGGER_LEVEL,
        s_aiio_uart0_rx_stream_storage,
        &s_aiio_uart0_rx_stream_control);
    if (s_aiio_uart0_rx_stream == NULL) {
        return -1;
    }

    if (hosal_uart_finalize(&uart_stdio) != 0) {
        return -1;
    }

    uart_stdio.config.uart_id = AIIO_UART0_ID;
    uart_stdio.config.tx_pin = AIIO_UART0_TX_PIN;
    uart_stdio.config.rx_pin = AIIO_UART0_RX_PIN;
    uart_stdio.config.cts_pin = AIIO_UART0_INVALID_PIN;
    uart_stdio.config.rts_pin = AIIO_UART0_INVALID_PIN;
    uart_stdio.config.baud_rate = AIIO_UART0_RX_BAUD_RATE;
    uart_stdio.config.data_width = HOSAL_DATA_WIDTH_8BIT;
    uart_stdio.config.parity = HOSAL_NO_PARITY;
    uart_stdio.config.stop_bits = HOSAL_STOP_BITS_1;
    uart_stdio.config.flow_control = HOSAL_FLOW_CONTROL_DISABLED;
    uart_stdio.config.mode = HOSAL_UART_MODE_INT;
    uart_stdio.rx_cb = aiio_uart0_rx_callback;
    uart_stdio.p_rxarg = NULL;

    if (hosal_uart_init(&uart_stdio) != 0) {
        aiio_uart0_restore_tx_only();
        return -1;
    }

    if (AIIO_UART0_RX_PIN == GLB_GPIO_PIN_7) {
        HBN_Hw_Pu_Pd_Cfg(DISABLE);
        HBN_Aon_Pad_IeSmt_Cfg(ENABLE);
        GLB_GPIO_INPUT_Enable(GLB_GPIO_PIN_7);
        GLB_UART_Sig_Swap_Set(UART_SIG_SWAP_NONE);
        GLB_UART_Fun_Sel(GLB_UART_SIG_7, GLB_UART_SIG_FUN_UART0_RXD);
    }

    task_handle = xTaskCreateStatic(aiio_uart0_rx_task,
                                    "uart0_rx_log",
                                    AIIO_UART0_RX_TASK_STACK_DEPTH,
                                    NULL,
                                    AIIO_UART0_RX_TASK_PRIORITY,
                                    s_aiio_uart0_rx_task_stack,
                                    &s_aiio_uart0_rx_task_control);
    if (task_handle == NULL) {
        hosal_uart_finalize(&uart_stdio);
        aiio_uart0_restore_tx_only();
        return -1;
    }

    {
        char output[80];
        int output_length;

        output_length = snprintf(output,
                                 sizeof(output),
                                 "[UART0 RX] GPIO%u, %lu 8N1 ready\r\n",
                                 (unsigned int)AIIO_UART0_RX_PIN,
                                 (unsigned long)AIIO_UART0_RX_BAUD_RATE);
        if (output_length > 0) {
            if ((size_t)output_length >= sizeof(output)) {
                output_length = (int)(sizeof(output) - 1U);
            }
            aiio_uart0_send(output, (uint32_t)output_length);
        }
    }
    aiio_uart0_rx_report_diagnostics();
    return 0;
#else
    return 0;
#endif
}
