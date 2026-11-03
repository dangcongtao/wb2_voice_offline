#include "app_volume.h"

#include <limits.h>
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
#include "app_voice.h"
#include "m_ci13162_ota.h"
#include "m_ota.h"
#include "process_data.h"
#include "system_typedef.h"

#define APP_VOLUME_CMD_SET 225U
#define APP_VOLUME_CMD_GET 226U
#define APP_VOLUME_RESPONSE_SIZE 128U
#define APP_VOLUME_LEVEL_MIN 1U
#define APP_VOLUME_LEVEL_MAX 7U

typedef enum {
    APP_VOLUME_OPERATION_NONE = 0,
    APP_VOLUME_OPERATION_MQTT_SET,
    APP_VOLUME_OPERATION_MQTT_GET,
} app_volume_operation_t;

typedef struct {
    uint8_t active;
    app_volume_operation_t operation;
    uint8_t target;
    uint32_t mqtt_cmd;
    uint32_t user_id;
    uint8_t act_id;
    TickType_t start_tick;
} app_volume_pending_t;

static app_volume_pending_t s_app_volume_pending;
static uint8_t s_app_volume_draining;
static TickType_t s_app_volume_drain_tick;

// Return true while an OTA path owns or is preparing to own the voice UART.
static bool app_volume_ota_active(void)
{
    return aiio_ci13162_uart_ota_held() ||
           m_ci13162_ota_is_busy() ||
           m_ota_is_busy();
}

// Read one required non-negative integral JSON number within the requested range.
static bool app_volume_read_number(cJSON *js_root,
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

// Publish one bounded volume response through the existing MQTT and BLE path.
static void app_volume_publish(uint32_t mqtt_cmd,
                               uint8_t level,
                               uint32_t user_id,
                               uint8_t act_id,
                               uint8_t result)
{
    char response[APP_VOLUME_RESPONSE_SIZE];
    int response_length;

    response_length = snprintf(
        response,
        sizeof(response),
        "{\"%s\":%lu,\"vol\":%u,\"u\":%lu,\"act_id\":%u,\"result\":%u}",
        gs_dev_type[device_type],
        (unsigned long)mqtt_cmd,
        (unsigned int)level,
        (unsigned long)user_id,
        (unsigned int)act_id,
        (unsigned int)result);
    if ((response_length <= 0) ||
        ((size_t)response_length >= sizeof(response))) {
        os_trace("[VOLUME] response formatting failed\r\n");
        return;
    }

    app_send_data(response, (uint32_t)response_length);
}

// Return the latest cached level, or the supplied fallback when unavailable.
static uint8_t app_volume_cached_or(uint8_t fallback)
{
    uint8_t level;

    return (aiio_ci13162_uart_get_volume(&level) == 0) ? level : fallback;
}

// Clear the current request and optionally reserve a drain window for a late reply.
static void app_volume_clear_pending(uint8_t drain_late_response)
{
    memset(&s_app_volume_pending, 0, sizeof(s_app_volume_pending));
    if (drain_late_response != 0U) {
        s_app_volume_draining = 1U;
        s_app_volume_drain_tick = xTaskGetTickCount();
    }
}

// Complete the MQTT request that owns the received volume response.
static void app_volume_on_info(uint8_t level, uint8_t result)
{
    app_volume_pending_t pending;

    if (s_app_volume_draining != 0U) {
        s_app_volume_draining = 0U;
        return;
    }
    if (s_app_volume_pending.active == 0U) {
        return;
    }

    pending = s_app_volume_pending;
    app_volume_clear_pending(0U);
    if (pending.operation == APP_VOLUME_OPERATION_MQTT_SET) {
        app_volume_publish(pending.mqtt_cmd,
                           level,
                           pending.user_id,
                           pending.act_id,
                           ((result != 0U) && (level == pending.target)) ? 1U : 0U);
    } else if (pending.operation == APP_VOLUME_OPERATION_MQTT_GET) {
        app_volume_publish(pending.mqtt_cmd,
                           level,
                           pending.user_id,
                           pending.act_id,
                           (result != 0U) ? 1U : 0U);
    }
}

// Return true while another UART owner or volume transaction is already active.
static bool app_volume_busy(void)
{
    return (s_app_volume_pending.active != 0U) ||
           (s_app_volume_draining != 0U) ||
           app_cwsl_is_pending() ||
           app_voice_is_pending() ||
           app_volume_ota_active();
}

// Start one absolute MQTT set or read-through query.
static void app_volume_start_mqtt(cJSON *js_root, uint32_t mqtt_cmd)
{
    uint32_t parsed_cmd = mqtt_cmd;
    uint32_t user_id = 0U;
    uint32_t act_id = 0U;
    uint32_t level = 0U;
    uint8_t request_value = 0U;
    uint8_t valid;
    int32_t send_result;

    valid = (app_volume_read_number(js_root,
                                    gs_dev_type[device_type],
                                    UINT32_MAX,
                                    &parsed_cmd) &&
             (parsed_cmd == mqtt_cmd) &&
             app_volume_read_number(js_root,
                                    CJSON_KEY_USER_ID,
                                    UINT32_MAX,
                                    &user_id) &&
             app_volume_read_number(js_root,
                                    CJSON_KEY_ACT_ID,
                                    UINT8_MAX,
                                    &act_id))
                ? 1U
                : 0U;

    if (mqtt_cmd == APP_VOLUME_CMD_SET) {
        if (!app_volume_read_number(js_root,
                                    "vol",
                                    APP_VOLUME_LEVEL_MAX,
                                    &level) ||
            (level < APP_VOLUME_LEVEL_MIN)) {
            valid = 0U;
        }
        request_value = (uint8_t)level;
    }

    if ((valid == 0U) || app_volume_busy()) {
        app_volume_publish(mqtt_cmd,
                           app_volume_cached_or((uint8_t)level),
                           user_id,
                           (uint8_t)act_id,
                           0U);
        return;
    }

    s_app_volume_pending.active = 1U;
    s_app_volume_pending.operation = (mqtt_cmd == APP_VOLUME_CMD_SET)
                                         ? APP_VOLUME_OPERATION_MQTT_SET
                                         : APP_VOLUME_OPERATION_MQTT_GET;
    s_app_volume_pending.target = request_value;
    s_app_volume_pending.mqtt_cmd = mqtt_cmd;
    s_app_volume_pending.user_id = user_id;
    s_app_volume_pending.act_id = (uint8_t)act_id;
    s_app_volume_pending.start_tick = xTaskGetTickCount();

    send_result = (mqtt_cmd == APP_VOLUME_CMD_SET)
                      ? aiio_ci13162_uart_send_volume(request_value)
                      : aiio_ci13162_uart_query_volume();
    if (send_result != 0) {
        app_volume_clear_pending(0U);
        app_volume_publish(mqtt_cmd,
                           app_volume_cached_or((uint8_t)level),
                           user_id,
                           (uint8_t)act_id,
                           0U);
    }
}

void app_volume_init(void)
{
    memset(&s_app_volume_pending, 0, sizeof(s_app_volume_pending));
    s_app_volume_draining = 0U;
    s_app_volume_drain_tick = 0U;
    aiio_ci13162_uart_set_volume_callback(app_volume_on_info);
}

void app_volume_set(cJSON *js_root)
{
    app_volume_start_mqtt(js_root, APP_VOLUME_CMD_SET);
}

void app_volume_get(cJSON *js_root)
{
    app_volume_start_mqtt(js_root, APP_VOLUME_CMD_GET);
}

void app_volume_poll(void)
{
    TickType_t current_tick = xTaskGetTickCount();

    if ((s_app_volume_pending.active != 0U) &&
        ((TickType_t)(current_tick - s_app_volume_pending.start_tick) >=
         pdMS_TO_TICKS(AIIO_CI13162_VOLUME_TIMEOUT_MS))) {
        app_volume_pending_t pending = s_app_volume_pending;

        app_volume_clear_pending(1U);
        app_volume_publish(pending.mqtt_cmd,
                           app_volume_cached_or(pending.target),
                           pending.user_id,
                           pending.act_id,
                           0U);
    }

    if ((s_app_volume_draining != 0U) &&
        ((TickType_t)(current_tick - s_app_volume_drain_tick) >=
         pdMS_TO_TICKS(AIIO_CI13162_VOLUME_DRAIN_MS))) {
        s_app_volume_draining = 0U;
    }
}

bool app_volume_is_pending(void)
{
    return (s_app_volume_pending.active != 0U) ||
           (s_app_volume_draining != 0U);
}

// Publish MQTT 225 for a voice-driven volume change without completing a pending SET or GET.
void app_volume_report_voice(uint8_t level, uint8_t cmd_id, uint8_t cwsl)
{
    char response[APP_VOLUME_RESPONSE_SIZE];
    int response_length;

    if (level > APP_VOLUME_LEVEL_MAX) {
        return;
    }

    response_length = snprintf(
        response,
        sizeof(response),
        "{\"%s\":225,\"vol\":%u,\"u\":0,\"act_id\":%u,\"result\":1,\"cmd_id\":%u,\"cwsl\":%u}",
        gs_dev_type[device_type],
        (unsigned int)level,
        (unsigned int)SRC_ACTION_VOICE,
        (unsigned int)cmd_id,
        (unsigned int)cwsl);
    if ((response_length <= 0) ||
        ((size_t)response_length >= sizeof(response))) {
        os_trace("[VOLUME] voice response formatting failed\r\n");
        return;
    }

    app_send_data(response, (uint32_t)response_length);
}
