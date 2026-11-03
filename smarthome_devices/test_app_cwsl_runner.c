#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define os_trace printf

/* Guard unwanted SDK headers */
#define _USER_CONFIG_H_
#define __EASY_FLASH_COMMON_H__
#define _APP_MQTT_H__
#define _APP_MQTT_H_

#define BUILD_TEST 1

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"

#include "components/app_config/system_typedef.h"
#include "components/third_party/cJson/cJSON.c"
#include "components/hardware/aiio_ci13162_uart.h"

#define CJSON_KEY_ACTION "action"
#define CJSON_KEY_USER_ID "u"
#define CJSON_KEY_ACT_ID "act_id"

char *gs_dev_type[] = {"fanoem", "lhswitch3v", "swminiv2", "sswitch2v", "swshockv2", "swshohuv2", "fanoem", "fandc", "fanac", "fanproacv2", "voiceboxv1"};
uint32_t device_type = 0;

static char s_last_published[256];
static int s_publish_count = 0;

void app_send_data(char *data, uint32_t len)
{
    (void)len;
    strncpy(s_last_published, data, sizeof(s_last_published) - 1);
    s_last_published[sizeof(s_last_published) - 1] = '\0';
    s_publish_count++;
}

static uint8_t s_mock_uart_action = 0;
static uint8_t s_mock_uart_data[4] = {0};
static uint8_t s_mock_uart_data_count = 0;
static int s_uart_send_count = 0;
static int s_mock_uart_ret = 0;

int32_t aiio_ci13162_uart_send_cwsl(uint8_t action, const uint8_t *data, uint8_t data_count)
{
    s_mock_uart_action = action;
    s_mock_uart_data_count = data_count;
    if (data != NULL && data_count <= sizeof(s_mock_uart_data)) {
        memcpy(s_mock_uart_data, data, data_count);
    }
    s_uart_send_count++;
    return s_mock_uart_ret;
}

static aiio_ci13162_cwsl_status_cb_t s_registered_status_cb = NULL;
static aiio_ci13162_cwsl_info_cb_t s_registered_info_cb = NULL;

void aiio_ci13162_uart_set_cwsl_callbacks(aiio_ci13162_cwsl_status_cb_t status_cb,
                                          aiio_ci13162_cwsl_info_cb_t info_cb)
{
    s_registered_status_cb = status_cb;
    s_registered_info_cb = info_cb;
}

static int s_resp_set_send_count = 0;
static uint8_t s_resp_set_last_cmd_id = 0;
static uint8_t s_resp_set_last_value = 0;
static int32_t s_mock_resp_set_ret = 0;
int32_t aiio_ci13162_uart_send_resp_set(uint8_t cmd_id, uint8_t value)
{
    s_resp_set_send_count++;
    s_resp_set_last_cmd_id = cmd_id;
    s_resp_set_last_value = value;
    return s_mock_resp_set_ret;
}

static aiio_ci13162_resp_set_cb_t s_registered_resp_set_cb = NULL;
void aiio_ci13162_uart_set_resp_set_callback(aiio_ci13162_resp_set_cb_t cb)
{
    s_registered_resp_set_cb = cb;
}

static int s_scene_resp_set_result_count = 0;
static uint8_t s_scene_resp_set_result_code = 0;
void app_scene_on_resp_set_result(uint8_t code)
{
    s_scene_resp_set_result_count++;
    s_scene_resp_set_result_code = code;
}

void aiio_ci13162_uart_set_version_query_paused(uint8_t paused)
{
    (void)paused;
}

bool aiio_ci13162_uart_ota_held(void)
{
    return false;
}

bool m_ci13162_ota_is_busy(void)
{
    return false;
}

bool m_ota_is_busy(void)
{
    return false;
}

bool app_volume_is_pending(void)
{
    return false;
}

static bool s_mock_voice_pending = false;

bool app_voice_is_pending(void)
{
    return s_mock_voice_pending;
}

bool app_scene_is_pending(void)
{
    return false;
}

static bool s_mock_scene_active = true;
bool app_scene_is_cmd_id_active(uint8_t cmd_id)
{
    (void)cmd_id;
    return s_mock_scene_active;
}

static int s_scene_learn_result_count = 0;
static uint8_t s_scene_learn_result_code = 0;
static uint8_t s_scene_learn_result_cmd_id = 0;
void app_scene_on_learn_result(uint8_t code, uint8_t cmd_id)
{
    s_scene_learn_result_count++;
    s_scene_learn_result_code = code;
    s_scene_learn_result_cmd_id = cmd_id;
}

void app_scene_on_delete_result(uint8_t code, uint8_t cmd_id)
{
    (void)code;
    (void)cmd_id;
}

static int s_scene_clear_all_count = 0;
void app_scene_clear_all(void)
{
    s_scene_clear_all_count++;
}

void m_fan_need_buzz(uint8_t num_buzz)
{
    (void)num_buzz;
}

static uint32_t s_mock_led_pulse_duration = 0;
static int s_mock_led_pulse_count = 0;
void m_led_stt_flash_pulse(uint32_t duration_ms)
{
    s_mock_led_pulse_duration = duration_ms;
    s_mock_led_pulse_count++;
}

static uint8_t s_mock_ble_beacon_cmd_id = 0;
static int s_mock_ble_beacon_count = 0;
void m_ble_send_voicebox_state(uint8_t cmd_id)
{
    s_mock_ble_beacon_cmd_id = cmd_id;
    s_mock_ble_beacon_count++;
}

void vTaskDelay(TickType_t xTicksToDelay) { (void)xTicksToDelay; }
static TickType_t s_mock_tick = 0;
TickType_t xTaskGetTickCount(void) { return s_mock_tick; }

/* Include the actual CWSL implementation */
#include "components/application/app_cwsl.c"

static void reset_cwsl_mocks(void)
{
    s_last_published[0] = '\0';
    s_publish_count = 0;
    s_mock_uart_action = 0;
    memset(s_mock_uart_data, 0, sizeof(s_mock_uart_data));
    s_mock_uart_data_count = 0;
    s_uart_send_count = 0;
    s_scene_clear_all_count = 0;
    s_scene_learn_result_count = 0;
    s_scene_learn_result_code = 0;
    s_scene_learn_result_cmd_id = 0;
    s_mock_uart_ret = 0;
    s_mock_tick = 0;
    s_mock_led_pulse_duration = 0;
    s_mock_led_pulse_count = 0;
    s_mock_ble_beacon_cmd_id = 0;
    s_mock_ble_beacon_count = 0;
    s_resp_set_send_count = 0;
    s_resp_set_last_cmd_id = 0;
    s_resp_set_last_value = 0;
    s_mock_resp_set_ret = 0;
    s_scene_resp_set_result_count = 0;
    s_scene_resp_set_result_code = 0;
    s_mock_scene_active = true;
}

/* Drop CWSL sessions left by earlier tests (app_cwsl_init would rerun the validation suite). */
static void reset_cwsl_sessions(void)
{
    memset(&s_app_cwsl_mutating, 0, sizeof(s_app_cwsl_mutating));
    memset(&s_app_cwsl_query, 0, sizeof(s_app_cwsl_query));
    memset(&s_app_cwsl_resp_set, 0, sizeof(s_app_cwsl_resp_set));
}

int main(void)
{
    printf("[TEST] Running native app_cwsl test suite...\n");

    /* TEST 1: Initialization and internal validation tests */
    {
        printf("--- Test 1: Init & Internal Validation ---\n");
        reset_cwsl_mocks();
        app_cwsl_init();
        assert(s_registered_status_cb != NULL);
        assert(s_registered_info_cb != NULL);
        printf("  PASS: CWSL init and built-in validation suite passed\n");
    }

    /* TEST 2: Mutating LEARN requests */
    {
        printf("--- Test 2: LEARN Requests (Range & Validation) ---\n");
        reset_cwsl_mocks();
        app_cwsl_init();

        /* Valid LEARN on fan turn on (7), lowest slot (255) */
        cJSON *req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":7,\"slot\":255,\"u\":1001,\"act_id\":1}");
        app_cwsl_learn(req);
        cJSON_Delete(req);

        assert(s_uart_send_count == 1);
        assert(s_mock_uart_action == APP_CWSL_ACTION_LEARN);
        assert(s_mock_uart_data_count == 2);
        assert(s_mock_uart_data[0] == 7U);
        assert(s_mock_uart_data[1] == 255U);
        assert(s_publish_count == 0); /* No MQTT ack until chip status returns */

        /* Callback status LEARN_OK (32) arrives from chip */
        s_registered_status_cb(32U, 7U, 0U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"fanoem\":220") != NULL);
        assert(strstr(s_last_published, "\"code\":32") != NULL);
        assert(strstr(s_last_published, "\"cmd_id\":7") != NULL);
        assert(strstr(s_last_published, "\"slot\":0") != NULL);
        assert(strstr(s_last_published, "\"u\":1001") != NULL);
        assert(strstr(s_last_published, "\"act_id\":1") != NULL);

        /* Mute (54) and Unmute (55) must be rejected with BAD_COMMAND_ID (65) */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":54,\"slot\":0,\"u\":1002,\"act_id\":2}");
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":65") != NULL);
        assert(strstr(s_last_published, "\"result\":0") != NULL);

        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":55,\"slot\":255,\"u\":1003,\"act_id\":3}");
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":65") != NULL);
        assert(strstr(s_last_published, "\"result\":0") != NULL);

        /* Scene commands (56..70) must be rejected with BAD_REQUEST (64) */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":56,\"slot\":255,\"u\":1004,\"act_id\":4}");
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":64") != NULL);
        assert(strstr(s_last_published, "\"result\":0") != NULL);

        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":65,\"slot\":255,\"u\":1005,\"act_id\":5}");
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":64") != NULL);

        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":66,\"slot\":0,\"u\":1006,\"act_id\":6}");
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":64") != NULL);

        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":70,\"slot\":255,\"u\":1007,\"act_id\":7}");
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":64") != NULL);

        /* Out-of-bounds commands (> 70) must be rejected with BAD_COMMAND_ID (65) */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":71,\"slot\":0,\"u\":1008,\"act_id\":8}");
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":65") != NULL);

        printf("  PASS: LEARN command validations and rejections verified\n");
    }

    /* TEST 3: DELETE Requests */
    {
        printf("--- Test 3: DELETE Requests ---\n");
        reset_cwsl_mocks();
        app_cwsl_init();

        /* Delete specific slot */
        cJSON *req = cJSON_Parse("{\"fanoem\":221,\"cmd_id\":7,\"slot\":0,\"u\":2001,\"act_id\":1}");
        app_cwsl_delete(req);
        cJSON_Delete(req);

        assert(s_uart_send_count == 1);
        assert(s_mock_uart_action == APP_CWSL_ACTION_DELETE);
        assert(s_mock_uart_data_count == 2);
        assert(s_mock_uart_data[0] == 7U);
        assert(s_mock_uart_data[1] == 0U);

        /* Chip confirms DELETE_OK (33) */
        s_registered_status_cb(33U, 7U, 0U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"fanoem\":221") != NULL);
        assert(strstr(s_last_published, "\"code\":33") != NULL);
        assert(s_scene_clear_all_count == 0);

        /* Delete All (255, 255) success clears scene_mask */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":221,\"cmd_id\":255,\"slot\":255,\"u\":2002,\"act_id\":2}");
        app_cwsl_delete(req);
        cJSON_Delete(req);

        assert(s_uart_send_count == 1);
        assert(s_mock_uart_data[0] == 255U);
        assert(s_mock_uart_data[1] == 255U);
        assert(s_scene_clear_all_count == 0);

        s_registered_status_cb(33U, 255U, 255U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":33") != NULL);
        assert(s_scene_clear_all_count == 1);

        /* Delete All failure/timeout does NOT clear scene_mask */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":221,\"cmd_id\":255,\"slot\":255,\"u\":2003,\"act_id\":3}");
        app_cwsl_delete(req);
        cJSON_Delete(req);
        assert(s_uart_send_count == 1);
        s_registered_status_cb(51U, 255U, 255U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"code\":51") != NULL);
        assert(s_scene_clear_all_count == 0);

        printf("  PASS: DELETE operations and Delete All verified (including scene clear)\n");
    }

    /* TEST 4: QUERY Requests (Capacity & Slots) */
    {
        printf("--- Test 4: QUERY Requests ---\n");
        reset_cwsl_mocks();
        app_cwsl_init();

        /* Query single command (cmd_id 7) */
        cJSON *req = cJSON_Parse("{\"fanoem\":223,\"cmd_id\":7,\"u\":3001,\"act_id\":1}");
        app_cwsl_query(req);
        cJSON_Delete(req);

        assert(s_uart_send_count == 1);
        assert(s_mock_uart_action == APP_CWSL_ACTION_QUERY);
        assert(s_mock_uart_data_count == 1);
        assert(s_mock_uart_data[0] == 7U);

        /* Response with slot bitmap (low=5, high=0 -> slots 0 and 2 active) */
        uint8_t data_info[2] = {5U, 0U};
        s_registered_info_cb(7U, data_info, 2U);

        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"fanoem\":223") != NULL);
        assert(strstr(s_last_published, "\"cmd_id\":7") != NULL);
        assert(strstr(s_last_published, "\"bitmap\":5") != NULL);

        /* Query total capacity (cmd_id 255) */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":223,\"cmd_id\":255,\"u\":3002,\"act_id\":2}");
        app_cwsl_query(req);
        cJSON_Delete(req);

        uint8_t data_total[3] = {2U, 6U, 12U};
        s_registered_info_cb(255U, data_total, 3U);

        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"wake\":2") != NULL);
        assert(strstr(s_last_published, "\"ncmd\":6") != NULL);
        assert(strstr(s_last_published, "\"nfree\":12") != NULL);

        printf("  PASS: QUERY single cmd and total capacity verified\n");
    }

    /* TEST 5: Voice Notification MQTT 224 with metadata */
    {
        printf("--- Test 5: Voice Notification (MQTT 224) ---\n");
        reset_cwsl_mocks();

        /* Default voice command (cwsl = 255) */
        app_cwsl_notify_voice_cmd(7U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"fanoem\":224,\"cmd_id\":7,\"cwsl\":255}") == 0);

        /* Learned voice command (cwsl = 3) */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(13U, 3U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"fanoem\":224,\"cmd_id\":13,\"cwsl\":3}") == 0);

        /* Scene command via voice (cmd_id = 60, cwsl = 0) */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(60U, 0U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"fanoem\":224,\"cmd_id\":60,\"cwsl\":0}") == 0);

        /* Highest scene command (cmd_id 70) is published */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(70U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"fanoem\":224,\"cmd_id\":70,\"cwsl\":255}") == 0);

        /* Invalid command ID 0 must not publish */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(0U, 255U);
        assert(s_publish_count == 0);

        /* Home command 71 is published */
        app_cwsl_notify_voice_cmd(71U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"fanoem\":224,\"cmd_id\":71,\"cwsl\":255}") == 0);

        printf("  PASS: Voice notifications publish proper JSON with cmd_id and cwsl\n");
    }

    /* TEST 6: VOICEBOX (voiceboxv1) CWSL Scope & LED Pulse on Scene */
    {
        printf("--- Test 6: VOICEBOX CWSL & LED Pulse Verification ---\n");
        reset_cwsl_mocks();
        device_type = 10; /* VOICEBOX */
        s_mock_led_pulse_count = 0;
        s_mock_led_pulse_duration = 0;

        /* Learn wake word (cmd_id 1) must be allowed */
        cJSON *req = cJSON_Parse("{\"voiceboxv1\":220,\"cmd_id\":1,\"slot\":0,\"u\":5001,\"act_id\":1}");
        assert(req != NULL);
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_uart_send_count == 1);
        assert(s_mock_uart_action == APP_CWSL_ACTION_LEARN);
        assert(s_mock_uart_data[0] == 1);

        /* Complete learn */
        s_registered_status_cb(32U, 1U, 0U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"voiceboxv1\":220") != NULL);
        assert(strstr(s_last_published, "\"code\":32") != NULL);
        assert(strstr(s_last_published, "\"cmd_id\":1") != NULL);
        assert(strstr(s_last_published, "\"slot\":0") != NULL);

        /* Learn fan command (cmd_id 7) must be rejected with 65 on voicebox */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"voiceboxv1\":220,\"cmd_id\":7,\"slot\":0,\"u\":5002,\"act_id\":2}");
        assert(req != NULL);
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"voiceboxv1\":220") != NULL);
        assert(strstr(s_last_published, "\"code\":65") != NULL);
        assert(strstr(s_last_published, "\"result\":0") != NULL);
        assert(s_uart_send_count == 0); /* did not send to UART */

        /* Delete fan command (cmd_id 7) must be rejected with 65 on voicebox */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"voiceboxv1\":221,\"cmd_id\":7,\"slot\":0,\"u\":5003,\"act_id\":3}");
        assert(req != NULL);
        app_cwsl_delete(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"voiceboxv1\":221") != NULL);
        assert(strstr(s_last_published, "\"code\":65") != NULL);
        assert(strstr(s_last_published, "\"result\":0") != NULL);

        /* Delete wake word (cmd_id 1) must be allowed */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"voiceboxv1\":221,\"cmd_id\":1,\"slot\":0,\"u\":5004,\"act_id\":4}");
        assert(req != NULL);
        app_cwsl_delete(req);
        cJSON_Delete(req);
        assert(s_uart_send_count == 1);
        assert(s_mock_uart_action == APP_CWSL_ACTION_DELETE);
        assert(s_mock_uart_data[0] == 1);
        s_registered_status_cb(33U, 1U, 0U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"voiceboxv1\":221") != NULL);
        assert(strstr(s_last_published, "\"code\":33") != NULL);

        /* Delete all (cmd_id 255, slot 255) must be allowed */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"voiceboxv1\":221,\"cmd_id\":255,\"slot\":255,\"u\":5005,\"act_id\":5}");
        assert(req != NULL);
        app_cwsl_delete(req);
        cJSON_Delete(req);
        assert(s_uart_send_count == 1);
        assert(s_mock_uart_action == APP_CWSL_ACTION_DELETE);
        assert(s_mock_uart_data[0] == 255);
        s_registered_status_cb(33U, 255U, 255U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"voiceboxv1\":221") != NULL);
        assert(strstr(s_last_published, "\"code\":33") != NULL);

        /* Query fan command (cmd_id 7) must be rejected with 65 on voicebox */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"voiceboxv1\":223,\"cmd_id\":7,\"u\":5006,\"act_id\":6}");
        assert(req != NULL);
        app_cwsl_query(req);
        cJSON_Delete(req);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"voiceboxv1\":223") != NULL);
        assert(strstr(s_last_published, "\"code\":65") != NULL);
        assert(strstr(s_last_published, "\"result\":0") != NULL);

        /* Query wake word (cmd_id 1) must be allowed */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"voiceboxv1\":223,\"cmd_id\":1,\"u\":5007,\"act_id\":7}");
        assert(req != NULL);
        app_cwsl_query(req);
        cJSON_Delete(req);
        assert(s_uart_send_count == 1);
        assert(s_mock_uart_action == APP_CWSL_ACTION_QUERY);
        uint8_t data_info[2] = {1U, 0U};
        s_registered_info_cb(1U, data_info, 2U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"voiceboxv1\":223") != NULL);
        assert(strstr(s_last_published, "\"bitmap\":1") != NULL);

        /* Query capacity (cmd_id 255) must be allowed */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"voiceboxv1\":223,\"cmd_id\":255,\"u\":5008,\"act_id\":8}");
        assert(req != NULL);
        app_cwsl_query(req);
        cJSON_Delete(req);
        assert(s_uart_send_count == 1);
        uint8_t data_total[3] = {1U, 0U, 15U};
        s_registered_info_cb(255U, data_total, 3U);
        assert(s_publish_count == 1);
        assert(strstr(s_last_published, "\"wake\":1") != NULL);

        /* Voice notify on scene command (cmd_id 56) triggers MQTT 224, LED 200ms pulse, AND BLE beacon */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(56U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":56,\"cwsl\":255}") == 0);
        assert(s_mock_led_pulse_count == 1);
        assert(s_mock_led_pulse_duration == 200);
        assert(s_mock_ble_beacon_count == 1);
        assert(s_mock_ble_beacon_cmd_id == 56U);

        /* Voice notify on scene command (cmd_id 66) with learned slot 2 */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(66U, 2U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":66,\"cwsl\":2}") == 0);
        assert(s_mock_ble_beacon_cmd_id == 66U);

        /* Voice notify on default voice command (cmd_id 7) triggers MQTT 224, LED 200ms pulse, AND BLE beacon */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(7U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":7,\"cwsl\":255}") == 0);
        assert(s_mock_led_pulse_count == 1);
        assert(s_mock_led_pulse_duration == 200);
        assert(s_mock_ble_beacon_count == 1);
        assert(s_mock_ble_beacon_cmd_id == 7U);

        /* Voice notify on wake word (cmd_id 1) triggers MQTT 224 but NOT LED pulse or BLE beacon */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(1U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":1,\"cwsl\":255}") == 0);
        assert(s_mock_led_pulse_count == 0);
        assert(s_mock_ble_beacon_count == 0);

        device_type = 0; /* restore */
        printf("  PASS: VOICEBOX wake-only CWSL validation, MQTT 224, BLE beacon, and 200ms LED pulse verified\n");
    }
    /* TEST 7: Scene add FSM with respond_id */
    {
        printf("--- Test 7: Scene add FSM with respond_id ---\n");
        device_type = 10;
        reset_cwsl_mocks();
        assert(app_cwsl_start_scene_learn(56U, 101U, 1U, 47U) == 0U);
        assert(s_mock_uart_action == APP_CWSL_ACTION_QUERY && s_mock_uart_data_count == 1 && s_mock_uart_data[0] == 56U);
        uint8_t empty_bitmap[2] = {0U, 0U};
        s_registered_info_cb(56U, empty_bitmap, 2U);
        assert(s_mock_uart_action == APP_CWSL_ACTION_LEARN && s_mock_uart_data_count == 3);
        assert(s_mock_uart_data[0] == 56U && s_mock_uart_data[1] == 255U && s_mock_uart_data[2] == 47U);
        s_registered_status_cb(32U, 56U, 3U);
        assert(s_scene_learn_result_count == 1 && s_scene_learn_result_code == 32U && s_scene_learn_result_cmd_id == 56U);

        device_type = 0;
        printf("  PASS: Scene add FSM with respond_id verified\n");
    }

    /* TEST 8: Scene add cancel (222) */
    {
        printf("--- Test 8: Scene add cancel (222) ---\n");
        device_type = 10;
        uint8_t empty_bitmap[2] = {0U, 0U};
        const char *stop_json = "{\"voiceboxv1\":222,\"u\":9,\"act_id\":2}";
        cJSON *req;
        int sent_before;
        int pub_before;

        /* (a) cancel in QUERY_OLD */
        reset_cwsl_mocks();
        assert(app_cwsl_start_scene_learn(56U, 101U, 1U, 47U) == 0U);
        sent_before = s_uart_send_count;
        req = cJSON_Parse(stop_json);
        app_cwsl_stop(req);
        cJSON_Delete(req);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"cmd_id\":56,\"slot\":255,\"code\":34,\"u\":9,\"act_id\":2}") == 0);
        assert(strstr(s_last_published, "\"result\"") == NULL);
        assert(s_scene_learn_result_code == 34U);
        assert(s_uart_send_count == sent_before);
        sent_before = s_uart_send_count;
        pub_before = s_publish_count;
        s_registered_info_cb(56U, empty_bitmap, 2U);
        assert(s_uart_send_count == sent_before);
        assert(s_publish_count == pub_before);

        /* (b) cancel in LEARNING, chip returns 34 */
        reset_cwsl_mocks();
        assert(app_cwsl_start_scene_learn(56U, 101U, 1U, 47U) == 0U);
        s_registered_info_cb(56U, empty_bitmap, 2U);
        req = cJSON_Parse(stop_json);
        app_cwsl_stop(req);
        cJSON_Delete(req);
        assert(s_mock_uart_action == APP_CWSL_ACTION_STOP);
        s_registered_status_cb(34U, 56U, 3U);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"cmd_id\":56,\"slot\":3,\"code\":34,\"u\":9,\"act_id\":2}") == 0);
        assert(strstr(s_last_published, "\"result\"") == NULL);

        /* (c) cancel in LEARNING, chip returns 48 */
        reset_cwsl_mocks();
        assert(app_cwsl_start_scene_learn(56U, 101U, 1U, 47U) == 0U);
        s_registered_info_cb(56U, empty_bitmap, 2U);
        req = cJSON_Parse(stop_json);
        app_cwsl_stop(req);
        cJSON_Delete(req);
        assert(s_mock_uart_action == APP_CWSL_ACTION_STOP);
        s_registered_status_cb(48U, 56U, 255U);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"cmd_id\":56,\"slot\":255,\"code\":48,\"u\":9,\"act_id\":2}") == 0);
        assert(strstr(s_last_published, "\"result\"") == NULL);

        /* (d) cancel in LEARNING, no chip frame -> timeout */
        reset_cwsl_mocks();
        assert(app_cwsl_start_scene_learn(56U, 101U, 1U, 47U) == 0U);
        s_registered_info_cb(56U, empty_bitmap, 2U);
        req = cJSON_Parse(stop_json);
        app_cwsl_stop(req);
        cJSON_Delete(req);
        s_mock_tick = 120000;
        app_cwsl_poll();
        assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"cmd_id\":56,\"slot\":255,\"code\":52,\"u\":9,\"act_id\":2}") == 0);
        assert(strstr(s_last_published, "\"result\"") == NULL);
        assert(s_scene_learn_result_code == 52U);

        /* (e) cancel in DELETING_OLD -> reject 67 */
        reset_cwsl_mocks();
        assert(app_cwsl_start_scene_learn(56U, 101U, 1U, 47U) == 0U);
        uint8_t used_bitmap[2] = {0x01U, 0x00U};
        s_registered_info_cb(56U, used_bitmap, 2U);
        s_registered_status_cb(32U, 56U, 3U);
        assert(s_mock_uart_action == APP_CWSL_ACTION_DELETE);
        req = cJSON_Parse(stop_json);
        app_cwsl_stop(req);
        cJSON_Delete(req);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"u\":9,\"act_id\":2,\"result\":0,\"code\":67}") == 0);

        device_type = 0;
        printf("  PASS: Scene add cancel (222) verified\n");
    }

    /* TEST 9: Scene respond_id edit (cmd:15) */
    {
        printf("--- Test 9: Scene respond_id edit ---\n");
        device_type = 10;
        cJSON *req;
        int sent_before;

        /* (a) success */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
        assert(s_resp_set_send_count == 1 && s_resp_set_last_cmd_id == 56U && s_resp_set_last_value == 47U);
        assert(app_cwsl_is_pending() == true);
        s_registered_resp_set_cb(1U);
        assert(s_scene_resp_set_result_count == 1 && s_scene_resp_set_result_code == 0U);
        assert(app_cwsl_is_pending() == false);

        /* (b) chip rejects */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
        s_registered_resp_set_cb(0U);
        assert(s_scene_resp_set_result_count == 1 && s_scene_resp_set_result_code == 64U);
        assert(app_cwsl_is_pending() == false);

        /* (c) timeout, then late reply is dropped */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
        s_mock_tick = 1999;
        app_cwsl_poll();
        assert(s_scene_resp_set_result_count == 0);
        s_mock_tick = 2000;
        app_cwsl_poll();
        assert(s_scene_resp_set_result_count == 1 && s_scene_resp_set_result_code == 52U);
        assert(app_cwsl_is_pending() == false);
        s_registered_resp_set_cb(1U);
        assert(s_scene_resp_set_result_count == 1);

        /* (d) TX fail */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        s_mock_resp_set_ret = -1;
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 70U);
        assert(app_cwsl_is_pending() == false);

        /* (e) out of range */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(55U, 47U) == 65U);
        assert(app_cwsl_start_scene_resp_set(71U, 47U) == 65U);
        assert(s_resp_set_send_count == 0);

        /* (f) busy while a resp-set is pending */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
        assert(app_cwsl_start_scene_learn(57U, 1U, 0U, 47U) == 67U);
        assert(app_cwsl_start_scene_delete(57U, 1U, 0U) == 67U);
        assert(app_cwsl_start_scene_resp_set(57U, 47U) == 67U);
        sent_before = s_uart_send_count;
        req = cJSON_Parse("{\"voiceboxv1\":220,\"cmd_id\":1,\"slot\":255,\"u\":3,\"act_id\":0}");
        app_cwsl_learn(req);
        cJSON_Delete(req);
        assert(strstr(s_last_published, "\"code\":67") != NULL);
        req = cJSON_Parse("{\"voiceboxv1\":221,\"cmd_id\":1,\"slot\":255,\"u\":3,\"act_id\":0}");
        app_cwsl_delete(req);
        cJSON_Delete(req);
        assert(strstr(s_last_published, "\"code\":67") != NULL);
        req = cJSON_Parse("{\"voiceboxv1\":223,\"cmd_id\":1,\"u\":3,\"act_id\":0}");
        app_cwsl_query(req);
        cJSON_Delete(req);
        assert(strstr(s_last_published, "\"code\":67") != NULL);
        req = cJSON_Parse("{\"voiceboxv1\":222,\"u\":3,\"act_id\":0}");
        app_cwsl_stop(req);
        cJSON_Delete(req);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":222,\"u\":3,\"act_id\":0,\"result\":0,\"code\":67}") == 0);
        assert(s_uart_send_count == sent_before);

        /* (g) reverse: learn session blocks resp-set */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_learn(56U, 1U, 0U, 47U) == 0U);
        assert(app_cwsl_start_scene_resp_set(57U, 47U) == 67U);

        /* (h1) late reply after timeout must not complete the next edit */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
        s_mock_tick = 2000;
        app_cwsl_poll();
        assert(s_scene_resp_set_result_count == 1 && s_scene_resp_set_result_code == 52U);
        sent_before = s_resp_set_send_count;
        assert(app_cwsl_start_scene_resp_set(57U, 50U) == 67U);
        assert(s_resp_set_send_count == sent_before);
        s_registered_resp_set_cb(1U);
        assert(s_scene_resp_set_result_count == 1);
        assert(app_cwsl_start_scene_resp_set(57U, 50U) == 0U);
        s_registered_resp_set_cb(1U);
        assert(s_scene_resp_set_result_count == 2 && s_scene_resp_set_result_code == 0U);

        /* (h2) drain window expires after 2000 ms without a late reply */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
        s_mock_tick = 2000;
        app_cwsl_poll();
        s_mock_tick = 3999;
        app_cwsl_poll();
        assert(app_cwsl_start_scene_resp_set(57U, 50U) == 67U);
        s_mock_tick = 4000;
        app_cwsl_poll();
        assert(app_cwsl_start_scene_resp_set(57U, 50U) == 0U);

        /* (h3) draining does not block learn */
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
        s_mock_tick = 2000;
        app_cwsl_poll();
        assert(app_cwsl_start_scene_learn(56U, 1U, 0U, 47U) == 0U);

        device_type = 0;
        printf("  PASS: Scene respond_id edit verified\n");
    }

    /* TEST 10: CWSL refuses with code 67 while a voice request is pending (230/231) */
    {
        printf("--- Test 10: Voice pending blocks CWSL ---\n");
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        s_mock_voice_pending = true;
        int sent = s_uart_send_count;
        cJSON *req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":7,\"slot\":255,\"u\":1001,\"act_id\":1}");
        app_cwsl_learn(req); cJSON_Delete(req);
        assert(strstr(s_last_published, "\"code\":67") != NULL && s_uart_send_count == sent);
        assert(app_cwsl_start_scene_learn(56U, 1U, 0U, 47U) == 67U);
        assert(app_cwsl_start_scene_delete(56U, 1U, 0U) == 67U);
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 67U);
        req = cJSON_Parse("{\"fanoem\":222,\"u\":1001,\"act_id\":1}");
        app_cwsl_stop(req); cJSON_Delete(req);
        assert(strstr(s_last_published, "\"code\":67") != NULL);
        req = cJSON_Parse("{\"fanoem\":223,\"cmd_id\":255,\"u\":1001,\"act_id\":1}");
        app_cwsl_query(req); cJSON_Delete(req);
        assert(strstr(s_last_published, "\"code\":67") != NULL && s_uart_send_count == sent);
        s_mock_voice_pending = false;
        printf("  PASS: Voice pending blocks CWSL\n");
    }

    /* TEST 11: app_cwsl_is_learning is true only for 220 learn / 227 add sessions */
    {
        printf("--- Test 11: app_cwsl_is_learning ---\n");
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(!app_cwsl_is_learning());
        cJSON *req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":7,\"slot\":255,\"u\":1001,\"act_id\":1}");
        app_cwsl_learn(req); cJSON_Delete(req);
        assert(app_cwsl_is_learning());
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_learn(56U, 1U, 0U, 47U) == 0U);
        assert(app_cwsl_is_learning());
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_delete(56U, 1U, 0U) == 0U);
        assert(!app_cwsl_is_learning());
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        req = cJSON_Parse("{\"fanoem\":223,\"cmd_id\":255,\"u\":1001,\"act_id\":1}");
        app_cwsl_query(req); cJSON_Delete(req);
        assert(!app_cwsl_is_learning());
        reset_cwsl_mocks();
        reset_cwsl_sessions();
        assert(app_cwsl_start_scene_resp_set(56U, 47U) == 0U);
        assert(!app_cwsl_is_learning());
        printf("  PASS: app_cwsl_is_learning verified\n");
    }

    /* TEST 12: Home command notify 71..103 */
    {
        printf("--- Test 12: Home command notify 71..103 ---\n");

        /* 12.1: VOICEBOX home command ignores scene mask: LED + BLE + MQTT 224 */
        reset_cwsl_mocks();
        device_type = 10; /* VOICEBOX */
        s_mock_scene_active = false;
        app_cwsl_notify_voice_cmd(102U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":102,\"cwsl\":255}") == 0);
        assert(s_mock_led_pulse_count == 1);
        assert(s_mock_led_pulse_duration == 200);
        assert(s_mock_ble_beacon_count == 1);
        assert(s_mock_ble_beacon_cmd_id == 102U);

        /* 12.2: range bounds and learned template cwsl */
        reset_cwsl_mocks();
        device_type = 10;
        app_cwsl_notify_voice_cmd(71U, 255U);
        app_cwsl_notify_voice_cmd(103U, 3U);
        assert(s_publish_count == 2);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":103,\"cwsl\":3}") == 0);
        assert(s_mock_ble_beacon_cmd_id == 103U);

        /* 12.3: IDs above 103 do nothing */
        reset_cwsl_mocks();
        device_type = 10;
        app_cwsl_notify_voice_cmd(104U, 255U);
        app_cwsl_notify_voice_cmd(255U, 255U);
        assert(s_publish_count == 0);
        assert(s_mock_led_pulse_count == 0);
        assert(s_mock_ble_beacon_count == 0);

        /* 12.4: non-VOICEBOX publishes 224 without LED or BLE */
        reset_cwsl_mocks();
        device_type = 0; /* fanoem */
        app_cwsl_notify_voice_cmd(102U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"fanoem\":224,\"cmd_id\":102,\"cwsl\":255}") == 0);
        assert(s_mock_led_pulse_count == 0);
        assert(s_mock_ble_beacon_count == 0);

        /* 12.5: regression - inactive scene still skipped */
        reset_cwsl_mocks();
        device_type = 10;
        s_mock_scene_active = false;
        app_cwsl_notify_voice_cmd(56U, 255U);
        assert(s_publish_count == 0);
        assert(s_mock_ble_beacon_count == 0);

        device_type = 0;
        printf("  PASS: Home command notify 71..103 verified\n");
    }

    printf("\nALL 12 CWSL TEST SCENARIOS PASSED 100%%!\n");
    return 0;
}
