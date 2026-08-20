#include "app_cwsl.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include "aiio_ci13162_config.h"
#include "aiio_ci13162_uart.h"
#include "app_config.h"
#include "m_ci13162_ota.h"
#include "m_ota.h"
#include "process_data.h"
#include "system_typedef.h"

#define APP_CWSL_CMD_LEARN 220U
#define APP_CWSL_CMD_DELETE 221U
#define APP_CWSL_CMD_STOP 222U
#define APP_CWSL_CMD_QUERY 223U
#define APP_CWSL_CMD_VOICE_NOTIFY 224U

#define APP_CWSL_ACTION_LEARN 1U
#define APP_CWSL_ACTION_DELETE 2U
#define APP_CWSL_ACTION_STOP 4U
#define APP_CWSL_ACTION_QUERY 6U

#define APP_CWSL_CODE_LEARN_OK 32U
#define APP_CWSL_CODE_DELETE_OK 33U
#define APP_CWSL_CODE_TIMEOUT 52U
#define APP_CWSL_CODE_BAD_REQUEST 64U
#define APP_CWSL_CODE_BAD_COMMAND_ID 65U
#define APP_CWSL_CODE_BAD_SLOT 66U
#define APP_CWSL_CODE_BUSY 67U
#define APP_CWSL_CODE_NOT_IN_LEARN_MODE 68U
#define APP_CWSL_CODE_OTA_HOLD 69U
#define APP_CWSL_CODE_TX_FAIL 70U

#define APP_CWSL_ANY 255U
#define APP_CWSL_RESPONSE_SIZE 180U

typedef struct {
    uint32_t mqtt_cmd;
    uint32_t user_id;
    uint8_t act_id;
    uint8_t cmd_id;
    uint8_t slot;
} app_cwsl_request_t;

typedef struct {
    uint8_t active;
    uint8_t stop_sent;
    app_cwsl_request_t request;
    TickType_t start_tick;
} app_cwsl_mutating_t;

typedef struct {
    uint8_t active;
    app_cwsl_request_t request;
    TickType_t start_tick;
} app_cwsl_query_t;

static app_cwsl_mutating_t s_app_cwsl_mutating;
static app_cwsl_query_t s_app_cwsl_query;
static uint8_t s_app_cwsl_factory_clear_pending;

// Return true when any OTA path owns or is preparing to own the CI13162 UART.
static bool app_cwsl_ota_active(void)
{
    return aiio_ci13162_uart_ota_held() ||
           m_ci13162_ota_is_busy() ||
           m_ota_is_busy();
}

// Read one required non-negative integral JSON number within the requested range.
static bool app_cwsl_read_number(cJSON *js_root,
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

// Parse common request fields and reject the deprecated MQTT action field.
static uint8_t app_cwsl_parse_common(cJSON *js_root,
                                     uint32_t expected_cmd,
                                     app_cwsl_request_t *request)
{
    uint32_t act_id;

    if ((js_root == NULL) || (request == NULL)) {
        return APP_CWSL_CODE_BAD_REQUEST;
    }

    memset(request, 0, sizeof(*request));
    request->mqtt_cmd = expected_cmd;

    if (cJSON_GetObjectItem(js_root, CJSON_KEY_ACTION) != NULL) {
        return APP_CWSL_CODE_BAD_REQUEST;
    }
    if (!app_cwsl_read_number(js_root,
                              gs_dev_type[device_type],
                              UINT32_MAX,
                              &request->mqtt_cmd) ||
        (request->mqtt_cmd != expected_cmd) ||
        !app_cwsl_read_number(js_root,
                              CJSON_KEY_USER_ID,
                              UINT32_MAX,
                              &request->user_id) ||
        !app_cwsl_read_number(js_root,
                              CJSON_KEY_ACT_ID,
                              UINT8_MAX,
                              &act_id)) {
        request->mqtt_cmd = expected_cmd;
        return APP_CWSL_CODE_BAD_REQUEST;
    }

    request->act_id = (uint8_t)act_id;
    return 0U;
}

// Parse and validate a LEARN or DELETE request against the product allow-list.
static uint8_t app_cwsl_parse_mutating(cJSON *js_root,
                                       uint32_t mqtt_cmd,
                                       app_cwsl_request_t *request)
{
    uint32_t cmd_id;
    uint32_t slot;
    uint8_t code;

    code = app_cwsl_parse_common(js_root, mqtt_cmd, request);
    if (code != 0U) {
        return code;
    }
    if (!app_cwsl_read_number(js_root, "cmd_id", UINT8_MAX, &cmd_id) ||
        !app_cwsl_read_number(js_root, "slot", UINT8_MAX, &slot)) {
        return APP_CWSL_CODE_BAD_REQUEST;
    }

    request->cmd_id = (uint8_t)cmd_id;
    request->slot = (uint8_t)slot;

    if (request->cmd_id == APP_CWSL_ANY) {
        if (mqtt_cmd == APP_CWSL_CMD_LEARN) {
            return APP_CWSL_CODE_BAD_COMMAND_ID;
        }
        return (request->slot == APP_CWSL_ANY)
                   ? 0U
                   : APP_CWSL_CODE_BAD_REQUEST;
    }
    if ((request->cmd_id < 1U) || (request->cmd_id > 14U)) {
        return APP_CWSL_CODE_BAD_COMMAND_ID;
    }
    if (((request->cmd_id >= 2U) && (request->cmd_id <= 6U)) ||
        ((request->cmd_id >= 13U) && (request->cmd_id <= 14U))) {
        return APP_CWSL_CODE_BAD_REQUEST;
    }

    if (request->cmd_id == 1U) {
        return ((request->slot <= 4U) || (request->slot == APP_CWSL_ANY))
                   ? 0U
                   : APP_CWSL_CODE_BAD_SLOT;
    }
    return ((request->slot <= 14U) || (request->slot == APP_CWSL_ANY))
               ? 0U
               : APP_CWSL_CODE_BAD_SLOT;
}

// Parse and validate one read-only QUERY request.
static uint8_t app_cwsl_parse_query(cJSON *js_root,
                                    app_cwsl_request_t *request)
{
    uint32_t cmd_id;
    uint8_t code;

    code = app_cwsl_parse_common(js_root, APP_CWSL_CMD_QUERY, request);
    if (code != 0U) {
        return code;
    }
    if (!app_cwsl_read_number(js_root, "cmd_id", UINT8_MAX, &cmd_id)) {
        return APP_CWSL_CODE_BAD_REQUEST;
    }

    request->cmd_id = (uint8_t)cmd_id;
    if (((request->cmd_id >= 1U) && (request->cmd_id <= 14U)) ||
        (request->cmd_id == APP_CWSL_ANY)) {
        return 0U;
    }
    return APP_CWSL_CODE_BAD_COMMAND_ID;
}

// Publish a bounded CWSL JSON string through the existing MQTT and BLE path.
static void app_cwsl_publish(char *response, int response_length)
{
    if ((response == NULL) ||
        (response_length <= 0) ||
        ((size_t)response_length >= APP_CWSL_RESPONSE_SIZE)) {
        os_trace("[CWSL] response formatting failed\r\n");
        return;
    }

    app_send_data(response, (uint32_t)response_length);
}

// Publish a LEARN or DELETE acknowledgement.
static void app_cwsl_publish_mutating_reject(const app_cwsl_request_t *request,
                                             uint8_t code)
{
    char response[APP_CWSL_RESPONSE_SIZE];
    int response_length;

    if (request == NULL) {
        return;
    }

    response_length = snprintf(
        response,
        sizeof(response),
        "{\"%s\":%lu,\"cmd_id\":%u,\"slot\":%u,\"u\":%lu,"
        "\"act_id\":%u,\"result\":0,\"code\":%u}",
        gs_dev_type[device_type],
        (unsigned long)request->mqtt_cmd,
        (unsigned int)request->cmd_id,
        (unsigned int)request->slot,
        (unsigned long)request->user_id,
        (unsigned int)request->act_id,
        (unsigned int)code);

    app_cwsl_publish(response, response_length);
}

// Publish a STOP acknowledgement.
static void app_cwsl_publish_stop_reject(const app_cwsl_request_t *request,
                                         uint8_t code)
{
    char response[APP_CWSL_RESPONSE_SIZE];
    int response_length;

    if (request == NULL) {
        return;
    }

    response_length = snprintf(
        response,
        sizeof(response),
        "{\"%s\":222,\"u\":%lu,\"act_id\":%u,\"result\":0,\"code\":%u}",
        gs_dev_type[device_type],
        (unsigned long)request->user_id,
        (unsigned int)request->act_id,
        (unsigned int)code);

    app_cwsl_publish(response, response_length);
}

// Publish a QUERY acknowledgement.
static void app_cwsl_publish_query_reject(const app_cwsl_request_t *request,
                                          uint8_t code)
{
    char response[APP_CWSL_RESPONSE_SIZE];
    int response_length;

    if (request == NULL) {
        return;
    }

    response_length = snprintf(
        response,
        sizeof(response),
        "{\"%s\":223,\"cmd_id\":%u,\"u\":%lu,\"act_id\":%u,"
        "\"result\":0,\"code\":%u}",
        gs_dev_type[device_type],
        (unsigned int)request->cmd_id,
        (unsigned long)request->user_id,
        (unsigned int)request->act_id,
        (unsigned int)code);

    app_cwsl_publish(response, response_length);
}

// Clear mutating correlation and resume automatic version requests.
static void app_cwsl_clear_mutating(void)
{
    memset(&s_app_cwsl_mutating, 0, sizeof(s_app_cwsl_mutating));
    aiio_ci13162_uart_set_version_query_paused(0U);
}

// Send DELETE all after a factory-clear STOP has completed.
static void app_cwsl_finish_factory_clear(void)
{
    static const uint8_t delete_all[2] = {APP_CWSL_ANY, APP_CWSL_ANY};

    app_cwsl_clear_mutating();
    memset(&s_app_cwsl_query, 0, sizeof(s_app_cwsl_query));
    if (!app_cwsl_ota_active()) {
        (void)aiio_ci13162_uart_send_cwsl(APP_CWSL_ACTION_DELETE,
                                         delete_all,
                                         2U);
    }
    s_app_cwsl_factory_clear_pending = 0U;
}

// Publish one final mutating status without a result field.
static void app_cwsl_publish_mutating_final(
    const app_cwsl_mutating_t *pending,
    uint8_t cmd_id,
    uint8_t slot,
    uint8_t code)
{
    char response[APP_CWSL_RESPONSE_SIZE];
    uint32_t mqtt_cmd;
    int response_length;

    if (pending == NULL) {
        return;
    }

    mqtt_cmd = (pending->stop_sent != 0U)
                   ? APP_CWSL_CMD_STOP
                   : pending->request.mqtt_cmd;
    response_length = snprintf(
        response,
        sizeof(response),
        "{\"%s\":%lu,\"cmd_id\":%u,\"slot\":%u,\"code\":%u,"
        "\"u\":%lu,\"act_id\":%u}",
        gs_dev_type[device_type],
        (unsigned long)mqtt_cmd,
        (unsigned int)cmd_id,
        (unsigned int)slot,
        (unsigned int)code,
        (unsigned long)pending->request.user_id,
        (unsigned int)pending->request.act_id);
    app_cwsl_publish(response, response_length);
}

// Publish one final QUERY timeout without bitmap or capacity fields.
static void app_cwsl_publish_query_failure(const app_cwsl_query_t *pending,
                                           uint8_t code)
{
    char response[APP_CWSL_RESPONSE_SIZE];
    int response_length;

    if (pending == NULL) {
        return;
    }

    response_length = snprintf(
        response,
        sizeof(response),
        "{\"%s\":223,\"cmd_id\":%u,\"code\":%u,\"u\":%lu,"
        "\"act_id\":%u}",
        gs_dev_type[device_type],
        (unsigned int)pending->request.cmd_id,
        (unsigned int)code,
        (unsigned long)pending->request.user_id,
        (unsigned int)pending->request.act_id);
    app_cwsl_publish(response, response_length);
}

// Deliver one matching cmd5 status to the pending mutating request.
static void app_cwsl_on_status(uint8_t code, uint8_t cmd_id, uint8_t slot)
{
    app_cwsl_mutating_t pending;
    uint8_t status_class;

    if (s_app_cwsl_mutating.active == 0U) {
        return;
    }

    if (s_app_cwsl_mutating.stop_sent == 0U) {
        if ((cmd_id != s_app_cwsl_mutating.request.cmd_id) ||
            ((s_app_cwsl_mutating.request.slot != APP_CWSL_ANY) &&
             (slot != s_app_cwsl_mutating.request.slot))) {
            return;
        }

        status_class = (uint8_t)(code >> 4U);
        if (status_class == 2U) {
            if (((code == APP_CWSL_CODE_LEARN_OK) &&
                 (s_app_cwsl_mutating.request.mqtt_cmd == APP_CWSL_CMD_LEARN)) ||
                ((code == APP_CWSL_CODE_DELETE_OK) &&
                 (s_app_cwsl_mutating.request.mqtt_cmd == APP_CWSL_CMD_DELETE))) {
                /* Expected successful terminal status. */
            } else {
                return;
            }
        } else if ((status_class != 3U) && (status_class != 4U)) {
            return;
        }
    }

    pending = s_app_cwsl_mutating;
    if (s_app_cwsl_factory_clear_pending != 0U) {
        app_cwsl_finish_factory_clear();
        return;
    }

    app_cwsl_clear_mutating();
    app_cwsl_publish_mutating_final(&pending, cmd_id, slot, code);
}

// Deliver one matching cmd6 response to the pending query request.
static void app_cwsl_on_info(uint8_t cmd_id,
                             const uint8_t *data,
                             uint8_t data_count)
{
    app_cwsl_query_t pending;
    char response[APP_CWSL_RESPONSE_SIZE];
    int response_length;

    if ((s_app_cwsl_query.active == 0U) ||
        (cmd_id != s_app_cwsl_query.request.cmd_id) ||
        (data == NULL)) {
        return;
    }
    if (((cmd_id == APP_CWSL_ANY) && (data_count != 3U)) ||
        ((cmd_id != APP_CWSL_ANY) && (data_count != 2U))) {
        return;
    }

    pending = s_app_cwsl_query;
    memset(&s_app_cwsl_query, 0, sizeof(s_app_cwsl_query));

    if (cmd_id == APP_CWSL_ANY) {
        response_length = snprintf(
            response,
            sizeof(response),
            "{\"%s\":223,\"cmd_id\":255,\"wake\":%u,\"ncmd\":%u,"
            "\"nfree\":%u,\"u\":%lu,\"act_id\":%u}",
            gs_dev_type[device_type],
            (unsigned int)data[0],
            (unsigned int)data[1],
            (unsigned int)data[2],
            (unsigned long)pending.request.user_id,
            (unsigned int)pending.request.act_id);
    } else {
        uint16_t bitmap = (uint16_t)data[0] | ((uint16_t)data[1] << 8U);

        response_length = snprintf(
            response,
            sizeof(response),
            "{\"%s\":223,\"cmd_id\":%u,\"bitmap\":%u,\"u\":%lu,"
            "\"act_id\":%u}",
            gs_dev_type[device_type],
            (unsigned int)cmd_id,
            (unsigned int)bitmap,
            (unsigned long)pending.request.user_id,
            (unsigned int)pending.request.act_id);
    }

    app_cwsl_publish(response, response_length);
}

#if defined(BUILD_TEST) && (BUILD_TEST == 1)

// Run validation and policy regression cases without UART or MQTT side effects.
static void app_cwsl_run_validation_tests(void)
{
    static const struct {
        const char *fields;
        uint32_t mqtt_cmd;
        uint8_t expected_code;
    } test_cases[] = {
        {"\"cmd_id\":7,\"slot\":255,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_LEARN, 0U},
        {"\"cmd_id\":1,\"slot\":4,\"u\":1,\"act_id\":255",
         APP_CWSL_CMD_LEARN, 0U},
        {"\"cmd_id\":2,\"slot\":0,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_LEARN, APP_CWSL_CODE_BAD_REQUEST},
        {"\"cmd_id\":13,\"slot\":0,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_LEARN, APP_CWSL_CODE_BAD_REQUEST},
        {"\"cmd_id\":99,\"slot\":0,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_LEARN, APP_CWSL_CODE_BAD_COMMAND_ID},
        {"\"cmd_id\":7,\"slot\":15,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_LEARN, APP_CWSL_CODE_BAD_SLOT},
        {"\"cmd_id\":1,\"slot\":5,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_LEARN, APP_CWSL_CODE_BAD_SLOT},
        {"\"cmd_id\":7,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_LEARN, APP_CWSL_CODE_BAD_REQUEST},
        {"\"cmd_id\":7,\"slot\":0.5,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_LEARN, APP_CWSL_CODE_BAD_REQUEST},
        {"\"cmd_id\":7,\"slot\":0,\"u\":1,\"act_id\":0,\"action\":1",
         APP_CWSL_CMD_LEARN, APP_CWSL_CODE_BAD_REQUEST},
        {"\"cmd_id\":255,\"slot\":255,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_DELETE, 0U},
        {"\"cmd_id\":255,\"slot\":0,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_DELETE, APP_CWSL_CODE_BAD_REQUEST},
        {"\"cmd_id\":13,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_QUERY, 0U},
        {"\"cmd_id\":99,\"u\":1,\"act_id\":0",
         APP_CWSL_CMD_QUERY, APP_CWSL_CODE_BAD_COMMAND_ID},
        {"\"cmd_id\":255,\"u\":1,\"act_id\":\"0\"",
         APP_CWSL_CMD_QUERY, APP_CWSL_CODE_BAD_REQUEST},
    };
    char json[160];
    uint32_t failed_count = 0U;
    size_t i;

    for (i = 0U; i < (sizeof(test_cases) / sizeof(test_cases[0])); i++) {
        app_cwsl_request_t request;
        cJSON *js_root;
        uint8_t code;
        int json_length;

        json_length = snprintf(json,
                               sizeof(json),
                               "{\"%s\":%lu,%s}",
                               gs_dev_type[device_type],
                               (unsigned long)test_cases[i].mqtt_cmd,
                               test_cases[i].fields);
        if ((json_length <= 0) || ((size_t)json_length >= sizeof(json))) {
            failed_count++;
            continue;
        }

        js_root = cJSON_Parse(json);
        if (js_root == NULL) {
            failed_count++;
            continue;
        }

        if (test_cases[i].mqtt_cmd == APP_CWSL_CMD_QUERY) {
            code = app_cwsl_parse_query(js_root, &request);
        } else {
            code = app_cwsl_parse_mutating(js_root,
                                           test_cases[i].mqtt_cmd,
                                           &request);
        }
        cJSON_Delete(js_root);
        if (code != test_cases[i].expected_code) {
            failed_count++;
        }
    }

    os_trace("[CWSL][TEST] validation total=%u failed=%lu\r\n",
             (unsigned int)(sizeof(test_cases) / sizeof(test_cases[0])),
             (unsigned long)failed_count);
}

#endif

// Execute one validated mutating request.
static void app_cwsl_start_mutating(cJSON *js_root,
                                    uint32_t mqtt_cmd,
                                    uint8_t action)
{
    app_cwsl_request_t request;
    uint8_t uart_data[2];
    uint8_t code;

    memset(&request, 0, sizeof(request));
    request.mqtt_cmd = mqtt_cmd;
    code = app_cwsl_parse_mutating(js_root, mqtt_cmd, &request);
    if (code != 0U) {
        app_cwsl_publish_mutating_reject(&request, code);
        return;
    }
    if (app_cwsl_ota_active()) {
        app_cwsl_publish_mutating_reject(&request, APP_CWSL_CODE_OTA_HOLD);
        return;
    }
    if ((s_app_cwsl_mutating.active != 0U) ||
        (s_app_cwsl_query.active != 0U)) {
        app_cwsl_publish_mutating_reject(&request, APP_CWSL_CODE_BUSY);
        return;
    }

    uart_data[0] = request.cmd_id;
    uart_data[1] = request.slot;
    aiio_ci13162_uart_set_version_query_paused(1U);
    if (aiio_ci13162_uart_send_cwsl(action, uart_data, 2U) != 0) {
        aiio_ci13162_uart_set_version_query_paused(0U);
        app_cwsl_publish_mutating_reject(&request, APP_CWSL_CODE_TX_FAIL);
        return;
    }

    s_app_cwsl_mutating.active = 1U;
    s_app_cwsl_mutating.stop_sent = 0U;
    s_app_cwsl_mutating.request = request;
    s_app_cwsl_mutating.start_tick = xTaskGetTickCount();
}

void app_cwsl_init(void)
{
    memset(&s_app_cwsl_mutating, 0, sizeof(s_app_cwsl_mutating));
    memset(&s_app_cwsl_query, 0, sizeof(s_app_cwsl_query));
    s_app_cwsl_factory_clear_pending = 0U;
    aiio_ci13162_uart_set_cwsl_callbacks(app_cwsl_on_status,
                                         app_cwsl_on_info);
    aiio_ci13162_uart_set_version_query_paused(0U);
#if defined(BUILD_TEST) && (BUILD_TEST == 1)
    app_cwsl_run_validation_tests();
#endif
}

void app_cwsl_learn(cJSON *js_root)
{
    app_cwsl_start_mutating(js_root,
                            APP_CWSL_CMD_LEARN,
                            APP_CWSL_ACTION_LEARN);
}

void app_cwsl_delete(cJSON *js_root)
{
    app_cwsl_start_mutating(js_root,
                            APP_CWSL_CMD_DELETE,
                            APP_CWSL_ACTION_DELETE);
}

void app_cwsl_stop(cJSON *js_root)
{
    app_cwsl_request_t request;
    uint8_t code;

    memset(&request, 0, sizeof(request));
    request.mqtt_cmd = APP_CWSL_CMD_STOP;
    code = app_cwsl_parse_common(js_root, APP_CWSL_CMD_STOP, &request);
    if (code != 0U) {
        app_cwsl_publish_stop_reject(&request, code);
        return;
    }
    if (app_cwsl_ota_active()) {
        app_cwsl_publish_stop_reject(&request, APP_CWSL_CODE_OTA_HOLD);
        return;
    }
    if (s_app_cwsl_mutating.active == 0U) {
        code = (s_app_cwsl_query.active != 0U)
                   ? APP_CWSL_CODE_BUSY
                   : APP_CWSL_CODE_NOT_IN_LEARN_MODE;
        app_cwsl_publish_stop_reject(&request, code);
        return;
    }
    if (s_app_cwsl_mutating.stop_sent != 0U) {
        app_cwsl_publish_stop_reject(&request, APP_CWSL_CODE_BUSY);
        return;
    }
    if (aiio_ci13162_uart_send_cwsl(APP_CWSL_ACTION_STOP, NULL, 0U) != 0) {
        app_cwsl_publish_stop_reject(&request, APP_CWSL_CODE_TX_FAIL);
        return;
    }

    s_app_cwsl_mutating.stop_sent = 1U;
    s_app_cwsl_mutating.request.user_id = request.user_id;
    s_app_cwsl_mutating.request.act_id = request.act_id;
    s_app_cwsl_mutating.start_tick = xTaskGetTickCount();
}

void app_cwsl_query(cJSON *js_root)
{
    app_cwsl_request_t request;
    uint8_t code;

    memset(&request, 0, sizeof(request));
    request.mqtt_cmd = APP_CWSL_CMD_QUERY;
    code = app_cwsl_parse_query(js_root, &request);
    if (code != 0U) {
        app_cwsl_publish_query_reject(&request, code);
        return;
    }
    if (app_cwsl_ota_active()) {
        app_cwsl_publish_query_reject(&request, APP_CWSL_CODE_OTA_HOLD);
        return;
    }
    if (s_app_cwsl_query.active != 0U) {
        app_cwsl_publish_query_reject(&request, APP_CWSL_CODE_BUSY);
        return;
    }
    if (aiio_ci13162_uart_send_cwsl(APP_CWSL_ACTION_QUERY,
                                    &request.cmd_id,
                                    1U) != 0) {
        app_cwsl_publish_query_reject(&request, APP_CWSL_CODE_TX_FAIL);
        return;
    }

    s_app_cwsl_query.active = 1U;
    s_app_cwsl_query.request = request;
    s_app_cwsl_query.start_tick = xTaskGetTickCount();
}

void app_cwsl_poll(void)
{
    TickType_t current_tick = xTaskGetTickCount();

    if ((s_app_cwsl_mutating.active != 0U) &&
        ((TickType_t)(current_tick - s_app_cwsl_mutating.start_tick) >=
         pdMS_TO_TICKS(AIIO_CI13162_CWSL_MUTATE_TIMEOUT_MS))) {
        if (s_app_cwsl_factory_clear_pending != 0U) {
            app_cwsl_finish_factory_clear();
        } else {
            app_cwsl_mutating_t pending = s_app_cwsl_mutating;

            app_cwsl_clear_mutating();
            app_cwsl_publish_mutating_final(&pending,
                                            pending.request.cmd_id,
                                            pending.request.slot,
                                            APP_CWSL_CODE_TIMEOUT);
        }
    }

    if ((s_app_cwsl_query.active != 0U) &&
        ((TickType_t)(current_tick - s_app_cwsl_query.start_tick) >=
         pdMS_TO_TICKS(AIIO_CI13162_CWSL_QUERY_TIMEOUT_MS))) {
        app_cwsl_query_t pending = s_app_cwsl_query;

        memset(&s_app_cwsl_query, 0, sizeof(s_app_cwsl_query));
        app_cwsl_publish_query_failure(&pending, APP_CWSL_CODE_TIMEOUT);
    }
}

bool app_cwsl_is_pending(void)
{
    return (s_app_cwsl_mutating.active != 0U) ||
           (s_app_cwsl_query.active != 0U);
}

bool app_cwsl_factory_clear(void)
{
    static const uint8_t delete_all[2] = {APP_CWSL_ANY, APP_CWSL_ANY};

    if (app_cwsl_ota_active()) {
        app_cwsl_clear_mutating();
        memset(&s_app_cwsl_query, 0, sizeof(s_app_cwsl_query));
        s_app_cwsl_factory_clear_pending = 0U;
        return true;
    }

    memset(&s_app_cwsl_query, 0, sizeof(s_app_cwsl_query));
    if (s_app_cwsl_mutating.active != 0U) {
        if (s_app_cwsl_mutating.stop_sent == 0U) {
            if (aiio_ci13162_uart_send_cwsl(APP_CWSL_ACTION_STOP,
                                            NULL,
                                            0U) != 0) {
                app_cwsl_clear_mutating();
                return true;
            }
            s_app_cwsl_mutating.stop_sent = 1U;
            s_app_cwsl_mutating.start_tick = xTaskGetTickCount();
        }
        s_app_cwsl_factory_clear_pending = 1U;
        return false;
    }

    (void)aiio_ci13162_uart_send_cwsl(APP_CWSL_ACTION_DELETE,
                                     delete_all,
                                     2U);
    s_app_cwsl_factory_clear_pending = 0U;
    return true;
}

bool app_cwsl_factory_clear_done(void)
{
    return s_app_cwsl_factory_clear_pending == 0U;
}

void app_cwsl_notify_voice_cmd(uint8_t cmd_id)
{
    char response[APP_CWSL_RESPONSE_SIZE];
    int response_length;

    if (cmd_id == 0U) {
        return;
    }

    response_length = snprintf(
        response,
        sizeof(response),
        "{\"%s\":%u,\"cmd_id\":%u}",
        gs_dev_type[device_type],
        (unsigned int)APP_CWSL_CMD_VOICE_NOTIFY,
        (unsigned int)cmd_id);

    app_cwsl_publish(response, response_length);
}

