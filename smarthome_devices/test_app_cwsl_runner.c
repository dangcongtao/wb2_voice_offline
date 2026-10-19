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

bool app_scene_is_pending(void)
{
    return false;
}

bool app_scene_is_cmd_id_active(uint8_t cmd_id)
{
    (void)cmd_id;
    return true;
}

void app_scene_on_learn_result(uint8_t code, uint8_t cmd_id)
{
    (void)code;
    (void)cmd_id;
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
    s_mock_uart_ret = 0;
    s_mock_tick = 0;
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

        /* Scene commands (56..65) must be rejected with BAD_REQUEST (64) */
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

        /* Out-of-bounds commands (> 65) must be rejected with BAD_COMMAND_ID (65) */
        reset_cwsl_mocks();
        req = cJSON_Parse("{\"fanoem\":220,\"cmd_id\":66,\"slot\":0,\"u\":1006,\"act_id\":6}");
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

        /* Invalid command IDs (0, 68) must not publish */
        reset_cwsl_mocks();
        app_cwsl_notify_voice_cmd(0U, 255U);
        assert(s_publish_count == 0);

        app_cwsl_notify_voice_cmd(68U, 255U);
        assert(s_publish_count == 0);

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

        /* Voice notify on scene command (cmd_id 56) triggers MQTT 224 AND LED 200ms pulse */
        reset_cwsl_mocks();
        s_mock_led_pulse_count = 0;
        app_cwsl_notify_voice_cmd(56U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":56,\"cwsl\":255}") == 0);
        assert(s_mock_led_pulse_count == 1);
        assert(s_mock_led_pulse_duration == 200);

        /* Voice notify on wake word (cmd_id 1) triggers MQTT 224 but NOT LED pulse */
        reset_cwsl_mocks();
        s_mock_led_pulse_count = 0;
        app_cwsl_notify_voice_cmd(1U, 255U);
        assert(s_publish_count == 1);
        assert(strcmp(s_last_published, "{\"voiceboxv1\":224,\"cmd_id\":1,\"cwsl\":255}") == 0);
        assert(s_mock_led_pulse_count == 0);

        device_type = 0; /* restore */
        printf("  PASS: VOICEBOX wake-only CWSL validation, MQTT 224, and 200ms LED pulse verified\n");
    }

    printf("\nALL 6 CWSL TEST SCENARIOS PASSED 100%%!\n");
    return 0;
}
