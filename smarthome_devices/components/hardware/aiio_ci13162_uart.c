#include "aiio_ci13162_uart.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <stream_buffer.h>
#include <task.h>
#include <hosal_uart.h>
#include <bl602_glb.h>
#include <bl602_uart.h>
#include <bl_gpio.h>
#include <bl_sys.h>
#include <bl_wdt.h>

#include "aiio_ci13162_config.h"
#include "app_config.h"
#include "app_cwsl.h"
#include "app_scene.h"
#include "app_fan.h"
#include "app_volume.h"
#include "hardware.h"
#include "m_fan.h"
#include "m_fan_control.h"
#include "system_typedef.h"

extern void app_send_data(char *data, uint32_t length);

#if AIIO_CI13162_UART_ENABLE == 1

#define AIIO_CI13162_UART_INVALID_PIN           255U
#define AIIO_CI13162_UART_ISR_BUFFER_SIZE       64U
#define AIIO_CI13162_UART_READ_BUFFER_SIZE      64U
#define AIIO_CI13162_UART_PRINT_BUFFER_SIZE     96U
#define AIIO_CI13162_UART_TASK_STACK_DEPTH      768U
#define AIIO_CI13162_UART_TASK_PRIORITY         11U
#define AIIO_CI13162_UART_COMMAND_QUEUE_LENGTH  8U
#define AIIO_CI13162_UART_RX_TRIGGER_LEVEL      1U
#define AIIO_CI13162_UART_DIAG_ENABLE           0
/* 1: log every JSON frame sent to and every raw chunk received from the CI13162 on the UART0 console. */
#ifndef AIIO_CI13162_UART_TRACE_ENABLE
#define AIIO_CI13162_UART_TRACE_ENABLE          DEBUG_ENABLE
#endif
#define AIIO_CI13162_UART_TX_IDLE_HOLD_MS       20U
#define AIIO_CI13162_VERSION_INITIAL_DELAY_MS   3000U
#define AIIO_CI13162_VERSION_RETRY_DELAY_MS     1000U
#define AIIO_CI13162_VERSION_MAX_ATTEMPTS       3U
#define AIIO_CI13162_VOICE_QUERY_RETRY_MS       1000U
#define AIIO_CI13162_VOICE_QUERY_MAX_ATTEMPTS   3U
#define AIIO_CI13162_VOICE_ID_MAX               4U
#define AIIO_CI13162_UART_GPIO_FUNCTION         7U
#define AIIO_CI13162_UART_DIAG_BUFFER_SIZE      384U
#define AIIO_CI13162_VOLUME_LEVEL_MIN           1U
#define AIIO_CI13162_VOLUME_LEVEL_MAX           7U
#define AIIO_CI13162_ASR_VOLUME_DOWN            40U
#define AIIO_CI13162_ASR_VOLUME_UP              41U
#define AIIO_CI13162_ASR_VOLUME_MIN             42U
#define AIIO_CI13162_ASR_VOLUME_MAX             43U
#define AIIO_CI13162_ASR_VOLUME_MUTE            44U
#define AIIO_CI13162_ASR_VOLUME_UNMUTE          45U

static const char s_aiio_ci13162_reset_message[] = "{\"cmd\":0,\"data\":[]}\r\n";
static const char s_aiio_ci13162_version_query[] = "{\"cmd\":3,\"data\":[]}\r\n";
static const char s_aiio_ci13162_volume_mute[] = "{\"cmd\":8,\"data\":[8]}\r\n";
static const char s_aiio_ci13162_volume_query[] = "{\"cmd\":11,\"data\":[]}\r\n";

// UART1 device owned by this driver; UART0 stays with the SDK log.
static hosal_uart_dev_t s_aiio_ci13162_uart;

typedef enum {
    AIIO_CI13162_CMD_RESET = 0,
    AIIO_CI13162_CMD_TURN = 1,
    AIIO_CI13162_CMD_MODE = 2,
    AIIO_CI13162_CMD_VERSION = 3,
    AIIO_CI13162_CMD_CWSL = 4,
    AIIO_CI13162_CMD_CWSL_STATUS = 5,
    AIIO_CI13162_CMD_CWSL_INFO = 6,
    AIIO_CI13162_CMD_TIMER = 7,
    AIIO_CI13162_CMD_VOLUME = 8,
    AIIO_CI13162_CMD_VOLUME_INFO = 9,
    AIIO_CI13162_CMD_ASR_UNHANDLED = 10,
    AIIO_CI13162_CMD_VOLUME_GET = 11,
    AIIO_CI13162_CMD_SCENE = 12,
    AIIO_CI13162_CMD_VOLUME_CHANGE = 13,
    AIIO_CI13162_CMD_STATE = 14,
    AIIO_CI13162_CMD_RESP_SET = 15,
    AIIO_CI13162_CMD_VOICE_SET = 17,
    AIIO_CI13162_CMD_VOICE_GET = 18,
    AIIO_CI13162_CMD_BUZZ = 200,
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
    uint8_t data[AIIO_CI13162_CWSL_DATA_MAX];
    uint32_t version;
    uint8_t cmd_id;
    uint8_t cwsl;
    uint8_t has_metadata;
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

#define AIIO_CI13162_UART_ERROR_QUEUE_LENGTH    4U

typedef struct {
    char raw[AIIO_CI13162_UART_LINE_MAX];
    const char *reason;
} aiio_ci13162_error_event_t;

static QueueHandle_t s_aiio_ci13162_error_queue;
static StaticQueue_t s_aiio_ci13162_error_queue_control;
static uint8_t s_aiio_ci13162_error_queue_storage[
    AIIO_CI13162_UART_ERROR_QUEUE_LENGTH * sizeof(aiio_ci13162_error_event_t)];
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
static uint32_t s_aiio_ci13162_voice_version;
static TickType_t s_aiio_ci13162_version_start_tick;
static TickType_t s_aiio_ci13162_version_last_query_tick;
static uint8_t s_aiio_ci13162_voice_version_valid;
static uint8_t s_aiio_ci13162_version_query_attempts;
static uint8_t s_aiio_ci13162_version_query_scheduled;
static uint8_t s_aiio_ci13162_version_query_paused;
static uint8_t s_aiio_ci13162_volume;
static uint8_t s_aiio_ci13162_volume_valid;
static uint8_t s_aiio_ci13162_volume_muted;
static uint8_t s_aiio_ci13162_voice_id;
static uint8_t s_aiio_ci13162_voice_set_sent;
static uint8_t s_aiio_ci13162_voice_query_scheduled;
static uint8_t s_aiio_ci13162_voice_query_attempts;
static TickType_t s_aiio_ci13162_voice_query_last_tick;
static aiio_ci13162_voice_set_cb_t s_aiio_ci13162_voice_set_cb;
static aiio_ci13162_voice_get_cb_t s_aiio_ci13162_voice_get_cb;
static aiio_ci13162_cwsl_status_cb_t s_aiio_ci13162_cwsl_status_cb;
static aiio_ci13162_cwsl_info_cb_t s_aiio_ci13162_cwsl_info_cb;
static aiio_ci13162_resp_set_cb_t s_aiio_ci13162_resp_set_cb;
static aiio_ci13162_volume_info_cb_t s_aiio_ci13162_volume_info_cb;
static SemaphoreHandle_t s_aiio_ci13162_tx_mutex;
static StaticSemaphore_t s_aiio_ci13162_tx_mutex_control;

#if AIIO_CI13162_UART_TRACE_ENABLE == 1

// Flush one chunk of trace text to the UART0 console.
static void aiio_ci13162_trace_flush(char *text, size_t *text_length)
{
    if (*text_length > 0U) {
        printf("%.*s", (int)*text_length, text);
        *text_length = 0U;
    }
}

// Log raw bytes exchanged with the CI13162 as safe ASCII and as hex.
static void aiio_ci13162_trace_bytes(const char *direction,
                                     const uint8_t *data,
                                     size_t length)
{
    // static const char hex_digits[] = "0123456789ABCDEF";
    char text[AIIO_CI13162_UART_PRINT_BUFFER_SIZE];
    size_t text_length = 0U;
    size_t i;

    if ((direction == NULL) || (data == NULL) || (length == 0U)) {
        return;
    }

    printf("[CI13162][%s] %u Arr_Bytes: ", direction, (unsigned int)length);
    for (i = 0U; i < length; i++) {
        uint8_t value = data[i];

        if (text_length >= sizeof(text)) {
            aiio_ci13162_trace_flush(text, &text_length);
        }
        text[text_length++] = ((value >= 0x20U) && (value <= 0x7EU)) ? (char)value : '.';
    }
    aiio_ci13162_trace_flush(text, &text_length);
    printf("\r\n");

    // printf("\r\n[CI13162][%s] HEX: ", direction);
    // for (i = 0U; i < length; i++) {
    //     uint8_t value = data[i];

    //     if ((text_length + 3U) > sizeof(text)) {
    //         aiio_ci13162_trace_flush(text, &text_length);
    //     }
    //     text[text_length++] = hex_digits[value >> 4U];
    //     text[text_length++] = hex_digits[value & 0x0FU];
    //     text[text_length++] = ' ';
    // }
    // aiio_ci13162_trace_flush(text, &text_length);
    // printf("\r\n");
}

#endif

// Send UART bytes safely across multiple task contexts.
static int aiio_ci13162_uart_tx(const void *data, uint32_t length)
{
    int sent_length = -1;

    if ((data == NULL) || (length == 0U)) {
        return -1;
    }

    if (s_aiio_ci13162_tx_mutex != NULL) {
        if (xSemaphoreTake(s_aiio_ci13162_tx_mutex, pdMS_TO_TICKS(50U)) == pdTRUE) {
            sent_length = hosal_uart_send(&s_aiio_ci13162_uart, data, length);
            (void)xSemaphoreGive(s_aiio_ci13162_tx_mutex);
        }
    } else {
        sent_length = hosal_uart_send(&s_aiio_ci13162_uart, data, length);
    }

#if AIIO_CI13162_UART_TRACE_ENABLE == 1
    aiio_ci13162_trace_bytes("TX", (const uint8_t *)data, (size_t)length);
    /* rx_irq/rx_bytes stay 0 when nothing ever arrives on the RX pin. */
    printf("[CI13162][TX] sent=%d rx_irq=%lu rx_bytes=%lu\r\n",
           sent_length,
           (unsigned long)s_aiio_ci13162_callback_count,
           (unsigned long)s_aiio_ci13162_received_bytes);
#endif
    return sent_length;
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

static const char *aiio_ci13162_get_reject_reason(aiio_ci13162_parse_result_t result)
{
    static const char *const reasons[] = {
        "OK",
        "INVALID_ARGUMENT",
        "INVALID_FORMAT",
        "NUMBER_OVERFLOW",
        "UNKNOWN_COMMAND",
        "INVALID_DATA",
    };
    if ((uint32_t)result < (sizeof(reasons) / sizeof(reasons[0]))) {
        return reasons[result];
    }
    return "INTERNAL_ERROR";
}

// Queue an error report to be sent over MQTT cmd 299 in the application task context.
static void aiio_ci13162_queue_error_report(const char *raw_line, const char *reason)
{
    aiio_ci13162_error_event_t err_event;

    if ((s_aiio_ci13162_error_queue == NULL) || (raw_line == NULL) || (reason == NULL)) {
        return;
    }

    memset(&err_event, 0, sizeof(err_event));
    strncpy(err_event.raw, raw_line, sizeof(err_event.raw) - 1U);
    err_event.reason = reason;

    (void)xQueueSend(s_aiio_ci13162_error_queue, &err_event, 0U);
}

#if DEBUG_ENABLE == 1

// Send debug text on the UART0 log (printf), never on the CI13162 UART1.
static void aiio_ci13162_uart_send(const void *data, uint32_t length)
{
    if ((data == NULL) || (length == 0U)) {
        return;
    }

    printf("%.*s", (int)length, (const char *)data);
}

// Send a null-terminated debug string on the UART0 log (printf).
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

// Report the UART1 RX pin, IRQ, FIFO, callback, and stream-buffer state.
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
    uint32_t baud_rate;
    int gpio_level;
    int output_length;

    aiio_ci13162_snapshot_counters(&snapshot);
    callback_delta = snapshot.callback_count - previous_snapshot.callback_count;
    empty_callback_delta = snapshot.empty_callback_count - previous_snapshot.empty_callback_count;
    received_delta = snapshot.received_bytes - previous_snapshot.received_bytes;
    stream_delta = snapshot.stream_bytes - previous_snapshot.stream_bytes;
    dropped_delta = snapshot.dropped_bytes - previous_snapshot.dropped_bytes;
    previous_snapshot = snapshot;

    gpio_function = GLB_GPIO_Get_Fun((GLB_GPIO_Type)AIIO_CI13162_UART_RX_PIN);
    gpio_level = bl_gpio_input_get_value((uint8_t)AIIO_CI13162_UART_RX_PIN);
    fifo_count = UART_GetRxFifoCount(AIIO_CI13162_UART_ID);
    irq_fifo = (UART_GetIntStatus(AIIO_CI13162_UART_ID, UART_INT_RX_FIFO_REQ) == SET) ? 1U : 0U;
    irq_timeout = (UART_GetIntStatus(AIIO_CI13162_UART_ID, UART_INT_RTO) == SET) ? 1U : 0U;
    irq_rx_end = (UART_GetIntStatus(AIIO_CI13162_UART_ID, UART_INT_RX_END) == SET) ? 1U : 0U;
    irq_rx_error = (UART_GetIntStatus(AIIO_CI13162_UART_ID, UART_INT_RX_FER) == SET) ? 1U : 0U;
    overflow = (UART_GetOverflowStatus(AIIO_CI13162_UART_ID, UART_RX_OVERFLOW) == SET) ? 1U : 0U;
    rx_busy = (UART_GetRxBusBusyStatus(AIIO_CI13162_UART_ID) == SET) ? 1U : 0U;
    baud_rate = UART_GetBaudrate(AIIO_CI13162_UART_ID);
    stream_available = xStreamBufferBytesAvailable(s_aiio_ci13162_rx_stream);

    if (gpio_function != AIIO_CI13162_UART_GPIO_FUNCTION) {
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
        "[CI13162][DIAG] cls=%s fun=%u pin=%u lvl=%d "
        "baud=%lu busy=%u fifo=%u "
        "irq=%u/%u/%u/%u cb=%lu(+%lu) empty=%lu(+%lu) rx=%lu(+%lu) "
        "stream_rx=%lu(+%lu) buf=%u/%u drop=%lu(+%lu) ovf=%u\r\n",
        classification,
        (unsigned int)gpio_function,
        (unsigned int)AIIO_CI13162_UART_RX_PIN,
        gpio_level,
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
    char output[80];
    const char *reason = aiio_ci13162_get_reject_reason(result);
    int output_length;

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


// Match a protocol key with optional double quotes without mutating cursor on mismatch.
static uint8_t aiio_ci13162_match_key(const char **cursor, const char *key)
{
    const char *pos;
    size_t key_len;
    uint8_t quoted;

    if ((cursor == NULL) || (*cursor == NULL) || (key == NULL)) {
        return 0U;
    }

    pos = *cursor;
    quoted = (*pos == '"') ? 1U : 0U;
    if (quoted != 0U) {
        pos++;
    }

    key_len = strlen(key);
    if (strncmp(pos, key, key_len) != 0) {
        return 0U;
    }
    pos += key_len;

    if (quoted != 0U) {
        if (*pos != '"') {
            return 0U;
        }
        pos++;
    } else {
        if (((*pos >= 'a') && (*pos <= 'z')) ||
            ((*pos >= 'A') && (*pos <= 'Z')) ||
            ((*pos >= '0') && (*pos <= '9')) ||
            (*pos == '_')) {
            return 0U;
        }
    }

    *cursor = pos;
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

static inline uint8_t aiio_ci13162_is_voice_command(uint8_t command)
{
    return ((command == AIIO_CI13162_CMD_TURN) ||
            (command == AIIO_CI13162_CMD_MODE) ||
            (command == AIIO_CI13162_CMD_TIMER) ||
            (command == AIIO_CI13162_CMD_SCENE) ||
            (command == AIIO_CI13162_CMD_VOLUME_CHANGE)) ? 1U : 0U;
}

// Validate the command-specific data count and range.
static aiio_ci13162_parse_result_t aiio_ci13162_validate_message(
    const aiio_ci13162_message_t *message)
{
    if (message == NULL) {
        return AIIO_CI13162_PARSE_INVALID_ARGUMENT;
    }

    if (aiio_ci13162_is_voice_command(message->command) != 0U) {
        if ((message->cwsl > 14U) && (message->cwsl != 255U)) {
            return AIIO_CI13162_PARSE_INVALID_DATA;
        }
    }

    switch (message->command) {
        case AIIO_CI13162_CMD_RESET:
            return (message->data_count == 0U) ? AIIO_CI13162_PARSE_OK
                                               : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_TURN:
            if (message->data_count != 1U) {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            if (message->data[0] > 1U) {
                return AIIO_CI13162_PARSE_INVALID_DATA;
            }
            if (message->data[0] == 1U) {
                if ((message->cmd_id != 7U) && (message->cmd_id != 8U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else {
                if ((message->cmd_id != 9U) && (message->cmd_id != 10U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            }
            return AIIO_CI13162_PARSE_OK;

        case AIIO_CI13162_CMD_MODE:
            if (message->data_count != 1U) {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            if ((message->data[0] < 1U) || (message->data[0] > 6U)) {
                return AIIO_CI13162_PARSE_INVALID_DATA;
            }
            if (message->data[0] == 1U) {
                if ((message->cmd_id != 11U) && (message->cmd_id != 12U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else if (message->data[0] == 2U) {
                if ((message->cmd_id != 13U) && (message->cmd_id != 14U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else if (message->data[0] == 3U) {
                if ((message->cmd_id != 15U) && (message->cmd_id != 16U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else if (message->data[0] == 4U) {
                if ((message->cmd_id != 17U) && (message->cmd_id != 18U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else if (message->data[0] == 5U) {
                if ((message->cmd_id < 19U) || (message->cmd_id > 21U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else if (message->data[0] == 6U) {
                if ((message->cmd_id < 22U) || (message->cmd_id > 24U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            }
            return AIIO_CI13162_PARSE_OK;

        case AIIO_CI13162_CMD_VERSION:
            return ((message->data_count == 1U) && (message->version > 0U))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_CWSL_STATUS:
            return (message->data_count == 3U) ? AIIO_CI13162_PARSE_OK
                                               : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_CWSL_INFO:
            if (message->data_count < 1U) {
                return AIIO_CI13162_PARSE_INVALID_DATA;
            }
            if (message->data[0] == UINT8_MAX) {
                return (message->data_count == 4U) ? AIIO_CI13162_PARSE_OK
                                                   : AIIO_CI13162_PARSE_INVALID_DATA;
            }
            /* 1..103: every cmd_id of the voice model, home commands too. */
            return ((message->data[0] >= 1U) &&
                    (message->data[0] <= APP_CWSL_HOME_CMD_ID_MAX) &&
                    (message->data_count == 3U))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_RESP_SET:
            return ((message->data_count == 1U) && (message->data[0] <= 1U))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_VOICE_SET:
            return ((message->data_count == 1U) && (message->data[0] <= 1U))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_VOICE_GET:
            return ((message->data_count == 1U) &&
                    (message->data[0] >= 1U) &&
                    (message->data[0] <= AIIO_CI13162_VOICE_ID_MAX))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_TIMER:
            if (message->data_count != 1U) {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            if (message->data[0] > 12U) {
                return AIIO_CI13162_PARSE_INVALID_DATA;
            }
            if (message->data[0] == 0U) {
                if (message->cmd_id != 47U) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else {
                uint8_t min_id = (uint8_t)(23U + (2U * (message->data[0] - 1U)));
                if ((message->cmd_id != min_id) && (message->cmd_id != (min_id + 1U))) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            }
            return AIIO_CI13162_PARSE_OK;

        /* cmd scene */
        case AIIO_CI13162_CMD_SCENE:
            if (message->data_count != 1U) {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            /* Scene 1..15: cmd_id = 55 + data (56..70). */
            if ((message->data[0] >= 1U) && (message->data[0] <= APP_SCENE_COUNT_MAX)) {
                return (message->cmd_id == (uint8_t)(55U + message->data[0]))
                           ? AIIO_CI13162_PARSE_OK
                           : AIIO_CI13162_PARSE_INVALID_DATA;
            }
            /* Home command 71..103: cmd_id = data. */
            if ((message->data[0] >= APP_CWSL_HOME_CMD_ID_MIN) &&
                (message->data[0] <= APP_CWSL_HOME_CMD_ID_MAX)) {
                return (message->cmd_id == message->data[0])
                           ? AIIO_CI13162_PARSE_OK
                           : AIIO_CI13162_PARSE_INVALID_DATA;
            }
            return AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_VOLUME_CHANGE:
            if (message->data_count != 1U) {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            if (message->data[0] > AIIO_CI13162_VOLUME_LEVEL_MAX) {
                return AIIO_CI13162_PARSE_INVALID_DATA;
            }
            if ((message->cmd_id < 48U) || (message->cmd_id > 55U)) {
                return AIIO_CI13162_PARSE_INVALID_DATA;
            }
            if (message->cmd_id == 54U) {
                if (message->data[0] != 0U) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else if ((message->cmd_id == 50U) || (message->cmd_id == 51U)) {
                if (message->data[0] != 1U) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else if ((message->cmd_id == 52U) || (message->cmd_id == 53U)) {
                if (message->data[0] != 7U) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            } else {
                if ((message->data[0] < 1U) || (message->data[0] > 7U)) {
                    return AIIO_CI13162_PARSE_INVALID_DATA;
                }
            }
            return AIIO_CI13162_PARSE_OK;

        case AIIO_CI13162_CMD_VOLUME_INFO:
            return ((message->data_count == 3U) &&
                    (message->data[0] <= AIIO_CI13162_VOLUME_LEVEL_MAX) &&
                    (message->data[1] <= 1U) &&
                    (message->data[2] <= 1U))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_ASR_UNHANDLED:
            return (message->data_count == 1U) ? AIIO_CI13162_PARSE_OK
                                               : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_STATE:
            return ((message->data_count == 1U) && (message->data[0] == 0U))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_BUZZ:
            return ((message->data_count == 0U) ||
                    ((message->data_count == 1U) && (message->data[0] <= 1U)))
                       ? AIIO_CI13162_PARSE_OK
                       : AIIO_CI13162_PARSE_INVALID_DATA;

        case AIIO_CI13162_CMD_CWSL:
        case AIIO_CI13162_CMD_VOLUME:
        case AIIO_CI13162_CMD_VOLUME_GET:
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
    uint8_t has_cmd = 0U;
    uint8_t has_data = 0U;
    uint8_t has_cmd_id = 0U;
    uint8_t has_cwsl = 0U;

    if ((line == NULL) || (message == NULL)) {
        return AIIO_CI13162_PARSE_INVALID_ARGUMENT;
    }

    memset(message, 0, sizeof(*message));
    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor++ != '{') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    while (1) {
        aiio_ci13162_skip_spaces(&cursor);
        if (*cursor == '}') {
            break;
        }

        if (aiio_ci13162_match_key(&cursor, "cmd_id") != 0U) {
            if (has_cmd_id != 0U) {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            aiio_ci13162_skip_spaces(&cursor);
            if (*cursor++ != ':') {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            aiio_ci13162_skip_spaces(&cursor);
            result = aiio_ci13162_parse_uint8(&cursor, &message->cmd_id);
            if (result != AIIO_CI13162_PARSE_OK) {
                return result;
            }
            has_cmd_id = 1U;
        } else if (aiio_ci13162_match_key(&cursor, "cmd") != 0U) {
            if (has_cmd != 0U) {
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
            has_cmd = 1U;
        } else if (aiio_ci13162_match_key(&cursor, "cwsl") != 0U) {
            if (has_cwsl != 0U) {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            aiio_ci13162_skip_spaces(&cursor);
            if (*cursor++ != ':') {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            aiio_ci13162_skip_spaces(&cursor);
            result = aiio_ci13162_parse_uint8(&cursor, &message->cwsl);
            if (result != AIIO_CI13162_PARSE_OK) {
                return result;
            }
            has_cwsl = 1U;
        } else if (aiio_ci13162_match_key(&cursor, "data") != 0U) {
            if (has_data != 0U) {
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
                while (*cursor != ']') {
                    uint32_t val32 = 0U;

                    if (has_cmd != 0U) {
                        if (((message->command <= AIIO_CI13162_CMD_MODE) ||
                             (message->command == AIIO_CI13162_CMD_TIMER) ||
                             (message->command == AIIO_CI13162_CMD_SCENE) ||
                             (message->command == AIIO_CI13162_CMD_VOLUME_CHANGE) ||
                             (message->command == AIIO_CI13162_CMD_STATE)) &&
                            (message->data_count >= 1U)) {
                            return AIIO_CI13162_PARSE_INVALID_FORMAT;
                        }
                    }

                    if (message->data_count >= AIIO_CI13162_CWSL_DATA_MAX) {
                        return AIIO_CI13162_PARSE_INVALID_FORMAT;
                    }

                    result = aiio_ci13162_parse_uint32(&cursor, &val32);
                    if (result != AIIO_CI13162_PARSE_OK) {
                        return result;
                    }

                    if (message->data_count == 0U) {
                        message->version = val32;
                    }
                    if (val32 <= UINT8_MAX) {
                        message->data[message->data_count] = (uint8_t)val32;
                    } else {
                        message->data[message->data_count] = 0U;
                    }
                    message->data_count++;

                    aiio_ci13162_skip_spaces(&cursor);
                    if (*cursor != ',') {
                        break;
                    }
                    cursor++;
                    aiio_ci13162_skip_spaces(&cursor);
                    if (*cursor == ']') {
                        return AIIO_CI13162_PARSE_INVALID_FORMAT;
                    }
                }
            }
            if (*cursor++ != ']') {
                return AIIO_CI13162_PARSE_INVALID_FORMAT;
            }
            has_data = 1U;
        } else {
            return AIIO_CI13162_PARSE_INVALID_FORMAT;
        }

        aiio_ci13162_skip_spaces(&cursor);
        if (*cursor == '}') {
            break;
        }
        if (*cursor++ != ',') {
            return AIIO_CI13162_PARSE_INVALID_FORMAT;
        }
    }

    if (*cursor++ != '}') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }
    aiio_ci13162_skip_spaces(&cursor);
    if (*cursor != '\0') {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    if ((has_cmd == 0U) || (has_data == 0U)) {
        return AIIO_CI13162_PARSE_INVALID_FORMAT;
    }

    if (aiio_ci13162_is_voice_command(message->command) != 0U) {
        if ((has_cmd_id == 0U) || (has_cwsl == 0U)) {
            return AIIO_CI13162_PARSE_INVALID_FORMAT;
        }
        message->has_metadata = 1U;
    } else {
        if ((has_cmd_id != 0U) || (has_cwsl != 0U)) {
            return AIIO_CI13162_PARSE_INVALID_FORMAT;
        }
        message->has_metadata = 0U;
    }

    return aiio_ci13162_validate_message(message);
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
        (s_aiio_ci13162_ota_held != 0U) ||
        (s_aiio_ci13162_version_query_paused != 0U)) {
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

// Ask the chip for the selected voice after boot, until the cache is valid.
static void aiio_ci13162_process_voice_query(void)
{
    TickType_t current_tick;

    if ((s_aiio_ci13162_voice_query_scheduled == 0U) ||
        (s_aiio_ci13162_voice_id != 0U) ||
        (s_aiio_ci13162_voice_query_attempts >= AIIO_CI13162_VOICE_QUERY_MAX_ATTEMPTS) ||
        (s_aiio_ci13162_ota_held != 0U) ||
        (s_aiio_ci13162_version_query_paused != 0U)) {
        return;
    }

    current_tick = xTaskGetTickCount();
    if ((s_aiio_ci13162_voice_query_attempts != 0U) &&
        ((TickType_t)(current_tick - s_aiio_ci13162_voice_query_last_tick) <
         pdMS_TO_TICKS(AIIO_CI13162_VOICE_QUERY_RETRY_MS))) {
        return;
    }

    s_aiio_ci13162_voice_query_last_tick = current_tick;
    s_aiio_ci13162_voice_query_attempts++;
    (void)aiio_ci13162_uart_query_voice();
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
        if ((line != NULL) && (length > 0U)) {
            line[length] = '\0';
            aiio_ci13162_queue_error_report(line, "EMPTY_OR_INVALID_LENGTH");
        }
        return;
    }

    line[length] = '\0';
#if DEBUG_ENABLE == 1
    aiio_ci13162_print_ascii((const uint8_t *)line, length);
    // aiio_ci13162_print_hex((const uint8_t *)line, length);
#endif

    result = aiio_ci13162_parse_line(line, &message);
    if (result != AIIO_CI13162_PARSE_OK) {
#if DEBUG_ENABLE == 1
        aiio_ci13162_report_reject(result);
#endif
        aiio_ci13162_queue_error_report(line, aiio_ci13162_get_reject_reason(result));
        return;
    }

    if ((message.command == AIIO_CI13162_CMD_BUZZ) ||
        (message.command == AIIO_CI13162_CMD_STATE)) {
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
        uint8_t expected_data[AIIO_CI13162_CWSL_DATA_MAX];
        uint32_t expected_version;
        uint8_t expected_cmd_id;
        uint8_t expected_cwsl;
    } test_cases[] = {
        {"{cmd:0,data:[]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_RESET, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{\"cmd\":200,\"data\":[]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_BUZZ, 0U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:200,data:[]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_BUZZ, 0U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{\"cmd\":200,\"data\":[0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_BUZZ, 1U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{\"cmd\":200,\"data\":[1]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_BUZZ, 1U, {1U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{\"cmd\":200,\"data\":[2]}", AIIO_CI13162_PARSE_INVALID_DATA,
         0U, 0U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{\"cmd\":14,\"data\":[0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_STATE, 1U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{\"cmd\":14,\"data\":[1]}", AIIO_CI13162_PARSE_INVALID_DATA,
         0U, 0U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{\"cmd\":14,\"data\":[0],\"cmd_id\":1}", AIIO_CI13162_PARSE_INVALID_FORMAT,
         0U, 0U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[0]}", AIIO_CI13162_PARSE_INVALID_FORMAT,
         0U, 0U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[0],cmd_id:9,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TURN, 1U, {0U, 0U, 0U, 0U}, 0U, 9U, 255U},
        {"{cmd:1,data:[1],cmd_id:7,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TURN, 1U, {1U, 0U, 0U, 0U}, 0U, 7U, 255U},
        {"{cmd:1,data:[1],cmd_id:8,cwsl:0}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TURN, 1U, {1U, 0U, 0U, 0U}, 0U, 8U, 0U},
        {"{cmd:1,data:[1],cmd_id:7,cwsl:15}", AIIO_CI13162_PARSE_INVALID_DATA,
         0U, 0U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[0],cmd_id:7,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA,
         0U, 0U, {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:2,data:[1],cmd_id:11,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_MODE, 1U, {1U, 0U, 0U, 0U}, 0U, 11U, 255U},
        {"{cmd:2,data:[4],cmd_id:17,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_MODE, 1U, {4U, 0U, 0U, 0U}, 0U, 17U, 255U},
        {"{cmd:2,data:[5],cmd_id:19,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_MODE, 1U, {5U, 0U, 0U, 0U}, 0U, 19U, 255U},
        {"{cmd:2,data:[6],cmd_id:22,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_MODE, 1U, {6U, 0U, 0U, 0U}, 0U, 22U, 255U},
        {" { \"cmd\" : 2 , \"data\" : [ 3 ] , \"cmd_id\" : 15 , \"cwsl\" : 255 } ",
         AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_MODE, 1U, {3U, 0U, 0U, 0U}, 0U, 15U, 255U},
        {"{\"cmd\":3,\"data\":[260813]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VERSION, 1U, {0U, 0U, 0U, 0U}, 260813U, 0U, 0U},
        {"{\"cmd\":5,\"data\":[32,7,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_STATUS, 3U, {32U, 7U, 0U, 0U}, 0U, 0U, 0U},
        {" { cmd : 6 , data : [ 7 , 5 , 0 ] } ", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {7U, 5U, 0U, 0U}, 0U, 0U, 0U},
        {"{\"cmd\":6,\"data\":[255,2,6,12]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 4U, {255U, 2U, 6U, 12U}, 0U, 0U, 0U},
        {"{cmd:6,data:[15,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {15U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[39,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {39U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[43,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {43U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[44,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {44U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[55,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {55U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[65,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {65U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[70,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {70U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[71,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {71U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[103,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_CWSL_INFO, 3U, {103U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[104,1,0]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:7,data:[],cmd_id:47,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:7,data:[0],cmd_id:47,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TIMER, 1U, {0U, 0U, 0U, 0U}, 0U, 47U, 255U},
        {"{cmd:7,data:[1],cmd_id:23,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TIMER, 1U, {1U, 0U, 0U, 0U}, 0U, 23U, 255U},
        {"{cmd:7,data:[12],cmd_id:45,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_TIMER, 1U, {12U, 0U, 0U, 0U}, 0U, 45U, 255U},
        {"{cmd:7,data:[13],cmd_id:45,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:7,data:[1,2],cmd_id:23,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:8,data:[0]}", AIIO_CI13162_PARSE_UNKNOWN_COMMAND, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:8,data:[3]}", AIIO_CI13162_PARSE_UNKNOWN_COMMAND, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[3,1]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[7,0]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[0,1]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[3,1,0]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VOLUME_INFO, 3U, {3U, 1U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[0,1,1]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VOLUME_INFO, 3U, {0U, 1U, 1U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[7,0,1]}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VOLUME_INFO, 3U, {7U, 0U, 1U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[3,1,2]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[8,1,0]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[3,2,0]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[3]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:9,data:[3,1,0,0]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:10,data:[1]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_ASR_UNHANDLED, 1U,
         {1U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:10,data:[40]}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_ASR_UNHANDLED, 1U,
         {40U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:10,data:[]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:10,data:[1,2]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:11,data:[]}", AIIO_CI13162_PARSE_UNKNOWN_COMMAND, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:12,data:[],cmd_id:56,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:12,data:[1],cmd_id:56,cwsl:255}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_SCENE, 1U,
         {1U, 0U, 0U, 0U}, 0U, 56U, 255U},
        {"{cmd:12,data:[10],cmd_id:65,cwsl:255}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_SCENE, 1U,
         {10U, 0U, 0U, 0U}, 0U, 65U, 255U},
        {"{cmd:12,data:[15],cmd_id:70,cwsl:255}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_SCENE, 1U,
         {15U, 0U, 0U, 0U}, 0U, 70U, 255U},
        {"{cmd:12,data:[0],cmd_id:56,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:12,data:[11],cmd_id:66,cwsl:255}", AIIO_CI13162_PARSE_OK, AIIO_CI13162_CMD_SCENE, 1U,
         {11U, 0U, 0U, 0U}, 0U, 66U, 255U},
        {"{cmd:12,data:[16],cmd_id:71,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:12,data:[1,2],cmd_id:56,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:13,data:[1],cmd_id:50,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VOLUME_CHANGE, 1U, {1U, 0U, 0U, 0U}, 0U, 50U, 255U},
        {"{cmd:13,data:[4],cmd_id:48,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VOLUME_CHANGE, 1U, {4U, 0U, 0U, 0U}, 0U, 48U, 255U},
        {"{cmd:13,data:[7],cmd_id:52,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VOLUME_CHANGE, 1U, {7U, 0U, 0U, 0U}, 0U, 52U, 255U},
        {"{cmd:13,data:[0],cmd_id:54,cwsl:255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VOLUME_CHANGE, 1U, {0U, 0U, 0U, 0U}, 0U, 54U, 255U},
        {"{\"cmd\":13,\"data\":[0],\"cmd_id\":54,\"cwsl\":255}", AIIO_CI13162_PARSE_OK,
         AIIO_CI13162_CMD_VOLUME_CHANGE, 1U, {0U, 0U, 0U, 0U}, 0U, 54U, 255U},
        {"{cmd:13,data:[8],cmd_id:48,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:13,data:[],cmd_id:48,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:13,data:[3,1],cmd_id:48,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:3,data:[]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:3,data:[1,2]}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:0,data:[0]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[1,0],cmd_id:7,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[2],cmd_id:7,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:2,data:[7],cmd_id:11,cwsl:255}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:4,data:[1,7,255]}", AIIO_CI13162_PARSE_UNKNOWN_COMMAND, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:5,data:[32,7]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[255,2,6]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[7,5,0,1]}", AIIO_CI13162_PARSE_INVALID_DATA, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:6,data:[255,2,6,12,0]}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[-1],cmd_id:7,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:5,data:[32,300,0]}", AIIO_CI13162_PARSE_NUMBER_OVERFLOW, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:256,data:[]}", AIIO_CI13162_PARSE_NUMBER_OVERFLOW, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[1],cmd_id:7,cwsl:255,x:1}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,cmd:1,data:[1],cmd_id:7,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[1],data:[1],cmd_id:7,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[1],cmd_id:7,cmd_id:7,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
        {"{cmd:1,data:[1],cmd_id:7,cwsl:255,cwsl:255}", AIIO_CI13162_PARSE_INVALID_FORMAT, 0U, 0U,
         {0U, 0U, 0U, 0U}, 0U, 0U, 0U},
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
              (memcmp(message.data,
                      test_cases[i].expected_data,
                      sizeof(message.data)) != 0) ||
              (message.version != test_cases[i].expected_version) ||
              (message.cmd_id != test_cases[i].expected_cmd_id) ||
              (message.cwsl != test_cases[i].expected_cwsl)))) {
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
    static const uint8_t fragmented_part_1[] = "{\"cmd\":1,\"cmd_id\":7,\"cwsl\":255,\"da";
    static const uint8_t fragmented_part_2[] = "ta:[1]}\r\n";
    static const uint8_t coalesced_messages[] =
        "{\"cmd\":5,\"data\":[32,7,0]}\r\n{\"cmd\":6,\"data\":[7,5,0]}\r\n";
    static const uint8_t stray_cr_message[] = "\r{\"cmd\":2,\"cmd_id\":17,\"cwsl\":255,\"data\":[4]}\r\n";
    static const uint8_t overflow_message[] =
        "{\"cmd\":5,\"data\":[32,7,0],\"padding\":1234567890123456789012345678901234567890}"
        "\r\n{\"cmd\":1,\"cmd_id\":7,\"cwsl\":255,\"data\":[1]}\r\n";
    static const uint8_t embedded_null_message[] = {
        '{', '"', 'c', 'm', 'd', '"', ':', '1', ',', '"', 'c', 'm', 'd', '_', 'i', 'd', '"', ':', '7', ',', '"', 'c', 'w', 's', 'l', '"', ':', '2', '5', '5', ',', '"', 'd', 'a', 't', 'a', '"', ':', '[', '1', ']', '}',
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
        (message.data[0] != 1U) ||
        (message.cmd_id != 7U) ||
        (message.cwsl != 255U)) {
        failed_count++;
    }

    memset(&assembler, 0, sizeof(assembler));
    for (i = 0U; i < sizeof(coalesced_messages) - 1U; i++) {
        aiio_ci13162_process_byte(&assembler, coalesced_messages[i]);
    }
    if ((xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) != pdPASS) ||
        (message.command != AIIO_CI13162_CMD_CWSL_STATUS) ||
        (message.data_count != 3U) ||
        (message.data[0] != 32U) ||
        (message.data[1] != 7U) ||
        (message.data[2] != 0U) ||
        (xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) != pdPASS) ||
        (message.command != AIIO_CI13162_CMD_CWSL_INFO) ||
        (message.data_count != 3U) ||
        (message.data[0] != 7U) ||
        (message.data[1] != 5U) ||
        (message.data[2] != 0U)) {
        failed_count++;
    }

    memset(&assembler, 0, sizeof(assembler));
    for (i = 0U; i < sizeof(stray_cr_message) - 1U; i++) {
        aiio_ci13162_process_byte(&assembler, stray_cr_message[i]);
    }
    if ((xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) != pdPASS) ||
        (message.command != AIIO_CI13162_CMD_MODE) ||
        (message.data[0] != 4U) ||
        (message.cmd_id != 17U) ||
        (message.cwsl != 255U)) {
        failed_count++;
    }

    memset(&assembler, 0, sizeof(assembler));
    for (i = 0U; i < sizeof(overflow_message) - 1U; i++) {
        aiio_ci13162_process_byte(&assembler, overflow_message[i]);
    }
    if ((xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) != pdPASS) ||
        (message.command != AIIO_CI13162_CMD_TURN) ||
        (message.data[0] != 1U) ||
        (message.cmd_id != 7U) ||
        (message.cwsl != 255U) ||
        (xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) == pdPASS)) {
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

    received_length = hosal_uart_receive(&s_aiio_ci13162_uart, input, sizeof(input));
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

// #if AIIO_CI13162_UART_TRACE_ENABLE == 1
//         aiio_ci13162_trace_bytes("RX", input, received_length);
// #endif

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
#endif
}

int32_t aiio_ci13162_power_init(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    if (!hw_device_support_voice()) {
        return -1;
    }

    bl_gpio_output_set((uint8_t)AIIO_CI13162_POWER_PIN, 1U);
    bl_gpio_enable_output((uint8_t)AIIO_CI13162_POWER_PIN, 1U, 0U);
    bl_gpio_output_set((uint8_t)AIIO_CI13162_POWER_PIN, 1U);
    return 0;
#else
    return -1;
#endif
}

int32_t aiio_ci13162_power_cycle(uint32_t off_duration_ms, uint32_t stabilize_delay_ms)
{
#if AIIO_CI13162_UART_ENABLE == 1
    uint32_t remaining;

    if (!hw_device_support_voice()) {
        return -1;
    }

    /* Drive power pin LOW (power OFF) */
    bl_gpio_output_set((uint8_t)AIIO_CI13162_POWER_PIN, 0U);
    bl_gpio_enable_output((uint8_t)AIIO_CI13162_POWER_PIN, 1U, 0U);
    bl_gpio_output_set((uint8_t)AIIO_CI13162_POWER_PIN, 0U);

    remaining = off_duration_ms;
    while (remaining > 0U) {
        uint32_t step = (remaining > 100U) ? 100U : remaining;
        bl_wdt_feed();
        vTaskDelay(pdMS_TO_TICKS(step));
        remaining -= step;
    }
    bl_wdt_feed();

    /* Drive power pin HIGH (power ON) */
    bl_gpio_output_set((uint8_t)AIIO_CI13162_POWER_PIN, 1U);

    if (stabilize_delay_ms > 0U) {
        vTaskDelay(pdMS_TO_TICKS(stabilize_delay_ms));
    }
    bl_wdt_feed();

    return 0;
#else
    (void)off_duration_ms;
    (void)stabilize_delay_ms;
    return -1;
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

    s_aiio_ci13162_error_queue = xQueueCreateStatic(
        AIIO_CI13162_UART_ERROR_QUEUE_LENGTH,
        sizeof(aiio_ci13162_error_event_t),
        s_aiio_ci13162_error_queue_storage,
        &s_aiio_ci13162_error_queue_control);
    if (s_aiio_ci13162_error_queue == NULL) {
        return -1;
    }

    s_aiio_ci13162_tx_mutex = xSemaphoreCreateMutexStatic(
        &s_aiio_ci13162_tx_mutex_control);
    if (s_aiio_ci13162_tx_mutex == NULL) {
        return -1;
    }

    // Pin is already GPIO idle-high. Do not finalize first: UART_Disable
    // while GPIO4 is still UART AF can drop TX and look like a BREAK.
    vTaskDelay(pdMS_TO_TICKS(AIIO_CI13162_UART_TX_IDLE_HOLD_MS));

    s_aiio_ci13162_uart.config.uart_id = AIIO_CI13162_UART_ID;
    s_aiio_ci13162_uart.config.tx_pin = AIIO_CI13162_UART_TX_PIN;
    s_aiio_ci13162_uart.config.rx_pin = AIIO_CI13162_UART_RX_PIN;
    s_aiio_ci13162_uart.config.cts_pin = AIIO_CI13162_UART_INVALID_PIN;
    s_aiio_ci13162_uart.config.rts_pin = AIIO_CI13162_UART_INVALID_PIN;
    s_aiio_ci13162_uart.config.baud_rate = AIIO_CI13162_UART_BAUD_RATE;
    s_aiio_ci13162_uart.config.data_width = HOSAL_DATA_WIDTH_8BIT;
    s_aiio_ci13162_uart.config.parity = HOSAL_NO_PARITY;
    s_aiio_ci13162_uart.config.stop_bits = HOSAL_STOP_BITS_1;
    s_aiio_ci13162_uart.config.flow_control = HOSAL_FLOW_CONTROL_DISABLED;
    s_aiio_ci13162_uart.config.mode = HOSAL_UART_MODE_INT;
    s_aiio_ci13162_uart.rx_cb = aiio_ci13162_rx_callback;
    s_aiio_ci13162_uart.p_rxarg = NULL;

    if (hosal_uart_init(&s_aiio_ci13162_uart) != 0) {
        return -1;
    }

    /* The TX-only log init (bfl_main: hosal_uart_init_only_tx) never claims UART
     * signal 7, so it keeps its reset value UART1_RXD (GLB_UART_SIG_SEL_0 reset
     * 0x76543210). hosal_uart_init also selects UART1_RXD on SIG(rx_pin % 8), so
     * UART1_RXD would be selected twice and UART1 no longer listens only to the
     * RX pin. Give SIG7 back to UART0_RXD (log RX pin GPIO7), as the old UART0
     * driver did. */
    if ((AIIO_CI13162_UART_RX_PIN % 8U) != 7U) {
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
        hosal_uart_finalize(&s_aiio_ci13162_uart);
        return -1;
    }

    s_aiio_ci13162_voice_version = 0U;
    s_aiio_ci13162_voice_version_valid = 0U;
    s_aiio_ci13162_version_query_attempts = 0U;
    s_aiio_ci13162_version_last_query_tick = 0U;
    s_aiio_ci13162_version_start_tick = xTaskGetTickCount();
    s_aiio_ci13162_version_query_scheduled = 1U;
    s_aiio_ci13162_version_query_paused = 0U;
    s_aiio_ci13162_volume = 0U;
    s_aiio_ci13162_volume_valid = 0U;
    s_aiio_ci13162_volume_muted = 0U;
    s_aiio_ci13162_voice_id = 0U;
    s_aiio_ci13162_voice_set_sent = 0U;
    s_aiio_ci13162_voice_query_scheduled = 0U;
    s_aiio_ci13162_voice_query_attempts = 0U;
    s_aiio_ci13162_voice_query_last_tick = 0U;

#if DEBUG_ENABLE == 1
    {
        char output[80];
        int output_length;

        output_length = snprintf(output,
                                 sizeof(output),
                                 "[CI13162] UART%u TX GPIO%u RX GPIO%u, %lu 8N1 ready\r\n",
                                 (unsigned int)AIIO_CI13162_UART_ID,
                                 (unsigned int)AIIO_CI13162_UART_TX_PIN,
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

static void aiio_ci13162_publish_error_mqtt(const char *raw_line, const char *reason)
{
    char escaped[AIIO_CI13162_UART_LINE_MAX * 2U];
    char payload[256];
    size_t e_idx = 0U;
    int len;

    if ((raw_line == NULL) || (reason == NULL)) {
        return;
    }

    for (size_t i = 0U; (raw_line[i] != '\0') && (e_idx < (sizeof(escaped) - 2U)); i++) {
        if ((raw_line[i] == '"') || (raw_line[i] == '\\')) {
            escaped[e_idx++] = '\\';
        }
        escaped[e_idx++] = raw_line[i];
    }
    escaped[e_idx] = '\0';

    len = snprintf(payload, sizeof(payload),
                   "{\"%s\":299,\"raw\":\"%s\",\"reason\":\"%s\"}",
                   gs_dev_type[device_type], escaped, reason);
    if ((len > 0) && ((size_t)len < sizeof(payload))) {
        os_trace("[CI13162][MQTT_ERR] Publish cmd 299: %s\r\n", payload);
        app_send_data(payload, (uint32_t)len);
    }
}

void aiio_ci13162_uart_process(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    aiio_ci13162_message_t message;
    m_fan_info_t *fan_info;
    uint8_t base_speed;

    if (s_aiio_ci13162_ota_held != 0U) {
        return;
    }

    aiio_ci13162_error_event_t err_event;
    while ((s_aiio_ci13162_error_queue != NULL) &&
           (xQueueReceive(s_aiio_ci13162_error_queue, &err_event, 0U) == pdPASS)) {
        aiio_ci13162_publish_error_mqtt(err_event.raw, err_event.reason);
    }

    if (s_aiio_ci13162_command_queue == NULL) {
        return;
    }

    while (xQueueReceive(s_aiio_ci13162_command_queue, &message, 0U) == pdPASS) {
        if (message.command == AIIO_CI13162_CMD_VERSION) {
            s_aiio_ci13162_voice_version = message.version;
            s_aiio_ci13162_voice_version_valid = 1U;
            s_aiio_ci13162_version_query_scheduled = 0U;
            if (s_aiio_ci13162_voice_id == 0U) {
                s_aiio_ci13162_voice_query_scheduled = 1U;
                s_aiio_ci13162_voice_query_attempts = 0U;
            }
            os_trace("[CI13162] version=%lu\r\n", (unsigned long)message.version);
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_CWSL_STATUS) {
            if (s_aiio_ci13162_cwsl_status_cb != NULL) {
                s_aiio_ci13162_cwsl_status_cb(message.data[0],
                                              message.data[1],
                                              message.data[2]);
            }
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_CWSL_INFO) {
            if (s_aiio_ci13162_cwsl_info_cb != NULL) {
                s_aiio_ci13162_cwsl_info_cb(message.data[0],
                                            &message.data[1],
                                            (uint8_t)(message.data_count - 1U));
            }
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_RESP_SET) {
            if (s_aiio_ci13162_resp_set_cb != NULL) {
                s_aiio_ci13162_resp_set_cb(message.data[0]);
            }
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_VOICE_SET) {
            if ((message.data[0] == 1U) && (s_aiio_ci13162_voice_set_sent != 0U)) {
                s_aiio_ci13162_voice_id = s_aiio_ci13162_voice_set_sent;
            }
            s_aiio_ci13162_voice_set_sent = 0U;
            if (s_aiio_ci13162_voice_set_cb != NULL) {
                s_aiio_ci13162_voice_set_cb(message.data[0]);
            }
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_VOICE_GET) {
            s_aiio_ci13162_voice_id = message.data[0];
            if (s_aiio_ci13162_voice_get_cb != NULL) {
                s_aiio_ci13162_voice_get_cb(message.data[0]);
            }
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_VOLUME_INFO) {
            s_aiio_ci13162_volume = message.data[0];
            s_aiio_ci13162_volume_muted = message.data[2];
            s_aiio_ci13162_volume_valid = 1U;
            if (s_aiio_ci13162_volume_info_cb != NULL) {
                s_aiio_ci13162_volume_info_cb(message.data[0], message.data[1],
                                              message.data[2]);
            }
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_VOLUME_CHANGE) {
            s_aiio_ci13162_volume = message.data[0];
            s_aiio_ci13162_volume_muted = (message.data[0] == 0U) ? 1U : 0U;
            s_aiio_ci13162_volume_valid = 1U;
            os_trace("[CI13162] volume change=%u cmd_id=%u cwsl=%u\r\n",
                     (unsigned int)message.data[0],
                     (unsigned int)message.cmd_id,
                     (unsigned int)message.cwsl);
            app_cwsl_notify_voice_cmd(message.cmd_id, message.cwsl);
            app_volume_report_voice(message.data[0], message.cmd_id, message.cwsl);
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_ASR_UNHANDLED) {
            os_trace("[CI13162] asr unhandled cmd_id=%u\r\n",
                     (unsigned int)message.data[0]);
            continue;
        }

        if (message.command == AIIO_CI13162_CMD_STATE) {
            continue;
        }

        if (device_type == VOICEBOX) {
            if (message.command == AIIO_CI13162_CMD_SCENE ||
                message.command == AIIO_CI13162_CMD_TURN ||
                message.command == AIIO_CI13162_CMD_MODE ||
                message.command == AIIO_CI13162_CMD_TIMER) {
                os_trace("[CI13162] voicebox cmd=0x%02X cmd_id=%u\r\n",
                         (unsigned int)message.command,
                         (unsigned int)message.cmd_id);
                app_cwsl_notify_voice_cmd(message.cmd_id, message.cwsl);
            } else if (message.command == AIIO_CI13162_CMD_RESET) {
#if DEBUG_ENABLE == 1
                os_trace("[CI13162] software reset\r\n");
#endif
                bl_sys_reset_por();
            }
            continue;
        }

        fan_info = m_fan_get_instance();
        if (fan_info == NULL) {
            return;
        }

        uint8_t prev_prevent_buzz = prevent_buzz;
        prevent_buzz = 1U;

        switch (message.command) {
            case AIIO_CI13162_CMD_RESET:
#if DEBUG_ENABLE == 1
                os_trace("[CI13162] software reset\r\n");
#endif
                bl_sys_reset_por();
                break;

            case AIIO_CI13162_CMD_TURN:
                if (message.data[0] == 0U) {
                    if (fan_info->mode != M_FAN_MODE_OFF) {
                        if (fan_info->time_set > 0U) {
                            m_fan_set_time_off_voice(0U, message.cmd_id, message.cwsl);
                        }
                        m_fan_set_speed(M_FAN_MODE_OFF, M_FAN_SPEED_OFF);
                    }
                } else if (fan_info->mode == M_FAN_MODE_OFF) {
                    m_fan_set_speed(M_FAN_MODE_ON, M_FAN_SPEED_1);
                }
                app_fan_respond_control_voice(fan_info->mode,
                                              fan_info->speed,
                                              fan_info->pcn,
                                              fan_info->swing,
                                              fan_info->light,
                                              0U,
                                              SRC_ACTION_VOICE,
                                              message.cmd_id,
                                              message.cwsl);
                app_cwsl_notify_voice_cmd(message.cmd_id, message.cwsl);
                break;

            case AIIO_CI13162_CMD_MODE:
                if (message.data[0] <= M_FAN_SPEED_3) {
                    m_fan_set_speed(M_FAN_MODE_ON, message.data[0]);
                } else if (message.data[0] == 4U) {
                    base_speed = fan_info->speed;
                    if ((base_speed < M_FAN_SPEED_1) || (base_speed > M_FAN_SPEED_3)) {
                        base_speed = M_FAN_SPEED_1;
                    }
                    m_fan_set_speed(M_FAN_MODE_NATURAL, base_speed);
                } else {
                    m_fan_set_swing((message.data[0] == 5U)
                                        ? M_FAN_SWING_ON
                                        : M_FAN_SWING_OFF);
                }
                app_fan_respond_control_voice(fan_info->mode,
                                              fan_info->speed,
                                              fan_info->pcn,
                                              fan_info->swing,
                                              fan_info->light,
                                              0U,
                                              SRC_ACTION_VOICE,
                                              message.cmd_id,
                                              message.cwsl);
                app_cwsl_notify_voice_cmd(message.cmd_id, message.cwsl);
                break;

            case AIIO_CI13162_CMD_TIMER:
                m_fan_set_time_off_voice((uint32_t)message.data[0] * 3600U,
                                         message.cmd_id,
                                         message.cwsl);
                app_cwsl_notify_voice_cmd(message.cmd_id, message.cwsl);
                break;

            case AIIO_CI13162_CMD_SCENE:
                os_trace("[CI13162] scene=%u\r\n",
                         (unsigned int)message.data[0]);
                app_cwsl_notify_voice_cmd(message.cmd_id, message.cwsl);
                break;

            default:
                break;
        }

        prevent_buzz = prev_prevent_buzz;
    }

#endif
}

void aiio_ci13162_uart_poll(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    aiio_ci13162_process_version_query();
    aiio_ci13162_process_voice_query();
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

    (void)xQueueReset(s_aiio_ci13162_command_queue);
    (void)xStreamBufferReset(s_aiio_ci13162_ota_rx_stream);
    while (hosal_uart_receive(&s_aiio_ci13162_uart, input, sizeof(input)) > 0) {
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

    (void)hosal_uart_ioctl(&s_aiio_ci13162_uart, HOSAL_UART_FLUSH, NULL);
    (void)hosal_uart_ioctl(&s_aiio_ci13162_uart,
                           HOSAL_UART_BAUD_SET,
                           (void *)(uintptr_t)AIIO_CI13162_UART_BAUD_RATE);
    aiio_ci13162_uart_ota_flush_rx();
    (void)xQueueReset(s_aiio_ci13162_command_queue);

    taskENTER_CRITICAL();
    s_aiio_ci13162_ota_held = 0U;
    taskEXIT_CRITICAL();
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

    if (hosal_uart_ioctl(&s_aiio_ci13162_uart, HOSAL_UART_FLUSH, NULL) != 0) {
        return -1;
    }
    if (hosal_uart_ioctl(&s_aiio_ci13162_uart,
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

    sent_length = hosal_uart_send(&s_aiio_ci13162_uart, data, (uint32_t)length);
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
    while (hosal_uart_receive(&s_aiio_ci13162_uart, input, sizeof(input)) > 0) {
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

int32_t aiio_ci13162_uart_send_cwsl(uint8_t action,
                                    const uint8_t *data,
                                    uint8_t data_count)
{
#if AIIO_CI13162_UART_ENABLE == 1
    char request[48];
    int request_length;
    int sent_length;

    if ((s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    switch (action) {
        case 1U:
            if ((data == NULL) || ((data_count != 2U) && (data_count != 3U))) {
                return -1;
            }
            if (data_count == 3U) {
                request_length = snprintf(request,
                                          sizeof(request),
                                          "{\"cmd\":4,\"data\":[%u,%u,%u,%u]}\r\n",
                                          (unsigned int)action,
                                          (unsigned int)data[0],
                                          (unsigned int)data[1],
                                          (unsigned int)data[2]);
                break;
            }
            request_length = snprintf(request,
                                      sizeof(request),
                                      "{\"cmd\":4,\"data\":[%u,%u,%u]}\r\n",
                                      (unsigned int)action,
                                      (unsigned int)data[0],
                                      (unsigned int)data[1]);
            break;

        case 2U:
            if ((data == NULL) || (data_count != 2U)) {
                return -1;
            }
            request_length = snprintf(request,
                                      sizeof(request),
                                      "{\"cmd\":4,\"data\":[%u,%u,%u]}\r\n",
                                      (unsigned int)action,
                                      (unsigned int)data[0],
                                      (unsigned int)data[1]);
            break;

        case 4U:
            if ((data != NULL) || (data_count != 0U)) {
                return -1;
            }
            request_length = snprintf(request,
                                      sizeof(request),
                                      "{\"cmd\":4,\"data\":[4]}\r\n");
            break;

        case 6U:
            if ((data == NULL) || (data_count != 1U)) {
                return -1;
            }
            request_length = snprintf(request,
                                      sizeof(request),
                                      "{\"cmd\":4,\"data\":[6,%u]}\r\n",
                                      (unsigned int)data[0]);
            break;

        default:
            return -1;
    }

    if ((request_length <= 0) || ((size_t)request_length >= sizeof(request))) {
        return -1;
    }

    sent_length = aiio_ci13162_uart_tx(request,
                                       (uint32_t)request_length);
    return (sent_length == request_length) ? 0 : -1;
#else
    (void)action;
    (void)data;
    (void)data_count;
    return -1;
#endif
}

void aiio_ci13162_uart_set_cwsl_callbacks(
    aiio_ci13162_cwsl_status_cb_t status_cb,
    aiio_ci13162_cwsl_info_cb_t info_cb)
{
#if AIIO_CI13162_UART_ENABLE == 1
    s_aiio_ci13162_cwsl_status_cb = status_cb;
    s_aiio_ci13162_cwsl_info_cb = info_cb;
#else
    (void)status_cb;
    (void)info_cb;
#endif
}

int32_t aiio_ci13162_uart_send_resp_set(uint8_t cmd_id, uint8_t value)
{
#if AIIO_CI13162_UART_ENABLE == 1
    char request[40];
    int request_length;
    int sent_length;

    if ((s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    request_length = snprintf(request,
                              sizeof(request),
                              "{\"cmd\":15,\"data\":[%u,%u]}\r\n",
                              (unsigned int)cmd_id,
                              (unsigned int)value);
    if ((request_length <= 0) || ((size_t)request_length >= sizeof(request))) {
        return -1;
    }

    sent_length = aiio_ci13162_uart_tx(request,
                                       (uint32_t)request_length);
    return (sent_length == request_length) ? 0 : -1;
#else
    (void)cmd_id;
    (void)value;
    return -1;
#endif
}

void aiio_ci13162_uart_set_resp_set_callback(aiio_ci13162_resp_set_cb_t cb)
{
#if AIIO_CI13162_UART_ENABLE == 1
    s_aiio_ci13162_resp_set_cb = cb;
#else
    (void)cb;
#endif
}

int32_t aiio_ci13162_uart_send_voice_set(uint8_t voice_id)
{
#if AIIO_CI13162_UART_ENABLE == 1
    char request[40];
    int request_length;
    int sent_length;

    if ((voice_id < 1U) ||
        (voice_id > AIIO_CI13162_VOICE_ID_MAX) ||
        (s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    request_length = snprintf(request,
                              sizeof(request),
                              "{\"cmd\":17,\"data\":[%u]}\r\n",
                              (unsigned int)voice_id);
    if ((request_length <= 0) || ((size_t)request_length >= sizeof(request))) {
        return -1;
    }

    sent_length = aiio_ci13162_uart_tx(request,
                                       (uint32_t)request_length);
    if (sent_length != request_length) {
        return -1;
    }

    s_aiio_ci13162_voice_set_sent = voice_id;
    return 0;
#else
    (void)voice_id;
    return -1;
#endif
}

int32_t aiio_ci13162_uart_query_voice(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    static const char query[] = "{\"cmd\":18,\"data\":[]}\r\n";
    int sent_length;
    uint32_t query_length;

    if ((s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    query_length = (uint32_t)(sizeof(query) - 1U);
    sent_length = aiio_ci13162_uart_tx(query, query_length);
    return (sent_length == (int)query_length) ? 0 : -1;
#else
    return -1;
#endif
}

void aiio_ci13162_uart_set_voice_callbacks(aiio_ci13162_voice_set_cb_t set_cb,
                                           aiio_ci13162_voice_get_cb_t get_cb)
{
#if AIIO_CI13162_UART_ENABLE == 1
    s_aiio_ci13162_voice_set_cb = set_cb;
    s_aiio_ci13162_voice_get_cb = get_cb;
#else
    (void)set_cb;
    (void)get_cb;
#endif
}

uint8_t aiio_ci13162_uart_get_voice_id(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    return s_aiio_ci13162_voice_id;
#else
    return 0U;
#endif
}

int32_t aiio_ci13162_uart_send_volume(uint8_t value)
{
#if AIIO_CI13162_UART_ENABLE == 1
    char request[40];
    int request_length;
    int sent_length;

    if ((value < 1U) ||
        (value > 7U) ||
        (s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    request_length = snprintf(request,
                              sizeof(request),
                              "{\"cmd\":8,\"data\":[%u]}\r\n",
                              (unsigned int)value);
    if ((request_length <= 0) || ((size_t)request_length >= sizeof(request))) {
        return -1;
    }

    sent_length = aiio_ci13162_uart_tx(request,
                                       (uint32_t)request_length);
    return (sent_length == request_length) ? 0 : -1;
#else
    (void)value;
    return -1;
#endif
}

int32_t aiio_ci13162_uart_query_volume(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    int sent_length;
    uint32_t query_length;

    if ((s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    query_length = (uint32_t)(sizeof(s_aiio_ci13162_volume_query) - 1U);
    sent_length = aiio_ci13162_uart_tx(s_aiio_ci13162_volume_query,
                                       query_length);
    return (sent_length == (int)query_length) ? 0 : -1;
#else
    return -1;
#endif
}

int32_t aiio_ci13162_uart_send_mute(void)
{
#if AIIO_CI13162_UART_ENABLE == 1
    int sent_length;
    uint32_t request_length;

    if ((s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    request_length = (uint32_t)(sizeof(s_aiio_ci13162_volume_mute) - 1U);
    sent_length = aiio_ci13162_uart_tx(s_aiio_ci13162_volume_mute,
                                       request_length);
    return (sent_length == (int)request_length) ? 0 : -1;
#else
    return -1;
#endif
}

void aiio_ci13162_uart_set_volume_callback(
    aiio_ci13162_volume_info_cb_t callback)
{
#if AIIO_CI13162_UART_ENABLE == 1
    s_aiio_ci13162_volume_info_cb = callback;
#else
    (void)callback;
#endif
}

int32_t aiio_ci13162_uart_get_volume_state(uint8_t *level, uint8_t *muted)
{
#if AIIO_CI13162_UART_ENABLE == 1
    if ((level == NULL) || (muted == NULL) ||
        (s_aiio_ci13162_volume_valid == 0U)) {
        return -1;
    }

    *level = s_aiio_ci13162_volume;
    *muted = s_aiio_ci13162_volume_muted;
    return 0;
#else
    (void)level;
    (void)muted;
    return -1;
#endif
}

int32_t aiio_ci13162_uart_get_volume(uint8_t *level)
{
#if AIIO_CI13162_UART_ENABLE == 1
    if ((level == NULL) || (s_aiio_ci13162_volume_valid == 0U)) {
        return -1;
    }

    *level = s_aiio_ci13162_volume;
    return 0;
#else
    (void)level;
    return -1;
#endif
}

void aiio_ci13162_uart_set_version_query_paused(uint8_t paused)
{
#if AIIO_CI13162_UART_ENABLE == 1
    s_aiio_ci13162_version_query_paused = (paused != 0U) ? 1U : 0U;
#else
    (void)paused;
#endif
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
    sent_length = aiio_ci13162_uart_tx(s_aiio_ci13162_version_query,
                                       query_length);
    return (sent_length == (int)query_length) ? 0 : -1;
#else
    return -1;
#endif
}

int32_t aiio_ci13162_uart_send_buzz_type(uint8_t is_long)
{
#if AIIO_CI13162_UART_ENABLE == 1
    static const char s_buzz_short[] = "{\"cmd\":200,\"data\":[0]}\r\n";
    static const char s_buzz_long[] = "{\"cmd\":200,\"data\":[1]}\r\n";
    const char *msg;
    uint32_t message_length;
    int sent_length;

    if ((s_aiio_ci13162_command_queue == NULL) ||
        (s_aiio_ci13162_ota_held != 0U)) {
        return -1;
    }

    if (is_long != 0U) {
        msg = s_buzz_long;
        message_length = (uint32_t)(sizeof(s_buzz_long) - 1U);
    } else {
        msg = s_buzz_short;
        message_length = (uint32_t)(sizeof(s_buzz_short) - 1U);
    }

    sent_length = aiio_ci13162_uart_tx(msg, message_length);
    return (sent_length == (int)message_length) ? 0 : -1;
#else
    (void)is_long;
    return -1;
#endif
}

int32_t aiio_ci13162_uart_send_buzz(void)
{
    return aiio_ci13162_uart_send_buzz_type(0U);
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
