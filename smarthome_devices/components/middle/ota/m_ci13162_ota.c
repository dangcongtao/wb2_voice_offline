#include "m_ci13162_ota.h"
#include "m_ci13162_ota_boot.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include "aiio_ci13162_uart.h"
#include "app_config.h"
#include "bl_sys.h"
#include "bl_wdt.h"
#include "ci13162_ota_fw2.h"
#include "ci13162_ota_http.h"
#include "ci13162_ota_protocol.h"
#include "IRrecv.h"
#include "m_ble.h"
#include "m_mqtt.h"
#include "system_typedef.h"

bool m_ci13162_ota_url_is_valid(const char *url)
{
    size_t length;

    if (url == NULL) {
        return false;
    }
    length = strlen(url);
    if ((length == 0U) || (length >= AIIO_CI13162_OTA_URL_MAX)) {
        return false;
    }
#if AIIO_CI13162_OTA_ENABLE_HTTPS == 1
    if ((strncmp(url, "http://", 7U) != 0) &&
        (strncmp(url, "https://", 8U) != 0)) {
        return false;
    }
#else
    if (strncmp(url, "http://", 7U) != 0) {
        return false;
    }
#endif
    return true;
}

#if AIIO_CI13162_OTA_ENABLE == 1

/* Stack depth is in StackType_t words. Keep 16 KiB even without HTTPS:
 * getaddrinfo() and the HTTP path still need headroom on this task. */
#define CI13162_OTA_TASK_STACK_DEPTH       (1024U * 4U)
#define CI13162_OTA_TASK_PRIORITY          12U
#define CI13162_OTA_ALLOC_RETRY_COUNT      6U
#define CI13162_OTA_ALLOC_RETRY_DELAY_MS   500U
#define CI13162_OTA_AGENT_ADDR             0x1FF80000U
#define CI13162_OTA_FW_BASE                0x00000000U
#define CI13162_OTA_HANDSHAKE_PERIOD_MS    25U
#define CI13162_OTA_READ_SLICE_MS          20U
#define CI13162_OTA_ACK_TIMEOUT_MS         5000U
#define CI13162_OTA_GET_INFO_TIMEOUT_MS    35000U
#define CI13162_OTA_ERASE_TIMEOUT_MS       30000U
#define CI13162_OTA_COMPLETE_TIMEOUT_MS    10000U
#define CI13162_OTA_START_DELAY_MS         1500U
#define CI13162_OTA_REBOOT_DELAY_MS        500U
#define CI13162_OTA_RX_READ_SIZE           128U
#define CI13162_OTA_FRAME_MAX_PAYLOAD      (AIIO_CI13162_OTA_RX_PARSER_SIZE - CI13162_OTA_FRAME_MIN_LEN)
#define CI13162_OTA_CONTROL_PAYLOAD_MAX    32U

extern uint8_t is_ota;
extern void ble_deinit(void);

static volatile uint8_t s_ci13162_ota_busy;
static char s_ci13162_ota_firmware_url[AIIO_CI13162_OTA_URL_MAX];
static char s_ci13162_ota_agent_url[AIIO_CI13162_OTA_URL_MAX];
static const char *s_ci13162_ota_stage = "init";
static uint32_t s_ci13162_ota_handshake_tries;
static int32_t s_ci13162_ota_last_err;

static void ci13162_ota_set_stage(const char *stage)
{
    if (stage != NULL) {
        s_ci13162_ota_stage = stage;
    }
}

static void ci13162_ota_log_stage(const char *stage)
{
    ci13162_ota_set_stage(stage);
    os_trace("[CI13162][OTA] %s, heap %u\r\n",
             s_ci13162_ota_stage,
             (unsigned int)xPortGetFreeHeapSize());
}

typedef struct {
    uint8_t *workspace;
    uint8_t http_io[AIIO_CI13162_OTA_HTTP_CHUNK_SIZE];
    uint8_t *parser_storage;
    uint8_t uart_read[CI13162_OTA_RX_READ_SIZE];
    uint8_t control_payload[CI13162_OTA_CONTROL_PAYLOAD_MAX];
    uint8_t tail_pad[AIIO_CI13162_OTA_UART_CHUNK_SIZE];
    ci13162_ota_rx_parser_t parser;
    ci13162_ota_http_ctx_t http;
    ci13162_ota_fw2_layout_t layout;
    uint32_t window_start;
    size_t window_length;
    uint8_t uart_acquired;
} ci13162_ota_context_t;

static int32_t ci13162_ota_uart_tx(const void *data, size_t length, void *user)
{
    (void)user;
    return (aiio_ci13162_uart_ota_write(data, length) < 0) ? -1 : 0;
}

static int32_t ci13162_ota_send(uint8_t msg_type,
                                uint8_t cmd,
                                uint8_t seq,
                                const uint8_t *payload,
                                uint16_t payload_len)
{
    return ci13162_ota_frame_send(ci13162_ota_uart_tx,
                                  NULL,
                                  msg_type,
                                  cmd,
                                  seq,
                                  payload,
                                  payload_len);
}

static int32_t ci13162_ota_next_frame(ci13162_ota_context_t *ctx,
                                      ci13162_ota_frame_t *frame,
                                      uint32_t timeout_ms)
{
    TickType_t start;
    TickType_t timeout_ticks;

    if ((ctx == NULL) || (frame == NULL)) {
        return -1;
    }

    start = xTaskGetTickCount();
    timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    if (timeout_ticks == 0U) {
        timeout_ticks = 1U;
    }

    while ((xTaskGetTickCount() - start) <= timeout_ticks) {
        int32_t parser_result;
        int32_t read_length;

        bl_wdt_feed();
        if (aiio_ci13162_uart_ota_take_overflow()) {
            return -1;
        }

        parser_result = ci13162_ota_rx_parser_pop(&ctx->parser, frame);
        if (parser_result == CI13162_OTA_PROTO_OK) {
            return 0;
        }
        if (parser_result != CI13162_OTA_PROTO_NEED_MORE) {
            return -1;
        }

        read_length = aiio_ci13162_uart_ota_read(ctx->uart_read,
                                                 sizeof(ctx->uart_read),
                                                 CI13162_OTA_READ_SLICE_MS);
        if (read_length < 0) {
            return -1;
        }
        if (read_length == 0) {
            continue;
        }
        if (ci13162_ota_rx_parser_push(&ctx->parser,
                                       ctx->uart_read,
                                       (size_t)read_length) != CI13162_OTA_PROTO_OK) {
            return -1;
        }
    }

    return -1;
}

static int32_t ci13162_ota_wait_frame(ci13162_ota_context_t *ctx,
                                      uint8_t msg_type,
                                      uint8_t cmd,
                                      uint32_t timeout_ms,
                                      uint8_t *payload,
                                      size_t payload_capacity,
                                      uint16_t *payload_len)
{
    TickType_t start;
    TickType_t timeout_ticks;

    start = xTaskGetTickCount();
    timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    if (timeout_ticks == 0U) {
        timeout_ticks = 1U;
    }

    while ((xTaskGetTickCount() - start) <= timeout_ticks) {
        TickType_t elapsed = xTaskGetTickCount() - start;
        TickType_t remaining_ticks = (elapsed < timeout_ticks) ? (timeout_ticks - elapsed) : 1U;
        uint32_t remaining_ms = (uint32_t)(remaining_ticks * portTICK_PERIOD_MS);
        ci13162_ota_frame_t frame;

        if (ci13162_ota_next_frame(ctx, &frame, remaining_ms) != 0) {
            return -1;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_NOTIFY) &&
            (frame.cmd == CI13162_OTA_CMD_PROGRESS)) {
            continue;
        }
        if ((frame.msg_type != msg_type) || (frame.cmd != cmd)) {
            continue;
        }
        if (payload_len != NULL) {
            *payload_len = frame.payload_len;
        }
        /* Tolerate ACK payloads larger than the caller buffer: the reference
         * tool ignores ACK data, so truncate the copy instead of failing.
         * payload_len still reports the real length for strict callers. */
        if ((frame.payload_len > 0U) && (payload != NULL) && (payload_capacity > 0U)) {
            size_t copy_length = frame.payload_len;

            if (copy_length > payload_capacity) {
                copy_length = payload_capacity;
            }
            memcpy(payload, frame.payload, copy_length);
        }
        return 0;
    }

    return -1;
}

static int32_t ci13162_ota_send_wait_ack(ci13162_ota_context_t *ctx,
                                         uint8_t cmd,
                                         const uint8_t *payload,
                                         uint16_t payload_len,
                                         uint32_t timeout_ms)
{
    uint8_t ack_payload[4];
    uint16_t ack_payload_len = 0U;

    if (ci13162_ota_send(CI13162_OTA_MSG_CMD, cmd, 0U, payload, payload_len) !=
        CI13162_OTA_PROTO_OK) {
        return -1;
    }
    return ci13162_ota_wait_frame(ctx,
                                  CI13162_OTA_MSG_ACK,
                                  cmd,
                                  timeout_ms,
                                  ack_payload,
                                  sizeof(ack_payload),
                                  &ack_payload_len);
}

static int32_t ci13162_ota_send_wait_success_ack(ci13162_ota_context_t *ctx,
                                                 uint8_t cmd,
                                                 const uint8_t *payload,
                                                 uint16_t payload_len,
                                                 uint32_t timeout_ms)
{
    uint8_t ack_payload[4];
    uint16_t ack_payload_len = 0U;

    if (ci13162_ota_send(CI13162_OTA_MSG_CMD, cmd, 0U, payload, payload_len) !=
        CI13162_OTA_PROTO_OK) {
        return -1;
    }
    if (ci13162_ota_wait_frame(ctx,
                               CI13162_OTA_MSG_ACK,
                               cmd,
                               timeout_ms,
                               ack_payload,
                               sizeof(ack_payload),
                               &ack_payload_len) != 0) {
        return -1;
    }
    return ((ack_payload_len == 0U) ||
            ((ack_payload_len == 1U) && (ack_payload[0] == 0x01U)))
               ? 0
               : -1;
}

static int32_t ci13162_ota_change_baud(ci13162_ota_context_t *ctx, uint32_t baud)
{
    size_t payload_len;

    if (ci13162_ota_build_change_baud(ctx->control_payload,
                                      sizeof(ctx->control_payload),
                                      baud,
                                      &payload_len) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_CHANGE_BAUD,
                                  ctx->control_payload,
                                  (uint16_t)payload_len,
                                  CI13162_OTA_ACK_TIMEOUT_MS) != 0) {
        return -1;
    }
    return aiio_ci13162_uart_ota_set_baud(baud);
}

static int32_t ci13162_ota_test_baud(ci13162_ota_context_t *ctx)
{
    ci13162_ota_put_u32_le(ctx->control_payload, 0x10U);
    return ci13162_ota_send_wait_ack(ctx,
                                     CI13162_OTA_CMD_TEST_BAUD,
                                     ctx->control_payload,
                                     4U,
                                     CI13162_OTA_ACK_TIMEOUT_MS);
}

static int32_t ci13162_ota_download_agent(ci13162_ota_context_t *ctx)
{
    uint32_t content_length;
    uint32_t got = 0U;
    uint32_t last_log = 0U;
    int32_t result;

    os_trace("[CI13162][OTA] Agent URL %s\r\n", s_ci13162_ota_agent_url);
    memset(&ctx->http, 0, sizeof(ctx->http));
    ctx->http.sock_fd = -1;
    result = ci13162_ota_http_open(&ctx->http,
                                   s_ci13162_ota_agent_url,
                                   ctx->http_io,
                                   sizeof(ctx->http_io),
                                   AIIO_CI13162_OTA_HTTP_TIMEOUT_MS);
    if (result != CI13162_OTA_HTTP_OK) {
        s_ci13162_ota_last_err = result;
        os_trace("[CI13162][OTA] Agent HTTP open failed (%d)\r\n", (int)result);
        return -1;
    }

    content_length = ci13162_ota_http_content_length(&ctx->http);
    if (content_length != AIIO_CI13162_OTA_AGENT_SIZE) {
        s_ci13162_ota_last_err = CI13162_OTA_HTTP_ERR_LENGTH;
        os_trace("[CI13162][OTA] Agent size mismatch got %u expect %u\r\n",
                 (unsigned int)content_length,
                 (unsigned int)AIIO_CI13162_OTA_AGENT_SIZE);
        ci13162_ota_http_close(&ctx->http);
        return -1;
    }

    os_trace("[CI13162][OTA] Reading agent body %u bytes\r\n",
             (unsigned int)AIIO_CI13162_OTA_AGENT_SIZE);
    while (got < AIIO_CI13162_OTA_AGENT_SIZE) {
        size_t want = (size_t)(AIIO_CI13162_OTA_AGENT_SIZE - got);

        if (want > AIIO_CI13162_OTA_HTTP_CHUNK_SIZE) {
            want = AIIO_CI13162_OTA_HTTP_CHUNK_SIZE;
        }
        bl_wdt_feed();
        result = ci13162_ota_http_read(&ctx->http, ctx->workspace + got, want);
        if (result <= 0) {
            s_ci13162_ota_last_err = result;
            os_trace("[CI13162][OTA] Agent read failed at %u/%u (%d)\r\n",
                     (unsigned int)got,
                     (unsigned int)AIIO_CI13162_OTA_AGENT_SIZE,
                     (int)result);
            ci13162_ota_http_close(&ctx->http);
            return -1;
        }
        got += (uint32_t)result;
        if ((got == AIIO_CI13162_OTA_AGENT_SIZE) ||
            ((got - last_log) >= 4096U)) {
            os_trace("[CI13162][HTTP] Agent body %u/%u\r\n",
                     (unsigned int)got,
                     (unsigned int)AIIO_CI13162_OTA_AGENT_SIZE);
            last_log = got;
        }
    }

    if (!ci13162_ota_http_is_complete(&ctx->http)) {
        s_ci13162_ota_last_err = CI13162_OTA_HTTP_ERR_EOF;
        os_trace("[CI13162][OTA] Agent HTTP incomplete %u/%u\r\n",
                 (unsigned int)ci13162_ota_http_body_read(&ctx->http),
                 (unsigned int)content_length);
        ci13162_ota_http_close(&ctx->http);
        return -1;
    }
    ci13162_ota_http_close(&ctx->http);
    os_trace("[CI13162][OTA] Agent downloaded\r\n");
    return 0;
}

static int32_t ci13162_ota_enter_maskrom(ci13162_ota_context_t *ctx)
{
    TickType_t start;
    TickType_t timeout_ticks;

    if (aiio_ci13162_uart_ota_set_baud(115200U) != 0) {
        return -1;
    }
    aiio_ci13162_uart_ota_flush_rx();
    ci13162_ota_rx_parser_reset(&ctx->parser);

    /* Send the app reset command first: binary OTA bytes emitted before the
     * JSON reset line can make the CI13162 application line parser discard
     * the whole line (frame contains 0x00 bytes and no CRLF). Per the OTA
     * guide, software reset goes first, then the handshake spam starts. */
    if (aiio_ci13162_uart_ota_send_app_reset() < 0) {
        return -1;
    }

    start = xTaskGetTickCount();
    timeout_ticks = pdMS_TO_TICKS(AIIO_CI13162_OTA_HANDSHAKE_TIMEOUT_MS);
    s_ci13162_ota_handshake_tries = 0U;
    while ((xTaskGetTickCount() - start) <= timeout_ticks) {
        ci13162_ota_frame_t frame;

        bl_wdt_feed();
        s_ci13162_ota_handshake_tries++;
        if (ci13162_ota_send(CI13162_OTA_MSG_REQ,
                             CI13162_OTA_CMD_HANDSHAKE,
                             0U,
                             NULL,
                             0U) != CI13162_OTA_PROTO_OK) {
            s_ci13162_ota_last_err = -1;
            return -1;
        }
        if (ci13162_ota_next_frame(ctx, &frame, CI13162_OTA_HANDSHAKE_PERIOD_MS) == 0) {
            if ((frame.msg_type == CI13162_OTA_MSG_ACK) &&
                (frame.cmd == CI13162_OTA_CMD_HANDSHAKE)) {
                return 0;
            }
        }
    }
    return -1;
}

static int32_t ci13162_ota_maskrom_control(ci13162_ota_context_t *ctx)
{
    uint32_t index;

    for (index = 0U; index < 7U; index++) {
        if (ci13162_ota_send(CI13162_OTA_MSG_CMD, 0x85U, 0U, NULL, 0U) !=
            CI13162_OTA_PROTO_OK) {
            return -1;
        }
        vTaskDelay(pdMS_TO_TICKS(15U));
    }
    if (ci13162_ota_wait_frame(ctx,
                               CI13162_OTA_MSG_ACK,
                               0x85U,
                               1000U,
                               NULL,
                               0U,
                               NULL) != 0) {
        return -1;
    }

    ctx->control_payload[0] = 0U;
    if (ci13162_ota_send_wait_ack(ctx, 0x47U, ctx->control_payload, 1U, 3000U) != 0) {
        return -1;
    }
    if (ci13162_ota_send_wait_ack(ctx, 0x84U, NULL, 0U, 3000U) != 0) {
        return -1;
    }
    return ci13162_ota_change_baud(ctx, 460800U);
}

static int32_t ci13162_ota_serve_memory(ci13162_ota_context_t *ctx,
                                        const uint8_t *data,
                                        uint32_t data_size,
                                        uint32_t timeout_ms)
{
    uint32_t coverage_end = 0U;

    while (1) {
        ci13162_ota_frame_t frame;

        if (ci13162_ota_next_frame(ctx, &frame, timeout_ms) != 0) {
            return -1;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_NOTIFY) &&
            (frame.cmd == CI13162_OTA_CMD_PROGRESS)) {
            continue;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_REQ) &&
            (frame.cmd == CI13162_OTA_CMD_WRITE)) {
            uint32_t offset;
            uint32_t size;

            if (ci13162_ota_parse_write_req(frame.payload,
                                            frame.payload_len,
                                            &offset,
                                            &size) != CI13162_OTA_PROTO_OK) {
                return -1;
            }
            if ((size == 0U) || (size > AIIO_CI13162_OTA_UART_CHUNK_SIZE) ||
                (offset > data_size) || (size > (data_size - offset)) ||
                (size > UINT16_MAX)) {
                return -1;
            }
            if (ci13162_ota_frame_send_write_rsp(ci13162_ota_uart_tx,
                                                 NULL,
                                                 frame.seq,
                                                 offset,
                                                 data + offset,
                                                 (uint16_t)size) != CI13162_OTA_PROTO_OK) {
                return -1;
            }
            if (offset > coverage_end) {
                return -1;
            }
            if ((offset + size) > coverage_end) {
                coverage_end = offset + size;
            }
            continue;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_REQ) &&
            (frame.cmd == CI13162_OTA_CMD_BLOCK_DONE)) {
            return (coverage_end == data_size) ? 0 : -1;
        }
    }
}

static int32_t ci13162_ota_load_agent(ci13162_ota_context_t *ctx)
{
    uint16_t agent_crc;
    size_t payload_len;

    if (ci13162_ota_test_baud(ctx) != 0) {
        return -1;
    }
    agent_crc = ci13162_ota_crc16_ccitt(ctx->workspace,
                                        AIIO_CI13162_OTA_AGENT_SIZE,
                                        0U);
    if (ci13162_ota_build_block_info(ctx->control_payload,
                                     sizeof(ctx->control_payload),
                                     CI13162_OTA_AGENT_ADDR,
                                     AIIO_CI13162_OTA_AGENT_SIZE,
                                     agent_crc,
                                     &payload_len) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_BLOCK_INFO,
                                  ctx->control_payload,
                                  (uint16_t)payload_len,
                                  CI13162_OTA_ACK_TIMEOUT_MS) != 0) {
        return -1;
    }
    if (ci13162_ota_serve_memory(ctx,
                                 ctx->workspace,
                                 AIIO_CI13162_OTA_AGENT_SIZE,
                                 AIIO_CI13162_OTA_COMMAND_TIMEOUT_MS) != 0) {
        return -1;
    }
    if (ci13162_ota_send_wait_success_ack(ctx,
                                  CI13162_OTA_CMD_VERIFY_AGENT,
                                  NULL,
                                  0U,
                                  CI13162_OTA_ACK_TIMEOUT_MS) != 0) {
        return -1;
    }
    return 0;
}

static int32_t ci13162_ota_updater_init(ci13162_ota_context_t *ctx)
{
    uint16_t info_length;

    if (aiio_ci13162_uart_ota_set_baud(115200U) != 0) {
        return -1;
    }
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_CHECK_READY,
                                  NULL,
                                  0U,
                                  CI13162_OTA_ACK_TIMEOUT_MS) != 0) {
        return -1;
    }
    ctx->control_payload[0] = 2U;
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_SET_FW_FMT,
                                  ctx->control_payload,
                                  1U,
                                  CI13162_OTA_ACK_TIMEOUT_MS) != 0) {
        return -1;
    }
    if (ci13162_ota_send(CI13162_OTA_MSG_CMD,
                         CI13162_OTA_CMD_GET_INFO,
                         0U,
                         NULL,
                         0U) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    return ci13162_ota_wait_frame(ctx,
                                  CI13162_OTA_MSG_ACK,
                                  CI13162_OTA_CMD_GET_INFO,
                                  CI13162_OTA_GET_INFO_TIMEOUT_MS,
                                  ctx->workspace,
                                  AIIO_CI13162_OTA_WORKSPACE_SIZE,
                                  &info_length);
}

static int32_t ci13162_ota_open_firmware(ci13162_ota_context_t *ctx)
{
    uint32_t content_length;
    size_t prefetch_length;
    int32_t result;

    memset(&ctx->http, 0, sizeof(ctx->http));
    ctx->http.sock_fd = -1;
    result = ci13162_ota_http_open(&ctx->http,
                                   s_ci13162_ota_firmware_url,
                                   ctx->http_io,
                                   sizeof(ctx->http_io),
                                   AIIO_CI13162_OTA_HTTP_TIMEOUT_MS);
    if (result != CI13162_OTA_HTTP_OK) {
        return -1;
    }

    content_length = ci13162_ota_http_content_length(&ctx->http);
    prefetch_length = AIIO_CI13162_OTA_WORKSPACE_SIZE;
    if ((uint32_t)prefetch_length > content_length) {
        prefetch_length = (size_t)content_length;
    }
    if ((prefetch_length < CI13162_OTA_FW2_MIN_IMAGE_SIZE) ||
        (ci13162_ota_http_read_exact(&ctx->http,
                                     ctx->workspace,
                                     prefetch_length) != CI13162_OTA_HTTP_OK)) {
        ci13162_ota_http_close(&ctx->http);
        return -1;
    }
    if (ci13162_ota_fw2_validate(ctx->workspace,
                                 prefetch_length,
                                 content_length,
                                 AIIO_CI13162_OTA_FIRMWARE_MAX_SIZE,
                                 &ctx->layout) != CI13162_OTA_FW2_OK) {
        ci13162_ota_http_close(&ctx->http);
        return -1;
    }
    ctx->window_start = 0U;
    ctx->window_length = prefetch_length;
    return 0;
}

static int32_t ci13162_ota_stream_fill(ci13162_ota_context_t *ctx,
                                       uint32_t offset,
                                       uint32_t size)
{
    uint32_t content_length = ctx->layout.content_length;
    uint32_t requested_end = offset + size;

    if ((requested_end < offset) ||
        (size == 0U) ||
        (size > AIIO_CI13162_OTA_UART_CHUNK_SIZE) ||
        (requested_end > ctx->layout.aligned_fw_size)) {
        return -1;
    }

    while (requested_end > (ctx->window_start + (uint32_t)ctx->window_length)) {
        uint32_t window_end = ctx->window_start + (uint32_t)ctx->window_length;
        size_t keep = ctx->window_length;
        size_t free_space;
        size_t want;
        int32_t read_length;

        if (window_end >= content_length) {
            break;
        }
        if (keep > AIIO_CI13162_OTA_UART_CHUNK_SIZE) {
            keep = AIIO_CI13162_OTA_UART_CHUNK_SIZE;
        }
        if (keep > 0U) {
            memmove(ctx->workspace,
                    ctx->workspace + ctx->window_length - keep,
                    keep);
        }
        ctx->window_start = window_end - (uint32_t)keep;
        ctx->window_length = keep;
        free_space = AIIO_CI13162_OTA_WORKSPACE_SIZE - ctx->window_length;
        want = free_space;
        if ((uint32_t)want > (content_length - window_end)) {
            want = (size_t)(content_length - window_end);
        }
        read_length = ci13162_ota_http_read(&ctx->http,
                                            ctx->workspace + ctx->window_length,
                                            want);
        if (read_length <= 0) {
            return -1;
        }
        ctx->window_length += (size_t)read_length;
    }

    if ((offset < ctx->window_start) ||
        (offset > (ctx->window_start + (uint32_t)ctx->window_length))) {
        return -1;
    }
    if (requested_end > content_length) {
        uint32_t http_read = ci13162_ota_http_body_read(&ctx->http);
        uint32_t available;
        size_t source_index;

        while (http_read < content_length) {
            size_t want;
            int32_t read_length;

            if (ctx->window_length == AIIO_CI13162_OTA_WORKSPACE_SIZE) {
                size_t keep = AIIO_CI13162_OTA_UART_CHUNK_SIZE;
                uint32_t window_end = ctx->window_start + (uint32_t)ctx->window_length;

                memmove(ctx->workspace,
                        ctx->workspace + ctx->window_length - keep,
                        keep);
                ctx->window_start = window_end - (uint32_t)keep;
                ctx->window_length = keep;
            }
            want = AIIO_CI13162_OTA_WORKSPACE_SIZE - ctx->window_length;
            if ((uint32_t)want > (content_length - http_read)) {
                want = (size_t)(content_length - http_read);
            }
            read_length = ci13162_ota_http_read(&ctx->http,
                                                ctx->workspace + ctx->window_length,
                                                want);
            if (read_length <= 0) {
                return -1;
            }
            ctx->window_length += (size_t)read_length;
            http_read += (uint32_t)read_length;
        }

        if (offset < ctx->window_start) {
            return -1;
        }
        available = (content_length > offset) ? (content_length - offset) : 0U;
        source_index = (size_t)(offset - ctx->window_start);
        if ((source_index + available) > ctx->window_length) {
            return -1;
        }
        if (available > 0U) {
            memcpy(ctx->tail_pad, ctx->workspace + source_index, available);
        }
        memset(ctx->tail_pad + available, 0xFF, size - available);
        return 1;
    }
    if (requested_end > (ctx->window_start + (uint32_t)ctx->window_length)) {
        return -1;
    }
    return 0;
}

static int32_t ci13162_ota_compute_stream_crc(uint32_t offset,
                                              const uint8_t *data,
                                              uint32_t size,
                                              uint32_t *crc_cursor,
                                              uint16_t *crc)
{
    uint32_t overlap_start;
    uint32_t overlap_end;

    if (offset > *crc_cursor) {
        return -1;
    }
    overlap_start = (*crc_cursor > offset) ? *crc_cursor : offset;
    overlap_end = offset + size;
    if (overlap_end > overlap_start) {
        *crc = ci13162_ota_crc16_ccitt(data + (overlap_start - offset),
                                       overlap_end - overlap_start,
                                       *crc);
        *crc_cursor = overlap_end;
    }
    return 0;
}

static int32_t ci13162_ota_finish_http_body(ci13162_ota_context_t *ctx)
{
    while (!ci13162_ota_http_is_complete(&ctx->http)) {
        uint32_t http_read = ci13162_ota_http_body_read(&ctx->http);
        uint32_t content_length = ctx->layout.content_length;
        size_t want;
        int32_t read_length;

        if (ctx->window_length == AIIO_CI13162_OTA_WORKSPACE_SIZE) {
            size_t keep = AIIO_CI13162_OTA_UART_CHUNK_SIZE;
            uint32_t window_end = ctx->window_start + (uint32_t)ctx->window_length;

            memmove(ctx->workspace,
                    ctx->workspace + ctx->window_length - keep,
                    keep);
            ctx->window_start = window_end - (uint32_t)keep;
            ctx->window_length = keep;
        }
        want = AIIO_CI13162_OTA_WORKSPACE_SIZE - ctx->window_length;
        if ((uint32_t)want > (content_length - http_read)) {
            want = (size_t)(content_length - http_read);
        }
        read_length = ci13162_ota_http_read(&ctx->http,
                                            ctx->workspace + ctx->window_length,
                                            want);
        if (read_length <= 0) {
            return -1;
        }
        ctx->window_length += (size_t)read_length;
    }
    return 0;
}

static int32_t ci13162_ota_write_firmware(ci13162_ota_context_t *ctx)
{
    uint32_t coverage_end = 0U;
    uint32_t crc_cursor = 0U;
    uint16_t crc = 0U;
    size_t payload_len;

    if (ci13162_ota_change_baud(ctx, 921600U) != 0) {
        return -1;
    }
    if (ci13162_ota_test_baud(ctx) != 0) {
        return -1;
    }
    if (ci13162_ota_build_block_info(ctx->control_payload,
                                     sizeof(ctx->control_payload),
                                     CI13162_OTA_FW_BASE,
                                     ctx->layout.aligned_fw_size,
                                     0U,
                                     &payload_len) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_BLOCK_INFO,
                                  ctx->control_payload,
                                  (uint16_t)payload_len,
                                  CI13162_OTA_ACK_TIMEOUT_MS) != 0) {
        return -1;
    }
    if (ci13162_ota_build_erase(ctx->control_payload,
                                sizeof(ctx->control_payload),
                                CI13162_OTA_FW_BASE,
                                ctx->layout.aligned_fw_size,
                                AIIO_CI13162_OTA_UART_CHUNK_SIZE,
                                &payload_len) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_ERASE,
                                  ctx->control_payload,
                                  (uint16_t)payload_len,
                                  CI13162_OTA_ERASE_TIMEOUT_MS) != 0) {
        return -1;
    }
    while (1) {
        ci13162_ota_frame_t frame;

        if (ci13162_ota_next_frame(ctx,
                                   &frame,
                                   AIIO_CI13162_OTA_COMMAND_TIMEOUT_MS) != 0) {
            return -1;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_NOTIFY) &&
            (frame.cmd == CI13162_OTA_CMD_PROGRESS)) {
            continue;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_REQ) &&
            (frame.cmd == CI13162_OTA_CMD_WRITE)) {
            uint32_t offset;
            uint32_t size;
            int32_t fill_result;
            const uint8_t *chunk;

            if (ci13162_ota_parse_write_req(frame.payload,
                                            frame.payload_len,
                                            &offset,
                                            &size) != CI13162_OTA_PROTO_OK) {
                return -1;
            }
            fill_result = ci13162_ota_stream_fill(ctx, offset, size);
            if (fill_result < 0) {
                return -1;
            }
            chunk = (fill_result == 1)
                        ? ctx->tail_pad
                        : ctx->workspace + (size_t)(offset - ctx->window_start);
            if (ci13162_ota_frame_send_write_rsp(ci13162_ota_uart_tx,
                                                 NULL,
                                                 frame.seq,
                                                 offset,
                                                 chunk,
                                                 (uint16_t)size) != CI13162_OTA_PROTO_OK) {
                return -1;
            }
            if (ci13162_ota_compute_stream_crc(offset,
                                               chunk,
                                               size,
                                               &crc_cursor,
                                               &crc) != 0) {
                return -1;
            }
            if (offset > coverage_end) {
                return -1;
            }
            if ((offset + size) > coverage_end) {
                coverage_end = offset + size;
            }
            continue;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_REQ) &&
            (frame.cmd == CI13162_OTA_CMD_BLOCK_DONE)) {
            break;
        }
    }

    if ((coverage_end != ctx->layout.aligned_fw_size) ||
        (crc_cursor != ctx->layout.aligned_fw_size) ||
        (ci13162_ota_finish_http_body(ctx) != 0)) {
        return -1;
    }
    ci13162_ota_http_close(&ctx->http);

    if (ci13162_ota_build_verify(ctx->control_payload,
                                 sizeof(ctx->control_payload),
                                 CI13162_OTA_FW_BASE,
                                 ctx->layout.aligned_fw_size,
                                 crc,
                                 &payload_len) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    return ci13162_ota_send_wait_success_ack(ctx,
                                     CI13162_OTA_CMD_VERIFY,
                                     ctx->control_payload,
                                     (uint16_t)payload_len,
                                     AIIO_CI13162_OTA_COMMAND_TIMEOUT_MS);
}

static uint16_t ci13162_ota_crc_ff(uint32_t size)
{
    uint8_t ff[64];
    uint16_t crc = 0U;

    memset(ff, 0xFF, sizeof(ff));
    while (size > 0U) {
        uint32_t count = (size > sizeof(ff)) ? sizeof(ff) : size;
        crc = ci13162_ota_crc16_ccitt(ff, count, crc);
        size -= count;
    }
    return crc;
}

static int32_t ci13162_ota_erase_nv(ci13162_ota_context_t *ctx)
{
#if AIIO_CI13162_OTA_ERASE_NV == 1
    uint32_t coverage_end = 0U;
    uint16_t crc;
    size_t payload_len;

    memset(ctx->tail_pad, 0xFF, sizeof(ctx->tail_pad));
    crc = ci13162_ota_crc_ff(ctx->layout.nv_size);
    if (ci13162_ota_build_block_info(ctx->control_payload,
                                     sizeof(ctx->control_payload),
                                     ctx->layout.nv_offset,
                                     ctx->layout.nv_size,
                                     0U,
                                     &payload_len) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_BLOCK_INFO,
                                  ctx->control_payload,
                                  (uint16_t)payload_len,
                                  CI13162_OTA_ACK_TIMEOUT_MS) != 0) {
        return -1;
    }
    if (ci13162_ota_build_erase(ctx->control_payload,
                                sizeof(ctx->control_payload),
                                ctx->layout.nv_offset,
                                ctx->layout.nv_size,
                                AIIO_CI13162_OTA_UART_CHUNK_SIZE,
                                &payload_len) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_ERASE,
                                  ctx->control_payload,
                                  (uint16_t)payload_len,
                                  CI13162_OTA_ERASE_TIMEOUT_MS) != 0) {
        return -1;
    }

    while (1) {
        ci13162_ota_frame_t frame;

        if (ci13162_ota_next_frame(ctx,
                                   &frame,
                                   AIIO_CI13162_OTA_COMMAND_TIMEOUT_MS) != 0) {
            return -1;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_NOTIFY) &&
            (frame.cmd == CI13162_OTA_CMD_PROGRESS)) {
            continue;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_REQ) &&
            (frame.cmd == CI13162_OTA_CMD_WRITE)) {
            uint32_t offset;
            uint32_t size;

            if ((ci13162_ota_parse_write_req(frame.payload,
                                             frame.payload_len,
                                             &offset,
                                             &size) != CI13162_OTA_PROTO_OK) ||
                (size == 0U) || (size > sizeof(ctx->tail_pad)) ||
                (offset > ctx->layout.nv_size) ||
                (size > (ctx->layout.nv_size - offset))) {
                return -1;
            }
            if (ci13162_ota_frame_send_write_rsp(ci13162_ota_uart_tx,
                                                 NULL,
                                                 frame.seq,
                                                 offset,
                                                 ctx->tail_pad,
                                                 (uint16_t)size) != CI13162_OTA_PROTO_OK) {
                return -1;
            }
            if (offset > coverage_end) {
                return -1;
            }
            if ((offset + size) > coverage_end) {
                coverage_end = offset + size;
            }
            continue;
        }
        if ((frame.msg_type == CI13162_OTA_MSG_REQ) &&
            (frame.cmd == CI13162_OTA_CMD_BLOCK_DONE)) {
            break;
        }
    }
    if (coverage_end != ctx->layout.nv_size) {
        return -1;
    }
    if (ci13162_ota_build_verify(ctx->control_payload,
                                 sizeof(ctx->control_payload),
                                 ctx->layout.nv_offset,
                                 ctx->layout.nv_size,
                                 crc,
                                 &payload_len) != CI13162_OTA_PROTO_OK) {
        return -1;
    }
    return ci13162_ota_send_wait_success_ack(ctx,
                                     CI13162_OTA_CMD_VERIFY,
                                     ctx->control_payload,
                                     (uint16_t)payload_len,
                                     AIIO_CI13162_OTA_COMMAND_TIMEOUT_MS);
#else
    (void)ctx;
    return 0;
#endif
}

static void ci13162_ota_cleanup(ci13162_ota_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ci13162_ota_http_close(&ctx->http);
    if (ctx->uart_acquired != 0U) {
        aiio_ci13162_uart_ota_release();
        ctx->uart_acquired = 0U;
    }
    if (ctx->parser_storage != NULL) {
        free(ctx->parser_storage);
        ctx->parser_storage = NULL;
    }
    if (ctx->workspace != NULL) {
        free(ctx->workspace);
        ctx->workspace = NULL;
    }
    free(ctx);
}

static ci13162_ota_context_t *ci13162_ota_alloc_context(void)
{
    ci13162_ota_context_t *ctx;

    ctx = (ci13162_ota_context_t *)calloc(1U, sizeof(*ctx));
    if (ctx == NULL) {
        return NULL;
    }
    ctx->http.sock_fd = -1;
    ctx->workspace = (uint8_t *)malloc(AIIO_CI13162_OTA_WORKSPACE_SIZE);
    ctx->parser_storage = (uint8_t *)malloc(AIIO_CI13162_OTA_RX_PARSER_SIZE);
    if ((ctx->workspace == NULL) || (ctx->parser_storage == NULL)) {
        free(ctx->parser_storage);
        free(ctx->workspace);
        free(ctx);
        return NULL;
    }
    return ctx;
}

static void ci13162_ota_task(void *argument)
{
    ci13162_ota_context_t *ctx = NULL;
    int32_t result = -1;
    uint32_t attempt;

    (void)argument;
    s_ci13162_ota_last_err = 0;
    s_ci13162_ota_handshake_tries = 0U;
    ci13162_ota_set_stage("init");
    vTaskDelay(pdMS_TO_TICKS(CI13162_OTA_START_DELAY_MS));
    is_ota = STATE_OTA_PROCESS;
    if (!m_ci13162_ota_boot_is_active()) {
        ble_deinit();
        m_ir_deinit();
        m_mqtt_stop();
    }

    /* Allocate the large OTA buffers only after optional product services
     * released their heap. Retry to tolerate asynchronous teardown. */
    for (attempt = 0U; attempt < CI13162_OTA_ALLOC_RETRY_COUNT; attempt++) {
        bl_wdt_feed();
        vTaskDelay(pdMS_TO_TICKS(CI13162_OTA_ALLOC_RETRY_DELAY_MS));
        os_trace("[CI13162][OTA] Allocation attempt %u, free heap: %u bytes\r\n",
                 (unsigned int)(attempt + 1U),
                 (unsigned int)xPortGetFreeHeapSize());
        ctx = ci13162_ota_alloc_context();
        if (ctx != NULL) {
            break;
        }
    }
    if (ctx == NULL) {
        ci13162_ota_set_stage("alloc");
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    os_trace("[CI13162][OTA] Context allocated, free heap: %u bytes\r\n",
             (unsigned int)xPortGetFreeHeapSize());

    ci13162_ota_log_stage("download_agent");
    if (ci13162_ota_download_agent(ctx) != 0) {
        goto EXIT;
    }

    os_trace("[CI13162][OTA] Firmware URL %s\r\n", s_ci13162_ota_firmware_url);
    os_trace("[CI13162][OTA] Taking UART0, console paused during chip OTA\r\n");
    (void)fflush(stdout);
    ci13162_ota_log_stage("uart_acquire");
    if (aiio_ci13162_uart_ota_acquire() != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ctx->uart_acquired = 1U;
    ci13162_ota_set_stage("parser_init");
    if (ci13162_ota_rx_parser_init(&ctx->parser,
                                   ctx->parser_storage,
                                   AIIO_CI13162_OTA_RX_PARSER_SIZE,
                                   CI13162_OTA_FRAME_MAX_PAYLOAD) != CI13162_OTA_PROTO_OK) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ci13162_ota_set_stage("enter_maskrom");
    if (ci13162_ota_enter_maskrom(ctx) != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ci13162_ota_set_stage("maskrom_control");
    if (ci13162_ota_maskrom_control(ctx) != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ci13162_ota_set_stage("load_agent");
    if (ci13162_ota_load_agent(ctx) != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ci13162_ota_set_stage("updater_init");
    if (ci13162_ota_updater_init(ctx) != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ci13162_ota_set_stage("open_firmware");
    if (ci13162_ota_open_firmware(ctx) != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ci13162_ota_set_stage("write_firmware");
    if (ci13162_ota_write_firmware(ctx) != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ci13162_ota_set_stage("erase_nv");
    if (ci13162_ota_erase_nv(ctx) != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    ci13162_ota_set_stage("complete");
    if (ci13162_ota_send_wait_ack(ctx,
                                  CI13162_OTA_CMD_COMPLETE,
                                  NULL,
                                  0U,
                                  CI13162_OTA_COMPLETE_TIMEOUT_MS) != 0) {
        s_ci13162_ota_last_err = -1;
        goto EXIT;
    }
    result = 0;

EXIT:
    ci13162_ota_cleanup(ctx);
    s_ci13162_ota_busy = 0U;
    os_trace("[CI13162][OTA] %s at %s, err %d, handshake %u, heap %u\r\n",
             (result == 0) ? "SUCCESS" : "FAILED",
             s_ci13162_ota_stage,
             (int)s_ci13162_ota_last_err,
             (unsigned int)s_ci13162_ota_handshake_tries,
             (unsigned int)xPortGetFreeHeapSize());
    if (result == 0) {
        if (m_ci13162_ota_boot_is_active() &&
            !m_ci13162_ota_boot_clear()) {
            os_trace("[CI13162][OTA] Pending request clear failed\r\n");
        }
        is_ota = STATE_OTA_SUCCESS;
    } else {
        is_ota = STATE_OTA_STOP;
    }
    os_trace("[CI13162][OTA] Rebooting WB2 after %s\r\n",
             (result == 0) ? "success" : "failure");
    (void)fflush(stdout);
    bl_wdt_feed();
    vTaskDelay(pdMS_TO_TICKS(CI13162_OTA_REBOOT_DELAY_MS));
    bl_sys_reset_por();
    vTaskDelete(NULL);
}

#endif /* AIIO_CI13162_OTA_ENABLE */

int32_t m_ci13162_ota_start(const char *firmware_url, const char *agent_url)
{
#if AIIO_CI13162_OTA_ENABLE == 1
    BaseType_t task_result;

    if (!m_ci13162_ota_url_is_valid(firmware_url) ||
        !m_ci13162_ota_url_is_valid(agent_url)) {
        os_trace("[CI13162][OTA] Start rejected, invalid URL\r\n");
        return -1;
    }

    taskENTER_CRITICAL();
    if ((s_ci13162_ota_busy != 0U) || (is_ota != STATE_OTA_STOP)) {
        taskEXIT_CRITICAL();
        os_trace("[CI13162][OTA] Start rejected, busy=%u is_ota=%u\r\n",
                 (unsigned int)s_ci13162_ota_busy,
                 (unsigned int)is_ota);
        return -1;
    }
    s_ci13162_ota_busy = 1U;
    is_ota = STATE_OTA_PROCESS;
    taskEXIT_CRITICAL();

    memcpy(s_ci13162_ota_firmware_url, firmware_url, strlen(firmware_url) + 1U);
    memcpy(s_ci13162_ota_agent_url, agent_url, strlen(agent_url) + 1U);
    /* Dynamic stack on purpose: it only takes heap while OTA runs. A create
     * failure here is harmless (nothing was shut down yet, respond 0). */
    task_result = xTaskCreate(ci13162_ota_task,
                              "ci13162_ota",
                              CI13162_OTA_TASK_STACK_DEPTH,
                              NULL,
                              CI13162_OTA_TASK_PRIORITY,
                              NULL);
    if (task_result != pdPASS) {
        s_ci13162_ota_busy = 0U;
        is_ota = STATE_OTA_STOP;
        os_trace("[CI13162][OTA] Task create failed: %ld\r\n",
                 (long)task_result);
        return -1;
    }
    return 0;
#else
    (void)firmware_url;
    (void)agent_url;
    return -1;
#endif
}

bool m_ci13162_ota_is_busy(void)
{
#if AIIO_CI13162_OTA_ENABLE == 1
    return s_ci13162_ota_busy != 0U;
#else
    return false;
#endif
}

uint32_t m_ci13162_ota_timeout_ms(void)
{
    return AIIO_CI13162_OTA_TOTAL_TIMEOUT_MS;
}
