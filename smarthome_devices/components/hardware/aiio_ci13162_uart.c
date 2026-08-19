#include "aiio_ci13162_uart.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <stream_buffer.h>
#include <task.h>
#include <hosal_uart.h>
#include <bl602_glb.h>
#include <bl602_hbn.h>
#include <bl602_uart.h>
#include <bl_gpio.h>
#include <bl_sys.h>

#include "aiio_ci13162_config.h"
#include "app_config.h"
#include "app_fan.h"
#include "m_fan.h"
#include "m_fan_control.h"
#include "system_typedef.h"

#if AIIO_CI13162_UART_ENABLE == 1

#define AIIO_CI13162_UART_ID                    0U
#define AIIO_CI13162_UART_TX_PIN                16U
#define AIIO_CI13162_UART_INVALID_PIN           255U
#define AIIO_CI13162_UART_ISR_BUFFER_SIZE       64U
#define AIIO_CI13162_UART_READ_BUFFER_SIZE      64U
#define AIIO_CI13162_UART_PRINT_BUFFER_SIZE     96U
#define AIIO_CI13162_UART_TASK_STACK_DEPTH      768U
#define AIIO_CI13162_UART_TASK_PRIORITY         11U
#define AIIO_CI13162_UART_COMMAND_QUEUE_LENGTH  4U
#define AIIO_CI13162_UART_RX_TRIGGER_LEVEL      1U
#define AIIO_CI13162_UART_DIAG_ENABLE           0
#define AIIO_CI13162_UART_TX_IDLE_HOLD_MS       20U
#define AIIO_CI13162_VERSION_INITIAL_DELAY_MS   3000U
#define AIIO_CI13162_VERSION_RETRY_DELAY_MS     1000U
#define AIIO_CI13162_VERSION_MAX_ATTEMPTS       3U
#define AIIO_CI13162_UART_GPIO_FUNCTION         7U
#define AIIO_CI13162_UART_DIAG_BUFFER_SIZE      384U
#define AIIO_CI13162_UART_HBN_IRQ_MODE_ADDRESS  (HBN_BASE + HBN_IRQ_MODE_OFFSET)
#define AIIO_CI13162_UART_GPIO7_CFG_ADDRESS     (GLB_BASE + GLB_GPIO_CFGCTL3_OFFSET)
#define AIIO_CI13162_UART_GLB_PARM_ADDRESS      (GLB_BASE + GLB_PARM_OFFSET)
#define AIIO_CI13162_UART_SIG_SEL_ADDRESS       (GLB_BASE + GLB_UART_SIG_SEL_0_OFFSET)
#define AIIO_CI13162_UART_REG32(address)        (*(volatile uint32_t *)(uintptr_t)(address))

static const char s_aiio_ci13162_reset_message[] = "{\"cmd\":0,\"data\":[]}\r\n";
static const char s_aiio_ci13162_version_query[] = "{\"cmd\":3,\"data\":[]}\r\n";

extern hosal_uart_dev_t uart_stdio;

typedef enum {
    AIIO_CI13162_CMD_RESET = 0,
    AIIO_CI13162_CMD_TURN = 1,
    AIIO_CI13162_CMD_MODE = 2,
    AIIO_CI13162_CMD_VERSION = 3,
} aiio_ci13162_command_t;

typedef enum {
    AIIO_CI13162_PARSE_OK = 0,
    AIIO_CI13162_PARSE_INVALID_ARGUMENT,
    AIIO_CI13162_PARSE_INVALID_FORMAT,
    AIIO_CI13162_PARSE_NUMBER_OVERFLOW,
    AIIO_CI13162_PARSE_UNKNOWN_COMMAND,
    AIIO_CI13162_PARSE_INVALID_DATA,
} aiio_ci13162_parse_result_t;

typedef struct {
    uint8_t command;
    uint8_t data_count;
    uint32_t data;
} aiio_ci13162_message_t;

typedef struct {
    char line[AIIO_CI13162_UART_LINE_MAX];
    size_t length;
    uint8_t pending_carriage_return;
    uint8_t discarding;
} aiio_ci13162_line_assembler_t;

typedef struct {
    uint32_t callback_count;
    uint32_t empty_callback_count;
    uint32_t received_bytes;
    uint32_t stream_bytes;
    uint32_t dropped_bytes;
} aiio_ci13162_counter_snapshot_t;

static StreamBufferHandle_t s_aiio_ci13162_rx_stream;
static StaticStreamBuffer_t s_aiio_ci13162_rx_stream_control;
static uint8_t s_aiio_ci13162_rx_stream_storage[AIIO_CI13162_UART_STREAM_SIZE];
static StreamBufferHandle_t s_aiio_ci13162_ota_rx_stream;
static StaticStreamBuffer_t s_aiio_ci13162_ota_rx_stream_control;
static uint8_t s_aiio_ci13162_ota_rx_stream_storage[AIIO_CI13162_OTA_RX_STREAM_SIZE];
static QueueHandle_t s_aiio_ci13162_command_queue;
static StaticQueue_t s_aiio_ci13162_command_queue_control;
static uint8_t s_aiio_ci13162_command_queue_storage[
    AIIO_CI13162_UART_COMMAND_QUEUE_LENGTH * sizeof(aiio_ci13162_message_t)];
static StaticTask_t s_aiio_ci13162_task_control;
static StackType_t s_aiio_ci13162_task_stack[AIIO_CI13162_UART_TASK_STACK_DEPTH];
static volatile uint32_t s_aiio_ci13162_callback_count;
static volatile uint32_t s_aiio_ci13162_empty_callback_count;
static volatile uint32_t s_aiio_ci13162_received_bytes;
static volatile uint32_t s_aiio_ci13162_stream_bytes;
static volatile uint32_t s_aiio_ci13162_dropped_bytes;
static volatile uint32_t s_aiio_ci13162_total_dropped_bytes;
static volatile uint8_t s_aiio_ci13162_ota_held;
static volatile uint8_t s_aiio_ci13162_ota_rx_overflow;
static uint8_t s_aiio_ci13162_log_was_enabled;
static uint32_t s_aiio_ci13162_voice_version;
static TickType_t s_aiio_ci13162_version_start_tick;
static TickType_t s_aiio_ci13162_version_last_query_tick;
static uint8_t s_aiio_ci13162_voice_version_valid;
static uint8_t s_aiio_ci13162_version_query_attempts;
static uint8_t s_aiio_ci13162_version_query_scheduled;

extern volatile bool sys_log_all_enable;

// Restore the original UART0 TX-only console configuration.
static void aiio_ci13162_restore_tx_only(void)
{
    uart_stdio.config.uart_id = AIIO_CI13162_UART_ID;
    uart_stdio.config.tx_pin = AIIO_CI13162_UART_TX_PIN;
    uart_stdio.config.rx_pin = AIIO_CI13162_UART_INVALID_PIN;
    uart_stdio.config.cts_pin = AIIO_CI13162_UART_INVALID_PIN;
    uart_stdio.config.rts_pin = AIIO_CI13162_UART_INVALID_PIN;
    uart_stdio.config.baud_rate = AIIO_CI13162_UART_BAUD_RATE;
    uart_stdio.config.data_width = HOSAL_DATA_WIDTH_8BIT;
    uart_stdio.config.parity = HOSAL_NO_PARITY;
    uart_stdio.config.stop_bits = HOSAL_STOP_BITS_1;
    uart_stdio.config.flow_control = HOSAL_FLOW_CONTROL_DISABLED;
    uart_stdio.config.mode = HOSAL_UART_MODE_POLL;
    uart_stdio.rx_cb = NULL;
    uart_stdio.p_rxarg = NULL;
    hosal_uart_init_only_tx(&uart_stdio);
}

// Atomically take the number of bytes dropped since the previous task check.
static uint32_t aiio_ci13162_take_dropped_bytes(void)
{
    uint32_t dropped_bytes;

    taskENTER_CRITICAL();
    dropped_bytes = s_aiio_ci13162_dropped_bytes;
    s_aiio_ci13162_dropped_bytes = 0U;
    taskEXIT_CRITICAL();
    return dropped_bytes;
}

#if DEBUG_ENABLE == 1

// Send debug text on the GPIO14 log path, never on UART0 TX.
static void aiio_ci13162_uart_send(const void *data, uint32_t length)
{
    if ((data == NULL) || (length == 0U)) {
        return;
    }

    printf("%.*s", (int)length, (const char *)data);
}

// Send a null-terminated debug string on the GPIO14 log path.
static void aiio_ci13162_uart_send_string(const char *text)
{
    if (text == NULL) {
        return;
    }

    aiio_ci13162_uart_send(text, (uint32_t)strlen(text));
}

#if AIIO_CI13162_UART_DIAG_ENABLE == 1

// Copy ISR counters atomically for task-context diagnostics.
static void aiio_ci13162_snapshot_counters(aiio_ci13162_counter_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    snapshot->callback_count = s_aiio_ci13162_callback_count;
    snapshot->empty_callback_count = s_aiio_ci13162_empty_callback_count;
    snapshot->received_bytes = s_aiio_ci13162_received_bytes;
    snapshot->stream_bytes = s_aiio_ci13162_stream_bytes;
    snapshot->dropped_bytes = s_aiio_ci13162_total_dropped_bytes;
    taskEXIT_CRITICAL();
}

// Report the GPIO7, UART0 IRQ, FIFO, callback, and stream-buffer state.
static void aiio_ci13162_report_diagnostics(void)
{
    static aiio_ci13162_counter_snapshot_t previous_snapshot;
    aiio_ci13162_counter_snapshot_t snapshot;
    char output[AIIO_CI13162_UART_DIAG_BUFFER_SIZE];
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

    aiio_ci13162_snapshot_counters(&snapshot);
    callback_delta = snapshot.callback_count - previous_snapshot.callback_count;
    empty_callback_delta = snapshot.empty_callback_count - previous_snapshot.empty_callback_count;
    received_delta = snapshot.received_bytes - previous_snapshot.received_bytes;
    stream_delta = snapshot.stream_bytes - previous_snapshot.stream_bytes;
    dropped_delta = snapshot.dropped_bytes - previous_snapshot.dropped_bytes;
    previous_snapshot = snapshot;

    hbn_irq_mode = AIIO_CI13162_UART_REG32(AIIO_CI13162_UART_HBN_IRQ_MODE_ADDRESS);
    gpio_config = AIIO_CI13162_UART_REG32(AIIO_CI13162_UART_GPIO7_CFG_ADDRESS);
    uart_signal_select = AIIO_CI13162_UART_REG32(AIIO_CI13162_UART_SIG_SEL_ADDRESS);
    glb_parameter = AIIO_CI13162_UART_REG32(AIIO_CI13162_UART_GLB_PARM_ADDRESS);
    aon_input_enable = (uint8_t)((hbn_irq_mode >> HBN_REG_AON_PAD_IE_SMT_POS) & 0x01U);
    aon_pull_enable = (uint8_t)((hbn_irq_mode >> HBN_REG_EN_HW_PU_PD_POS) & 0x01U);
    gpio_input_enable = (uint8_t)((gpio_config >> GLB_REG_GPIO_7_IE_POS) & 0x01U);
    gpio_schmitt_enable = (uint8_t)((gpio_config >> GLB_REG_GPIO_7_SMT_POS) & 0x01U);
    uart_signal_function = (uint8_t)((uart_signal_select >> GLB_UART_SIG_7_SEL_POS) & 0x0FU);
    uart_swap = (uint8_t)((glb_parameter >> GLB_UART_SWAP_SET_POS) & 0x07U);
    gpio_function = GLB_GPIO_Get_Fun((GLB_GPIO_Type)AIIO_CI13162_UART_RX_PIN);
    gpio_level = bl_gpio_input_get_value((uint8_t)AIIO_CI13162_UART_RX_PIN);
    fifo_count = UART_GetRxFifoCount(UART0_ID);
    irq_fifo = (UART_GetIntStatus(UART0_ID, UART_INT_RX_FIFO_REQ) == SET) ? 1U : 0U;
    irq_timeout = (UART_GetIntStatus(UART0_ID, UART_INT_RTO) == SET) ? 1U : 0U;
    irq_rx_end = (UART_GetIntStatus(UART0_ID, UART_INT_RX_END) == SET) ? 1U : 0U;
    irq_rx_error = (UART_GetIntStatus(UART0_ID, UART_INT_RX_FER) == SET) ? 1U : 0U;
    overflow = (UART_GetOverflowStatus(UART0_ID, UART_RX_OVERFLOW) == SET) ? 1U : 0U;
    rx_busy = (UART_GetRxBusBusyStatus(UART0_ID) == SET) ? 1U : 0U;
    baud_rate = UART_GetBaudrate(UART0_ID);
    stream_available = xStreamBufferBytesAvailable(s_aiio_ci13162_rx_stream);

    if ((aon_input_enable == 0U) ||
        (gpio_input_enable == 0U) ||
        (gpio_schmitt_enable == 0U)) {
        classification = "PAD_DISABLED";
    } else if ((gpio_function != AIIO_CI13162_UART_GPIO_FUNCTION) ||
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
        "[CI13162][DIAG] cls=%s fun=%u sig7=%u swap=%u pin=%u lvl=%d "
        "aon=%u hwpull=%u ie=%u smt=%u baud=%lu busy=%u fifo=%u "
        "irq=%u/%u/%u/%u cb=%lu(+%lu) empty=%lu(+%lu) rx=%lu(+%lu) "
        "stream_rx=%lu(+%lu) buf=%u/%u drop=%lu(+%lu) ovf=%u\r\n",
        classification,
        (unsigned int)gpio_function,
        (unsigned int)uart_signal_function,
        (unsigned int)uart_swap,
        (unsigned int)AIIO_CI13162_UART_RX_PIN,
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
        (unsigned int)AIIO_CI13162_UART_STREAM_SIZE,
        (unsigned long)snapshot.dropped_bytes,
        (unsigned long)dropped_delta,
        (unsigned int)overflow);
    if (output_length <= 0) {
        return;
    }
    if ((size_t)output_length >= sizeof(output)) {
        output_length = (int)(sizeof(output) - 1U);
    }

    aiio_ci13162_uart_send(output, (uint32_t)output_length);
}

#endif

// Print one complete CI13162 message as safe ASCII text.
static void aiio_ci13162_print_ascii(const uint8_t *data, size_t length)
{
    char output[AIIO_CI13162_UART_PRINT_BUFFER_SIZE];
    size_t output_length = 0U;
    size_t i;

    if ((data == NULL) || (length == 0U)) {
        return;
    }

    aiio_ci13162_uart_send_string("[CI13162][ASCII] ");

    for (i = 0U; i < length; i++) {
        uint8_t value = data[i];

        output[output_length++] = ((value >= 0x20U) && (value <= 0x7EU)) ? (char)value : '.';
        if (output_length == sizeof(output)) {
            aiio_ci13162_uart_send(output, (uint32_t)output_length);
            output_length = 0U;
        }
    }

    if (output_length > 0U) {
        aiio_ci13162_uart_send(output, (uint32_t)output_length);
    }
    aiio_ci13162_uart_send_string("\r\n");
}

// Print one complete CI13162 message as hexadecimal bytes.
static void aiio_ci13162_print_hex(const uint8_t *data, size_t length)
{
    static const char aiio_hex_digits[] = "0123456789ABCDEF";
    char output[AIIO_CI13162_UART_PRINT_BUFFER_SIZE];
    size_t output_length = 0U;
    size_t i;

    if ((data == NULL) || (length == 0U)) {
        return;
    }

    aiio_ci13162_uart_send_string("[CI13162][HEX]   ");

    for (i = 0U; i < length; i++) {
        uint8_t value = data[i];

        if ((output_length + 3U) > sizeof(output)) {
            aiio_ci13162_uart_send(output, (uint32_t)output_length);
            output_length = 0U;
        }

        output[output_length++] = aiio_hex_digits[value >> 4U];
        output[output_length++] = aiio_hex_digits[value & 0x0FU];
        output[output_length++] = ' ';
    }

    if (output_length > 0U) {
        aiio_ci13162_uart_send(output, (uint32_t)output_length);
    }
    aiio_ci13162_uart_send_string("\r\n");
}

// Report a rejected line without performing any command side effect.
static void aiio_ci13162_report_reject(aiio_ci13162_parse_result_t result)
{
    static const char *const reasons[] = {
        "OK",
        "INVALID_ARGUMENT",
        "INVALID_FORMAT",
        "NUMBER_OVERFLOW",
        "UNKNOWN_COMMAND",
        "INVALID_DATA",
    };
    char output[80];
    const char *reason = "INTERNAL_ERROR";
    int output_length;

    if ((uint32_t)result < (sizeof(reasons) / sizeof(reasons[0]))) {
        reason = reasons[result];
    }

    output_length = snprintf(output, sizeof(output),
                             "[CI13162][REJECT] reason=%s\r\n", reason);
    if (output_length <= 0) {
        return;
    }
    if ((size_t)output_length >= sizeof(output)) {
        output_length = (int)(sizeof(output) - 1U);
    }

    aiio_ci13162_uart_send(output, (uint32_t)output_length);
}

// Report an overlong or malformed UART line.
static void aiio_ci13162_report_line_error(const char *reason)
{
    char output[80];
    int output_length;

    if (reason == NULL) {
        return;
    }

    output_length = snprintf(output, sizeof(output),
                             "[CI13162][REJECT] reason=%s\r\n", reason);
    if (output_length <= 0) {
        return;
    }
    if ((size_t)output_length >= sizeof(output)) {
        output_length = (int)(sizeof(output) - 1U);
    }

    aiio_ci13162_uart_send(output, (uint32_t)output_length);
}

// Report bytes discarded because the ISR stream buffer was full.
static void aiio_ci13162_report_dropped_bytes(uint32_t dropped_bytes)
{
    char output[64];
    int output_length;

    if (dropped_bytes == 0U) {
        return;
    }

    output_length = snprintf(output, sizeof(output),
                             "[CI13162][WARN] dropped %lu byte(s)\r\n",
                             (unsigned long)dropped_bytes);
    if (output_length <= 0) {
        return;
    }
    if ((size_t)output_length >= sizeof(output)) {
        output_length = (int)(sizeof(output) - 1U);
    }

    aiio_ci13162_uart_send(output, (uint32_t)output_length);
}

#endif

// Skip optional ASCII spacing between protocol tokens.
static void aiio_ci13162_skip_spaces(const char **cursor)
{
    if ((cursor == NULL) || (*cursor == NULL)) {
        return;
    }

    while ((**cursor == ' ') || (**cursor == '\t')) {
        (*cursor)++;
    }
}

// Match one case-sensitive protocol token.
static uint8_t aiio_ci13162_match_token(const char **cursor, const char *token)
{
    size_t token_length;

    if ((cursor == NULL) || (*cursor == NULL) || (token == NULL)) {
        return 0U;
    }

    token_length = strlen(token);
    if (strncmp(*cursor, token, token_length) != 0) {
        return 0U;
    }

    *cursor += token_length;
    return 1U;
}

// Match a protocol key with optional double quotes.
static uint8_t aiio_ci13162_match_key(const char **cursor, const char *key)
{
    uint8_t quoted;

    if ((cursor == NULL) || (*cursor == NULL) || (key == NULL)) {
        return 0U;
    }

    quoted = (**cursor == '"') ? 1U : 0U;
    if (quoted != 0U) {
        (*cursor)++;
    }

    if (aiio_ci13162_match_token(cursor, key) == 0U) {
        return 0U;
    }

    if (quoted != 0U) {
        if (**cursor != '"') {
            return 0U;
        }
        (*cursor)++;
    }

    return 1U;
}

// Parse one unsigned byte-sized decimal value.
static aiio_ci13162_parse_result_t aiio_ci13162_parse_uint8(const char **cursor, uint8_t *value)
{
    uint32_t parsed_value = 0U;
    uint8_t has_digit = 0U;

    if ((cursor == NULL) || (*cursor == NULL) || (value == NULL)) {
        return AIIO_CI13162_PARSE_INVALID_ARGUMENT;
    }

    while ((**cursor >= '0') && (**cursor <= '9')) {
        has_digit = 1U;
        parsed_value = (parsed_value * 10U) + (uint32_t)(**cursor - '0');
        if (parsed_value > UINT8_MAX) {
            return AIIO_CI13162_PARSE_NUMBER_OVERFLOW;
        }
        (*cursor)++;
    }

    if (has_digit == 0U) {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    *value = (uint8_t)parsed_value;
    return AIIO_CI13162_PARSE_OK;
}

// Parse one unsigned 32-bit decimal value.
static aiio_ci13162_parse_result_t aiio_ci13162_parse_uint32(const char **cursor, uint32_t *value)
{
    uint32_t parsed_value = 0U;
    uint8_t has_digit = 0U;

    if ((cursor == NULL) || (*cursor == NULL) || (value == NULL)) {
        return AIIO_CI13162_PARSE_INVALID_ARGUMENT;
    }

    while ((**cursor >= '0') && (**cursor <= '9')) {
        uint32_t digit = (uint32_t)(**cursor - '0');

        has_digit = 1U;
        if (parsed_value > (UINT32_MAX / 10U)) {
            return AIIO_CI13162_PARSE_NUMBER_OVERFLOW;
        }
        parsed_value *= 10U;
        if (parsed_value > (UINT32_MAX - digit)) {
            return AIIO_CI13162_PARSE_NUMBER_OVERFLOW;
        }
        parsed_value += digit;
        (*cursor)++;
    }

    if (has_digit == 0U) {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    *value = parsed_value;
    return AIIO_CI13162_PARSE_OK;
}

// Validate the command-specific data count and range.
static aiio_ci13162_parse_result_t aiio_ci13162_validate_message(
    const aiio_ci13162_message_t *message)
{
    if (message == NULL) {
        return AIIO_CI13162_PARSE_INVALID_ARGUMENT;
    }

    switch (message->command) {
        case AIIO_CI13162_CMD_RESET:
            return (message->data_count == 0U) ? AIIO_CI13162_PARSE_OK
                                               : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_TURN:
            return ((message->data_count == 1U) && (message->data <= 1U))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_MODE:
            return ((message->data_count == 1U) &&
                    (message->data >= 1U) &&
                    (message->data <= 4U))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_VERSION:
            return (message->data_count == 1U) ? AIIO_CI13162_PARSE_OK
                                               : AIIO_CI13162_PARSE_INVALID_DATA;

        default:
            return AIIO_CI13162_PARSE_UNKNOWN_COMMAND;
    }
}

// Parse one complete CI13162 line without allocating memory.
static aiio_ci13162_parse_result_t aiio_ci13162_parse_line(
    const char *line,
    aiio_ci13162_message_t *message)
{
    const char *cursor = line;
    aiio_ci13162_parse_result_t result;

    if ((line == NULL) || (message == NULL)) {
        return AIIO_CI13162_PARSE_INVALID_ARGUMENT;
    }

    memset(message, 0, sizeof(*message));
    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor++ != '{') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    aiio_ci13162_skip_spaces(&cursor);
    if (aiio_ci13162_match_key(&cursor, "cmd") == 0U) {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }
    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor++ != ':') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }
    aiio_ci13162_skip_spaces(&cursor);
    result = aiio_ci13162_parse_uint8(&cursor, &message->command);
    if (result != AIIO_CI13162_PARSE_OK) {
        return result;
    }

    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor++ != ',') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }
    aiio_ci13162_skip_spaces(&cursor);
    if (aiio_ci13162_match_key(&cursor, "data") == 0U) {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }
    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor++ != ':') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }
    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor++ != '[') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor != ']') {
        result = aiio_ci13162_parse_uint32(&cursor, &message->data);
        if (result != AIIO_CI13162_PARSE_OK) {
            return result;
        }
        message->data_count = 1U;
        aiio_ci13162_skip_spaces(&cursor);
    }
    if (*cursor++ != ']') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor++ != '}') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }
    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor != '\0') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    return aiio_ci13162_validate_message(message);
}

// Report the current fan state using the voice source identifier.
static void aiio_ci13162_report_fan_state(const m_fan_info_t *fan_info)
{
    if (fan_info == NULL) {
        return;
    }

    app_fan_respond_control(fan_info->mode,
                            fan_info->speed,
                            fan_info->pcn,
                            fan_info->swing,
                            fan_info->light,
                            0U,
                            SRC_ACTION_VOICE);
}

// Send the scheduled version query without blocking the application task.
static void aiio_ci13162_process_version_query(void)
{
    TickType_t current_tick;
    TickType_t reference_tick;
    TickType_t delay_tick;

    if ((s_aiio_ci13162_version_query_scheduled == 0U) ||
        (s_aiio_ci13162_voice_version_valid != 0U) ||
        (s_aiio_ci13162_version_query_attempts >= AIIO_CI13162_VERSION_MAX_ATTEMPTS) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return;
    }

    current_tick = xTaskGetTickCount();
    if (s_aiio_ci13162_version_query_attempts == 0U) {
        reference_tick = s_aiio_ci13162_version_start_tick;
        delay_tick = pdMS_TO_TICKS(AIIO_CI13162_VERSION_INITIAL_DELAY_MS);
    } else {
        reference_tick = s_aiio_ci13162_version_last_query_tick;
        delay_tick = pdMS_TO_TICKS(AIIO_CI13162_VERSION_RETRY_DELAY_MS);
    }

    if ((TickType_t)(current_tick - reference_tick) < delay_tick) {
        return;
    }

    s_aiio_ci13162_version_last_query_tick = current_tick;
    s_aiio_ci13162_version_query_attempts++;
    (void)aiio_ci13162_uart_request_version();
}

// Queue one validated command for application-context dispatch.
static uint8_t aiio_ci13162_queue_message(const aiio_ci13162_message_t *message)
{
    if ((message == NULL) || (s_aiio_ci13162_command_queue == NULL)) {
        return 0U;
    }

    return (xQueueSend(s_aiio_ci13162_command_queue, message, 0U) == pdPASS) ? 1U : 0U;
}

// Parse and queue one CRLF-delimited line.
static void aiio_ci13162_handle_line(char *line, size_t length)
{
    aiio_ci13162_message_t message;
    aiio_ci13162_parse_result_t result;

    if ((line == NULL) || (length == 0U) ||
        (length >= AIIO_CI13162_UART_LINE_MAX)) {
#if DEBUG_ENABLE == 1
        aiio_ci13162_report_line_error("EMPTY_OR_INVALID_LENGTH");
#endif
        return;
    }

    line[length] = '\0';
#if DEBUG_ENABLE == 1
    aiio_ci13162_print_ascii((const uint8_t *)line, length);
    aiio_ci13162_print_hex((const uint8_t *)line, length);
#endif

    result = aiio_ci13162_parse_line(line, &message);
    if (result != AIIO_CI13162_PARSE_OK) {
#if DEBUG_ENABLE == 1
        aiio_ci13162_report_reject(result);
#endif
        return;
    }

    if (aiio_ci13162_queue_message(&message) == 0U) {
#if DEBUG_ENABLE == 1
        aiio_ci13162_report_line_error("COMMAND_QUEUE_FULL");
#endif
    }
}

#if BUILD_TEST == 1

// Run parser-only regression cases without executing valid fan commands.
static void aiio_ci13162_run_parser_tests(void)
{
    static const struct {
        const char *line;
        aiio_ci13162_parse_result_t expected_result;
        uint8_t expected_command;
        uint8_t expected_data_count;
        uint32_t expected_data;
    } test_cases[] = {
        {"{cmd:0,data:[]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_RESET, 0U, 0U},
        {"{cmd:1,data:[0]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_TURN, 1U, 0U},
        {"{cmd:1,data:[1]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_TURN, 1U, 1U},
        {"{cmd:2,data:[1]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_MODE, 1U, 1U},
        {"{cmd:2,data:[2]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_MODE, 1U, 2U},
        {"{cmd:2,data:[3]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_MODE, 1U, 3U},
        {"{cmd:2,data:[4]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_MODE, 1U, 4U},
        {"{\"cmd\":0,\"data\":[]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_RESET, 0U, 0U},
        {"{\"cmd\":1,\"data\":[0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TURN, 1U, 0U},
        {"{\"cmd\":1,\"data\":[1]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TURN, 1U, 1U},
        {"{\"cmd\":2,\"data\":[4]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_MODE, 1U, 4U},
        {" { cmd : 1 , data : [ 1 ] } ", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TURN, 1U, 1U},
        {" { \"cmd\" : 2 , \"data\" : [ 3 ] } ", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_MODE, 1U, 3U},
        {"{\"cmd\":3,\"data\":[260813]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VERSION, 1U, 260813U},
        {"{cmd:3,data:[260813]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VERSION, 1U, 260813U},
        {"{cmd:3,data:[]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U, 0U},
        {"{cmd:0,data:[0]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U, 0U},
        {"{cmd:1,data:[]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U, 0U},
        {"{cmd:1,data:[2]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U, 0U},
        {"{cmd:2,data:[0]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U, 0U},
        {"{cmd:2,data:[5]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U, 0U},
        {"{cmd:9,data:[1]}", AIIO_CI13162_PARSE_UNKNOWN_COMMAND, 0U, 0U, 0U},
        {"{cmd:1,data:[-1]}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U, 0U},
        {"{cmd:256,data:[]}", AIIO_CI13162_PARSE_NUMBER_OVERFLOW, 0U, 0U, 0U},
        {"{cmd:1,data:[1],x:1}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U, 0U},
        {"{\"cmd:1,\"data\":[1]}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U, 0U},
    };
    aiio_ci13162_message_t message;
    aiio_ci13162_parse_result_t result;
    uint32_t failed_count = 0U;
    size_t i;

    for (i = 0U; i < (sizeof(test_cases) / sizeof(test_cases[0])); i++) {
        result = aiio_ci13162_parse_line(test_cases[i].line, &message);
        if ((result != test_cases[i].expected_result) ||
            ((result == AIIO_CI13162_PARSE_OK) &&
             ((message.command != test_cases[i].expected_command) ||
              (message.data_count != test_cases[i].expected_data_count) ||
              (message.data != test_cases[i].expected_data)))) {
            failed_count++;
        }
    }

#if DEBUG_ENABLE == 1
    {
        char output[80];
        int output_length;

        output_length = snprintf(output,
                                 sizeof(output),
                                 "[CI13162][TEST] parser total=%u failed=%lu\r\n",
                                 (unsigned int)(sizeof(test_cases) / sizeof(test_cases[0])),
                                 (unsigned long)failed_count);
        if (output_length > 0) {
            if ((size_t)output_length >= sizeof(output)) {
                output_length = (int)(sizeof(output) - 1U);
            }
            aiio_ci13162_uart_send(output, (uint32_t)output_length);
        }
    }
#else
    (void)failed_count;
#endif
}

#endif

// Reset a line assembler after a complete or rejected frame.
static void aiio_ci13162_reset_assembler(aiio_ci13162_line_assembler_t *assembler)
{
    if (assembler == NULL) {
        return;
    }

    assembler->length = 0U;
    assembler->pending_carriage_return = 0U;
    assembler->discarding = 0U;
}

// Consume one byte and queue only lines terminated by CRLF.
static void aiio_ci13162_process_byte(
    aiio_ci13162_line_assembler_t *assembler,
    uint8_t value)
{
    if (assembler == NULL) {
        return;
    }

    if (assembler->discarding != 0U) {
        if ((assembler->pending_carriage_return != 0U) && (value == '\n')) {
            aiio_ci13162_reset_assembler(assembler);
            return;
        }
        assembler->pending_carriage_return = (value == '\r') ? 1U : 0U;
        return;
    }

    if (assembler->pending_carriage_return != 0U) {
        if (value == '\n') {
            aiio_ci13162_handle_line(assembler->line, assembler->length);
            aiio_ci13162_reset_assembler(assembler);
            return;
        }

        assembler->pending_carriage_return = 0U;
        if (assembler->length > 0U) {
            assembler->length = 0U;
            assembler->discarding = 1U;
            assembler->pending_carriage_return = (value == '\r') ? 1U : 0U;
#if DEBUG_ENABLE == 1
            aiio_ci13162_report_line_error("INVALID_TERMINATOR");
#endif
            return;
        }
    }

    if (value == '\r') {
        assembler->pending_carriage_return = 1U;
        return;
    }

    if (value == '\n') {
        assembler->length = 0U;
#if DEBUG_ENABLE == 1
        aiio_ci13162_report_line_error("LF_WITHOUT_CR");
#endif
        return;
    }

    if (value == '\0') {
        assembler->length = 0U;
        assembler->discarding = 1U;
#if DEBUG_ENABLE == 1
        aiio_ci13162_report_line_error("NON_ASCII_DATA");
#endif
        return;
    }

    if (assembler->length >= (AIIO_CI13162_UART_LINE_MAX - 1U)) {
        assembler->length = 0U;
        assembler->discarding = 1U;
#if DEBUG_ENABLE == 1
        aiio_ci13162_report_line_error("LINE_OVERFLOW");
#endif
        return;
    }

    assembler->line[assembler->length++] = (char)value;
}

#if BUILD_TEST == 1

// Run byte-assembler tests without executing any fan command.
static void aiio_ci13162_run_assembler_tests(void)
{
    static const uint8_t fragmented_part_1[] = "{cmd:1,da";
    static const uint8_t fragmented_part_2[] = "ta:[1]}\r\n";
    static const uint8_t coalesced_messages[] =
        "{cmd:1,data:[0]}\r\n{cmd:2,data:[3]}\r\n";
    static const uint8_t stray_cr_message[] = "\r{cmd:2,data:[4]}\r\n";
    static const uint8_t embedded_null_message[] = {
        '{', 'c', 'm', 'd', ':', '1', ',', 'd', 'a', 't', 'a', ':', '[', '1', ']', '}',
        '\0', 'X', '\r', '\n'
    };
    aiio_ci13162_line_assembler_t assembler;
    aiio_ci13162_message_t message;
    uint32_t failed_count = 0U;
    size_t i;

    xQueueReset(s_aiio_ci13162_command_queue);
    memset(&assembler, 0, sizeof(assembler));
    for (i = 0U; i < sizeof(fragmented_part_1) - 1U; i++) {
        aiio_ci13162_process_byte(&assembler, fragmented_part_1[i]);
    }
    for (i = 0U; i < sizeof(fragmented_part_2) - 1U; i++) {
        aiio_ci13162_process_byte(&assembler, fragmented_part_2[i]);
    }
    if ((xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) != pdPASS) ||
        (message.command != AIIO_CI13162_CMD_TURN) ||
        (message.data != 1U)) {
        failed_count++;
    }

    memset(&assembler, 0, sizeof(assembler));
    for (i = 0U; i < sizeof(coalesced_messages) - 1U; i++) {
        aiio_ci13162_process_byte(&assembler, coalesced_messages[i]);
    }
    if ((xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) != pdPASS) ||
        (message.command != AIIO_CI13162_CMD_TURN) ||
        (message.data != 0U) ||
        (xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) != pdPASS) ||
        (message.command != AIIO_CI13162_CMD_MODE) ||
        (message.data != 3U)) {
        failed_count++;
    }

    memset(&assembler, 0, sizeof(assembler));
    for (i = 0U; i < sizeof(stray_cr_message) - 1U; i++) {
        aiio_ci13162_process_byte(&assembler, stray_cr_message[i]);
    }
    if ((xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) != pdPASS) ||
        (message.command != AIIO_CI13162_CMD_MODE) ||
        (message.data != 4U)) {
        failed_count++;
    }

    memset(&assembler, 0, sizeof(assembler));
    for (i = 0U; i < sizeof(embedded_null_message); i++) {
        aiio_ci13162_process_byte(&assembler, embedded_null_message[i]);
    }
    if (xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) == pdPASS) {
        failed_count++;
    }

#if DEBUG_ENABLE == 1
    {
        char output[80];
        int output_length;

        output_length = snprintf(output,
                                 sizeof(output),
                                 "[CI13162][TEST] assembler failed=%lu\r\n",
                                 (unsigned long)failed_count);
        if (output_length > 0) {
            if ((size_t)output_length >= sizeof(output)) {
                output_length = (int)(sizeof(output) - 1U);
            }
            aiio_ci13162_uart_send(output, (uint32_t)output_length);
        }
    }
#else
    (void)failed_count;
#endif
    xQueueReset(s_aiio_ci13162_command_queue);
}

#endif

// Drain the hardware RX FIFO into the active application or OTA stream buffer.
static int aiio_ci13162_rx_callback(void *argument)
{
    uint8_t input[AIIO_CI13162_UART_ISR_BUFFER_SIZE];
    StreamBufferHandle_t target_stream;
    BaseType_t higher_priority_task_woken = pdFALSE;
    int received_length;
    size_t stored_length;

    (void)argument;
    s_aiio_ci13162_callback_count++;

    received_length = hosal_uart_receive(&uart_stdio, input, sizeof(input));
    target_stream = (s_aiio_ci13162_ota_held != 0U)
                        ? s_aiio_ci13162_ota_rx_stream
                        : s_aiio_ci13162_rx_stream;
    if ((received_length <= 0) || (target_stream == NULL)) {
        s_aiio_ci13162_empty_callback_count++;
        return 0;
    }

    s_aiio_ci13162_received_bytes += (uint32_t)received_length;
    stored_length = xStreamBufferSendFromISR(target_stream,
                                              input,
                                              (size_t)received_length,
                                              &higher_priority_task_woken);
    s_aiio_ci13162_stream_bytes += (uint32_t)stored_length;
    if (stored_length < (size_t)received_length) {
        uint32_t dropped_length;

        dropped_length = (uint32_t)((size_t)received_length - stored_length);
        if (s_aiio_ci13162_ota_held != 0U) {
            s_aiio_ci13162_ota_rx_overflow = 1U;
        } else {
            s_aiio_ci13162_dropped_bytes += dropped_length;
        }
        s_aiio_ci13162_total_dropped_bytes += dropped_length;
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
    return 0;
}

// Assemble and process CRLF-delimited voice commands outside interrupt context.
static void aiio_ci13162_uart_task(void *argument)
{
    aiio_ci13162_line_assembler_t assembler;
    uint8_t input[AIIO_CI13162_UART_READ_BUFFER_SIZE];

    (void)argument;
    memset(&assembler, 0, sizeof(assembler));

    while (1) {
        uint32_t dropped_bytes;
        size_t received_length;
        size_t i;

        received_length = xStreamBufferReceive(s_aiio_ci13162_rx_stream,
                                                input,
                                                sizeof(input),
                                                portMAX_DELAY);
        if (s_aiio_ci13162_ota_held != 0U) {
            aiio_ci13162_reset_assembler(&assembler);
            vTaskDelay(pdMS_TO_TICKS(10U));
            continue;
        }

        dropped_bytes = aiio_ci13162_take_dropped_bytes();
        if (dropped_bytes > 0U) {
            /* The stream has no marker for the exact gap position. Discard all
             * buffered bytes so a frame spanning the gap can never execute. */
            aiio_ci13162_reset_assembler(&assembler);
            assembler.discarding = 1U;
            taskENTER_CRITICAL();
            (void)xStreamBufferReset(s_aiio_ci13162_rx_stream);
            taskEXIT_CRITICAL();
#if DEBUG_ENABLE == 1
            aiio_ci13162_report_line_error("STREAM_DROP_RESYNC");
            aiio_ci13162_report_dropped_bytes(dropped_bytes);
#endif
            continue;
        }

        if (received_length == 0U) {
            continue;
        }

        for (i = 0U; i < received_length; i++) {
            aiio_ci13162_process_byte(&assembler, input[i]);
        }
    }
}

#endif

void aiio_ci13162_uart_hold_tx_idle(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    // Latch HIGH first so remux to GPIO cannot emit a start bit.
    bl_gpio_output_set((uint8_t)AIIO_CI13162_UART_TX_PIN, 1U);
    bl_gpio_enable_output((uint8_t)AIIO_CI13162_UART_TX_PIN, 1U, 0U);
    bl_gpio_output_set((uint8_t)AIIO_CI13162_UART_TX_PIN, 1U);
    UART_Disable(UART0_ID, UART_TXRX);
#endif
}

int32_t aiio_ci13162_uart_init(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    TaskHandle_t task_handle;

    aiio_ci13162_uart_hold_tx_idle();

    if ((AIIO_CI13162_UART_RX_PIN == AIIO_CI13162_UART_TX_PIN) ||
        (AIIO_CI13162_UART_RX_PIN == AIIO_CI13162_UART_INVALID_PIN) ||
        (AIIO_CI13162_UART_BAUD_RATE == 0U) ||
        (AIIO_CI13162_UART_STREAM_SIZE < 2U) ||
        (AIIO_CI13162_UART_LINE_MAX < 2U)) {
        return -1;
    }

    s_aiio_ci13162_rx_stream = xStreamBufferCreateStatic(
        sizeof(s_aiio_ci13162_rx_stream_storage),
        AIIO_CI13162_UART_RX_TRIGGER_LEVEL,
        s_aiio_ci13162_rx_stream_storage,
        &s_aiio_ci13162_rx_stream_control);
    if (s_aiio_ci13162_rx_stream == NULL) {
        return -1;
    }

    s_aiio_ci13162_ota_rx_stream = xStreamBufferCreateStatic(
        sizeof(s_aiio_ci13162_ota_rx_stream_storage),
        AIIO_CI13162_UART_RX_TRIGGER_LEVEL,
        s_aiio_ci13162_ota_rx_stream_storage,
        &s_aiio_ci13162_ota_rx_stream_control);
    if (s_aiio_ci13162_ota_rx_stream == NULL) {
        return -1;
    }

    s_aiio_ci13162_command_queue = xQueueCreateStatic(
        AIIO_CI13162_UART_COMMAND_QUEUE_LENGTH,
        sizeof(aiio_ci13162_message_t),
        s_aiio_ci13162_command_queue_storage,
        &s_aiio_ci13162_command_queue_control);
    if (s_aiio_ci13162_command_queue == NULL) {
        return -1;
    }

    // Pin is already GPIO idle-high. Do not finalize first: UART_Disable
    // while GPIO16 is still UART AF can drop TX and look like a BREAK.
    vTaskDelay(pdMS_TO_TICKS(AIIO_CI13162_UART_TX_IDLE_HOLD_MS));

    uart_stdio.config.uart_id = AIIO_CI13162_UART_ID;
    uart_stdio.config.tx_pin = AIIO_CI13162_UART_TX_PIN;
    uart_stdio.config.rx_pin = AIIO_CI13162_UART_RX_PIN;
    uart_stdio.config.cts_pin = AIIO_CI13162_UART_INVALID_PIN;
    uart_stdio.config.rts_pin = AIIO_CI13162_UART_INVALID_PIN;
    uart_stdio.config.baud_rate = AIIO_CI13162_UART_BAUD_RATE;
    uart_stdio.config.data_width = HOSAL_DATA_WIDTH_8BIT;
    uart_stdio.config.parity = HOSAL_NO_PARITY;
    uart_stdio.config.stop_bits = HOSAL_STOP_BITS_1;
    uart_stdio.config.flow_control = HOSAL_FLOW_CONTROL_DISABLED;
    uart_stdio.config.mode = HOSAL_UART_MODE_INT;
    uart_stdio.rx_cb = aiio_ci13162_rx_callback;
    uart_stdio.p_rxarg = NULL;

    if (hosal_uart_init(&uart_stdio) != 0) {
        aiio_ci13162_restore_tx_only();
        return -1;
    }

    if (AIIO_CI13162_UART_RX_PIN == GLB_GPIO_PIN_7) {
        HBN_Hw_Pu_Pd_Cfg(DISABLE);
        HBN_Aon_Pad_IeSmt_Cfg(ENABLE);
        GLB_GPIO_INPUT_Enable(GLB_GPIO_PIN_7);
        GLB_UART_Sig_Swap_Set(UART_SIG_SWAP_NONE);
        GLB_UART_Fun_Sel(GLB_UART_SIG_7, GLB_UART_SIG_FUN_UART0_RXD);
    }

    task_handle = xTaskCreateStatic(aiio_ci13162_uart_task,
                                    "ci13162_uart",
                                    AIIO_CI13162_UART_TASK_STACK_DEPTH,
                                    NULL,
                                    AIIO_CI13162_UART_TASK_PRIORITY,
                                    s_aiio_ci13162_task_stack,
                                    &s_aiio_ci13162_task_control);
    if (task_handle == NULL) {
        hosal_uart_finalize(&uart_stdio);
        aiio_ci13162_restore_tx_only();
        return -1;
    }

    s_aiio_ci13162_voice_version = 0U;
    s_aiio_ci13162_voice_version_valid = 0U;
    s_aiio_ci13162_version_query_attempts = 0U;
    s_aiio_ci13162_version_last_query_tick = 0U;
    s_aiio_ci13162_version_start_tick = xTaskGetTickCount();
    s_aiio_ci13162_version_query_scheduled = 1U;

#if DEBUG_ENABLE == 1
    {
        char output[80];
        int output_length;

        output_length = snprintf(output,
                                 sizeof(output),
                                 "[CI13162] GPIO%u, %lu 8N1 ready\r\n",
                                 (unsigned int)AIIO_CI13162_UART_RX_PIN,
                                 (unsigned long)AIIO_CI13162_UART_BAUD_RATE);
        if (output_length > 0) {
            if ((size_t)output_length >= sizeof(output)) {
                output_length = (int)(sizeof(output) - 1U);
            }
            aiio_ci13162_uart_send(output, (uint32_t)output_length);
        }
    }
#endif
#if BUILD_TEST == 1
    aiio_ci13162_run_parser_tests();
    aiio_ci13162_run_assembler_tests();
#endif
    return 0;
#else
    return 0;
#endif
}

void aiio_ci13162_uart_process(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    aiio_ci13162_message_t message;
    m_fan_info_t *fan_info;
    uint8_t base_speed;

    if ((s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return;
    }

    while (xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) == pdPASS) {
        if (message.command == AIIO_CI13162_CMD_VERSION) {
            s_aiio_ci13162_voice_version = message.data;
            s_aiio_ci13162_voice_version_valid = 1U;
            s_aiio_ci13162_version_query_scheduled = 0U;
            os_trace("[CI13162] version=%lu\r\n", (unsigned long)message.data);
            continue;
        }

        fan_info = m_fan_get_instance();
        if (fan_info == NULL) {
            return;
        }

        switch (message.command) {
            case AIIO_CI13162_CMD_RESET:
#if DEBUG_ENABLE == 1
                os_trace("[CI13162] software reset\r\n");
#endif
                bl_sys_reset_por();
                break;

            case AIIO_CI13162_CMD_TURN:
                if (message.data == 0U) {
                    if (fan_info->mode != M_FAN_MODE_OFF) {
                        m_fan_off(SRC_ACTION_VOICE);
                    } else {
                        (void)check_fan_change(fan_info->mode,
                                               fan_info->speed,
                                               fan_info->pcn,
                                               fan_info->swing,
                                               fan_info->light);
                        aiio_ci13162_report_fan_state(fan_info);
                    }
                } else if (fan_info->mode == M_FAN_MODE_OFF) {
                    m_fan_set_speed(M_FAN_MODE_ON, M_FAN_SPEED_1);
                    m_fan_need_buzz(1U);
                    aiio_ci13162_report_fan_state(fan_info);
                } else {
                    (void)check_fan_change(fan_info->mode,
                                           fan_info->speed,
                                           fan_info->pcn,
                                           fan_info->swing,
                                           fan_info->light);
                    aiio_ci13162_report_fan_state(fan_info);
                }
                break;

            case AIIO_CI13162_CMD_MODE:
                m_fan_need_buzz(1U);
                if (message.data <= M_FAN_SPEED_3) {
                    m_fan_set_speed(M_FAN_MODE_ON, message.data);
                } else {
                    base_speed = fan_info->speed;
                    if ((base_speed < M_FAN_SPEED_1) || (base_speed > M_FAN_SPEED_3)) {
                        base_speed = M_FAN_SPEED_1;
                    }
                    m_fan_set_speed(M_FAN_MODE_NATURAL, base_speed);
                }
                aiio_ci13162_report_fan_state(fan_info);
                break;

            default:
                break;
        }
    }

    aiio_ci13162_process_version_query();
#endif
}

int32_t aiio_ci13162_uart_ota_acquire(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    uint8_t input[AIIO_CI13162_UART_ISR_BUFFER_SIZE];

    if ((s_aiio_ci13162_rx_stream == NULL) ||
        (s_aiio_ci13162_ota_rx_stream == NULL) ||
        (s_aiio_ci13162_command_queue == NULL)) {
        return -1;
    }

    taskENTER_CRITICAL();
    if (s_aiio_ci13162_ota_held != 0U) {
        taskEXIT_CRITICAL();
        return -1;
    }
    s_aiio_ci13162_ota_held = 1U;
    s_aiio_ci13162_ota_rx_overflow = 0U;
    taskEXIT_CRITICAL();

    s_aiio_ci13162_log_was_enabled = sys_log_all_enable ? 1U : 0U;
    (void)bl_sys_logall_disable();
    (void)xQueueReset(s_aiio_ci13162_command_queue);
    (void)xStreamBufferReset(s_aiio_ci13162_ota_rx_stream);
    while (hosal_uart_receive(&uart_stdio, input, sizeof(input)) > 0) {
    }
    return 0;
#else
    return -1;
#endif
}

void aiio_ci13162_uart_ota_release(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    if (s_aiio_ci13162_ota_held == 0U) {
        return;
    }

    (void)hosal_uart_ioctl(&uart_stdio, HOSAL_UART_FLUSH, NULL);
    (void)hosal_uart_ioctl(&uart_stdio,
                           HOSAL_UART_BAUD_SET,
                           (void *)(uintptr_t)AIIO_CI13162_UART_BAUD_RATE);
    aiio_ci13162_uart_ota_flush_rx();
    (void)xQueueReset(s_aiio_ci13162_command_queue);

    taskENTER_CRITICAL();
    s_aiio_ci13162_ota_held = 0U;
    taskEXIT_CRITICAL();

    if (s_aiio_ci13162_log_was_enabled != 0U) {
        (void)bl_sys_logall_enable();
    }
#endif
}

bool aiio_ci13162_uart_ota_held(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    return s_aiio_ci13162_ota_held != 0U;
#else
    return false;
#endif
}

int32_t aiio_ci13162_uart_ota_set_baud(uint32_t baud)
{
#if AIIO_CI13162_UART_ENABLE == 1
    if ((s_aiio_ci13162_ota_held == 0U) || (baud == 0U)) {
        return -1;
    }

    if (hosal_uart_ioctl(&uart_stdio, HOSAL_UART_FLUSH, NULL) != 0) {
        return -1;
    }
    if (hosal_uart_ioctl(&uart_stdio,
                         HOSAL_UART_BAUD_SET,
                         (void *)(uintptr_t)baud) != 0) {
        return -1;
    }
    aiio_ci13162_uart_ota_flush_rx();
    vTaskDelay(pdMS_TO_TICKS(20U));
    return 0;
#else
    (void)baud;
    return -1;
#endif
}

int32_t aiio_ci13162_uart_ota_write(const void *data, size_t length)
{
#if AIIO_CI13162_UART_ENABLE == 1
    int sent_length;

    if ((s_aiio_ci13162_ota_held == 0U) ||
        (data == NULL) ||
        (length == 0U) ||
        (length > UINT32_MAX)) {
        return -1;
    }

    sent_length = hosal_uart_send(&uart_stdio, data, (uint32_t)length);
    return (sent_length == (int)length) ? sent_length : -1;
#else
    (void)data;
    (void)length;
    return -1;
#endif
}

int32_t aiio_ci13162_uart_ota_read(void *data, size_t length, uint32_t timeout_ms)
{
#if AIIO_CI13162_UART_ENABLE == 1
    size_t received_length;

    if ((s_aiio_ci13162_ota_held == 0U) ||
        (s_aiio_ci13162_ota_rx_stream == NULL) ||
        (data == NULL) ||
        (length == 0U)) {
        return -1;
    }

    received_length = xStreamBufferReceive(s_aiio_ci13162_ota_rx_stream,
                                            data,
                                            length,
                                            pdMS_TO_TICKS(timeout_ms));
    if (s_aiio_ci13162_ota_rx_overflow != 0U) {
        return -1;
    }
    return (int32_t)received_length;
#else
    (void)data;
    (void)length;
    (void)timeout_ms;
    return -1;
#endif
}

void aiio_ci13162_uart_ota_flush_rx(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    uint8_t input[AIIO_CI13162_UART_ISR_BUFFER_SIZE];

    if (s_aiio_ci13162_ota_rx_stream != NULL) {
        (void)xStreamBufferReset(s_aiio_ci13162_ota_rx_stream);
    }
    while (hosal_uart_receive(&uart_stdio, input, sizeof(input)) > 0) {
    }
    s_aiio_ci13162_ota_rx_overflow = 0U;
#endif
}

bool aiio_ci13162_uart_ota_take_overflow(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    uint8_t overflow;

    taskENTER_CRITICAL();
    overflow = s_aiio_ci13162_ota_rx_overflow;
    s_aiio_ci13162_ota_rx_overflow = 0U;
    taskEXIT_CRITICAL();
    return overflow != 0U;
#else
    return false;
#endif
}

int32_t aiio_ci13162_uart_ota_send_app_reset(void)
{
    return aiio_ci13162_uart_ota_write(
        s_aiio_ci13162_reset_message,
        sizeof(s_aiio_ci13162_reset_message) - 1U);
}

int32_t aiio_ci13162_uart_request_version(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    int sent_length;
    uint32_t query_length;

    if ((s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    query_length = (uint32_t)(sizeof(s_aiio_ci13162_version_query) - 1U);
    sent_length = hosal_uart_send(&uart_stdio,
                                  s_aiio_ci13162_version_query,
                                  query_length);
    return (sent_length == (int)query_length) ? 0 : -1;
#else
    return -1;
#endif
}

uint32_t aiio_ci13162_uart_get_voice_version(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    return (s_aiio_ci13162_voice_version_valid != 0U)
               ? s_aiio_ci13162_voice_version
               : 0U;
#else
    return 0U;
#endif
}
