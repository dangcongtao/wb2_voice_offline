#include "app_voice.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include "aiio_ci13162_config.h"
#include "aiio_ci13162_uart.h"
#include "app_config.h"
#include "app_cwsl.h"
#include "app_volume.h"
#include "m_ci13162_ota.h"
#include "m_ota.h"
#include "process_data.h"
#include "system_typedef.h"

#define APP_VOICE_CMD_SET 230U
#define APP_VOICE_CMD_GET 231U
#define APP_VOICE_ID_MIN 1U
#define APP_VOICE_ID_MAX 4U
#define APP_VOICE_KEY_VOICE_ID "voice_id"
#define APP_VOICE_RESPONSE_SIZE 128U

#define APP_VOICE_CODE_TIMEOUT 52U
#define APP_VOICE_CODE_BAD_REQUEST 64U
#define APP_VOICE_CODE_BUSY 67U
#define APP_VOICE_CODE_OTA_HOLD 69U
#define APP_VOICE_CODE_TX_FAIL 70U

typedef enum {
    APP_VOICE_OPERATION_NONE = 0,
    APP_VOICE_OPERATION_SET,
    APP_VOICE_OPERATION_GET,
    APP_VOICE_OPERATION_BUTTON_QUERY,
} app_voice_operation_t;

typedef struct {
    uint8_t active;
    app_voice_operation_t operation;
    uint8_t target;
    uint32_t user_id;
    uint8_t act_id;
    TickType_t start_tick;
} app_voice_pending_t;

static app_voice_pending_t s_app_voice_pending;
static uint8_t s_app_voice_draining;
static app_voice_operation_t s_app_voice_drain_operation;
static TickType_t s_app_voice_drain_tick;

// Return true while an OTA path owns or is preparing to own the voice UART.
static bool app_voice_ota_active(void)
{
    return aiio_ci13162_uart_ota_held() ||
           m_ci13162_ota_is_busy() ||
           m_ota_is_busy();
}

// Read one required non-negative integral JSON number within the requested range.
static bool app_voice_read_number(cJSON *js_root,
                                  const char *key,
                                  uint32_t maximum,
                                  uint32_t *value)
{
    cJSON *item;
    double number;
    uint32_t converted;

    if ((js_root == NULL) || (key == NULL) || (value == NULL)) {
        return false;
    }

    item = cJSON_GetObjectItem(js_root, key);
    if ((item == NULL) || ((item->type & 0xFF) != cJSON_Number)) {
        return false;
    }

    number = item->valuedouble;
    if (!((number >= 0.0) && (number <= (double)maximum))) {
        return false;
    }

    converted = (uint32_t)number;
    if ((double)converted != number) {
        return false;
    }

    *value = converted;
    return true;
}

// Publish one bounded voice response through the existing MQTT and BLE path.
// A GET failure reply carries no voice_id key.
static void app_voice_publish(uint32_t mqtt_cmd,
                              uint8_t voice_id,
                              uint32_t user_id,
                              uint8_t act_id,
                              uint8_t result,
                              uint8_t code)
{
    char response[APP_VOICE_RESPONSE_SIZE];
    int response_length;

    if (result != 0U) {
        response_length = snprintf(
            response,
            sizeof(response),
            "{\"%s\":%lu,\"u\":%lu,\"voice_id\":%u,\"act_id\":%u,\"result\":1}",
            gs_dev_type[device_type],
            (unsigned long)mqtt_cmd,
            (unsigned long)user_id,
            (unsigned int)voice_id,
            (unsigned int)act_id);
    } else if (mqtt_cmd == APP_VOICE_CMD_SET) {
        response_length = snprintf(
            response,
            sizeof(response),
            "{\"%s\":%lu,\"u\":%lu,\"voice_id\":%u,\"act_id\":%u,\"result\":0,\"code\":%u}",
            gs_dev_type[device_type],
            (unsigned long)mqtt_cmd,
            (unsigned long)user_id,
            (unsigned int)voice_id,
            (unsigned int)act_id,
            (unsigned int)code);
    } else {
        response_length = snprintf(
            response,
            sizeof(response),
            "{\"%s\":%lu,\"u\":%lu,\"act_id\":%u,\"result\":0,\"code\":%u}",
            gs_dev_type[device_type],
            (unsigned long)mqtt_cmd,
            (unsigned long)user_id,
            (unsigned int)act_id,
            (unsigned int)code);
    }
    if ((response_length <= 0) ||
        ((size_t)response_length >= sizeof(response))) {
        os_trace("[VOICE] response formatting failed\r\n");
        return;
    }

    app_send_data(response, (uint32_t)response_length);
}

// Return the MQTT command number that owns the given operation.
static uint32_t app_voice_cmd_of(app_voice_operation_t operation)
{
    return ((operation == APP_VOICE_OPERATION_SET) ||
            (operation == APP_VOICE_OPERATION_BUTTON_QUERY))
               ? APP_VOICE_CMD_SET
               : APP_VOICE_CMD_GET;
}

// Clear the current request and optionally reserve a drain window for a late reply.
static void app_voice_clear_pending(uint8_t drain_late_response)
{
    app_voice_operation_t operation = s_app_voice_pending.operation;

    memset(&s_app_voice_pending, 0, sizeof(s_app_voice_pending));
    if (drain_late_response != 0U) {
        s_app_voice_draining = 1U;
        s_app_voice_drain_operation = operation;
        s_app_voice_drain_tick = xTaskGetTickCount();
    }
}

// Return the voice that follows the given one (1..4, wrapping).
static uint8_t app_voice_next_of(uint8_t voice_id)
{
    return (uint8_t)((voice_id % APP_VOICE_ID_MAX) + 1U);
}

// Complete a pending SET with the chip verdict, or swallow its late reply.
static void app_voice_on_set(uint8_t applied)
{
    app_voice_pending_t pending;

    if ((s_app_voice_draining != 0U) &&
        (s_app_voice_drain_operation == APP_VOICE_OPERATION_SET)) {
        s_app_voice_draining = 0U;
        return;
    }
    if ((s_app_voice_pending.active == 0U) ||
        (s_app_voice_pending.operation != APP_VOICE_OPERATION_SET)) {
        return;
    }

    pending = s_app_voice_pending;
    app_voice_clear_pending(0U);
    app_voice_publish(APP_VOICE_CMD_SET,
                      pending.target,
                      pending.user_id,
                      pending.act_id,
                      (applied != 0U) ? 1U : 0U,
                      APP_VOICE_CODE_BAD_REQUEST);
}

// Complete a pending GET with the reported voice, or swallow its late reply.
static void app_voice_on_get(uint8_t voice_id)
{
    app_voice_pending_t pending;

    if ((s_app_voice_draining != 0U) &&
        ((s_app_voice_drain_operation == APP_VOICE_OPERATION_GET) ||
         (s_app_voice_drain_operation == APP_VOICE_OPERATION_BUTTON_QUERY))) {
        s_app_voice_draining = 0U;
        return;
    }
    if ((s_app_voice_pending.active == 0U) ||
        ((s_app_voice_pending.operation != APP_VOICE_OPERATION_GET) &&
         (s_app_voice_pending.operation != APP_VOICE_OPERATION_BUTTON_QUERY))) {
        return;
    }

    pending = s_app_voice_pending;
    app_voice_clear_pending(0U);
    if (pending.operation == APP_VOICE_OPERATION_BUTTON_QUERY) {
        uint8_t target;
        int32_t send_result;

        if ((voice_id < APP_VOICE_ID_MIN) || (voice_id > APP_VOICE_ID_MAX)) {
            app_voice_publish(APP_VOICE_CMD_SET, 0U, 0U, (uint8_t)SRC_ACTION_DEVICE,
                              0U, APP_VOICE_CODE_BAD_REQUEST);
            return;
        }
        target = app_voice_next_of(voice_id);
        s_app_voice_pending.active = 1U;
        s_app_voice_pending.operation = APP_VOICE_OPERATION_SET;
        s_app_voice_pending.target = target;
        s_app_voice_pending.user_id = 0U;
        s_app_voice_pending.act_id = (uint8_t)SRC_ACTION_DEVICE;
        s_app_voice_pending.start_tick = xTaskGetTickCount();
        send_result = aiio_ci13162_uart_send_voice_set(target);
        if (send_result != 0) {
            app_voice_clear_pending(0U);
            app_voice_publish(APP_VOICE_CMD_SET, target, 0U, (uint8_t)SRC_ACTION_DEVICE,
                              0U, APP_VOICE_CODE_TX_FAIL);
        }
        return;
    }
    app_voice_publish(APP_VOICE_CMD_GET,
                      voice_id,
                      pending.user_id,
                      pending.act_id,
                      1U,
                      0U);
}

// Return true while another UART owner or voice transaction is already active.
static bool app_voice_busy(void)
{
    return (s_app_voice_pending.active != 0U) ||
           (s_app_voice_draining != 0U) ||
           app_cwsl_is_pending() ||
           app_volume_is_pending();
}

// Validate and start one MQTT set or get. Check order: 64, 69, 67, send (70).
static void app_voice_start_mqtt(cJSON *js_root, uint32_t mqtt_cmd)
{
    uint32_t parsed_cmd = mqtt_cmd;
    uint32_t user_id = 0U;
    uint32_t act_id = 0U;
    uint32_t voice_id = 0U;
    uint8_t valid;
    uint8_t code = 0U;
    int32_t send_result;

    valid = (app_voice_read_number(js_root,
                                   gs_dev_type[device_type],
                                   UINT32_MAX,
                                   &parsed_cmd) &&
             (parsed_cmd == mqtt_cmd))
                ? 1U
                : 0U;
    if (!app_voice_read_number(js_root, CJSON_KEY_USER_ID, UINT32_MAX, &user_id)) {
        user_id = 0U;
        valid = 0U;
    }
    if (!app_voice_read_number(js_root, CJSON_KEY_ACT_ID, UINT8_MAX, &act_id)) {
        act_id = 0U;
        valid = 0U;
    }
    if (mqtt_cmd == APP_VOICE_CMD_SET) {
        if (!app_voice_read_number(js_root,
                                   APP_VOICE_KEY_VOICE_ID,
                                   UINT8_MAX,
                                   &voice_id)) {
            voice_id = 0U;
            valid = 0U;
        } else if ((voice_id < APP_VOICE_ID_MIN) ||
                   (voice_id > APP_VOICE_ID_MAX)) {
            valid = 0U;
        }
    }

    if (valid == 0U) {
        code = APP_VOICE_CODE_BAD_REQUEST;
    } else if (app_voice_ota_active()) {
        code = APP_VOICE_CODE_OTA_HOLD;
    } else if (app_voice_busy()) {
        code = APP_VOICE_CODE_BUSY;
    }
    if (code != 0U) {
        app_voice_publish(mqtt_cmd, (uint8_t)voice_id, user_id, (uint8_t)act_id, 0U, code);
        return;
    }

    s_app_voice_pending.active = 1U;
    s_app_voice_pending.operation = (mqtt_cmd == APP_VOICE_CMD_SET)
                                        ? APP_VOICE_OPERATION_SET
                                        : APP_VOICE_OPERATION_GET;
    s_app_voice_pending.target = (uint8_t)voice_id;
    s_app_voice_pending.user_id = user_id;
    s_app_voice_pending.act_id = (uint8_t)act_id;
    s_app_voice_pending.start_tick = xTaskGetTickCount();

    send_result = (mqtt_cmd == APP_VOICE_CMD_SET)
                      ? aiio_ci13162_uart_send_voice_set((uint8_t)voice_id)
                      : aiio_ci13162_uart_query_voice();
    if (send_result != 0) {
        app_voice_clear_pending(0U);
        app_voice_publish(mqtt_cmd,
                          (uint8_t)voice_id,
                          user_id,
                          (uint8_t)act_id,
                          0U,
                          APP_VOICE_CODE_TX_FAIL);
    }
}

void app_voice_init(void)
{
    memset(&s_app_voice_pending, 0, sizeof(s_app_voice_pending));
    s_app_voice_draining = 0U;
    s_app_voice_drain_operation = APP_VOICE_OPERATION_NONE;
    s_app_voice_drain_tick = 0U;
    aiio_ci13162_uart_set_voice_callbacks(app_voice_on_set, app_voice_on_get);
}

void app_voice_set(cJSON *js_root)
{
    app_voice_start_mqtt(js_root, APP_VOICE_CMD_SET);
}

void app_voice_get(cJSON *js_root)
{
    app_voice_start_mqtt(js_root, APP_VOICE_CMD_GET);
}

void app_voice_poll(void)
{
    TickType_t current_tick = xTaskGetTickCount();

    if ((s_app_voice_pending.active != 0U) &&
        ((TickType_t)(current_tick - s_app_voice_pending.start_tick) >=
         pdMS_TO_TICKS(AIIO_CI13162_VOICE_TIMEOUT_MS))) {
        app_voice_pending_t pending = s_app_voice_pending;

        app_voice_clear_pending(1U);
        app_voice_publish(app_voice_cmd_of(pending.operation),
                          pending.target,
                          pending.user_id,
                          pending.act_id,
                          0U,
                          APP_VOICE_CODE_TIMEOUT);
    }

    if ((s_app_voice_draining != 0U) &&
        ((TickType_t)(current_tick - s_app_voice_drain_tick) >=
         pdMS_TO_TICKS(AIIO_CI13162_VOICE_DRAIN_MS))) {
        s_app_voice_draining = 0U;
    }
}

bool app_voice_button_next(void)
{
    uint8_t current;
    int32_t send_result;
    uint8_t reported = 0U;

    if (app_voice_ota_active() || app_voice_busy()) {
        return false;
    }

    current = aiio_ci13162_uart_get_voice_id();
    memset(&s_app_voice_pending, 0, sizeof(s_app_voice_pending));
    s_app_voice_pending.active = 1U;
    s_app_voice_pending.user_id = 0U;
    s_app_voice_pending.act_id = (uint8_t)SRC_ACTION_DEVICE;
    s_app_voice_pending.start_tick = xTaskGetTickCount();
    if ((current >= APP_VOICE_ID_MIN) && (current <= APP_VOICE_ID_MAX)) {
        reported = app_voice_next_of(current);
        s_app_voice_pending.operation = APP_VOICE_OPERATION_SET;
        s_app_voice_pending.target = reported;
        send_result = aiio_ci13162_uart_send_voice_set(reported);
    } else {
        s_app_voice_pending.operation = APP_VOICE_OPERATION_BUTTON_QUERY;
        send_result = aiio_ci13162_uart_query_voice();
    }
    if (send_result != 0) {
        app_voice_clear_pending(0U);
        app_voice_publish(APP_VOICE_CMD_SET, reported, 0U, (uint8_t)SRC_ACTION_DEVICE,
                          0U, APP_VOICE_CODE_TX_FAIL);
    }
    return true;
}

bool app_voice_is_pending(void)
{
    return (s_app_voice_pending.active != 0U) ||
           (s_app_voice_draining != 0U);
}
