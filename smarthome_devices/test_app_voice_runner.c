#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define os_trace printf

/* Guard unwanted SDK headers */
#define _USER_CONFIG_H_
/* app_config.h is blocked: mirror its VOICEBOX hardware selection.
 * Default V2; build with -DVOICEBOX_HW=1 for V1. */
#define VOICEBOX_HW_V1 1
#define VOICEBOX_HW_V2 2
#ifndef VOICEBOX_HW
#define VOICEBOX_HW VOICEBOX_HW_V2
#endif
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
uint32_t device_type = 10;

static char s_last_published[256];
static int s_publish_count = 0;

void app_send_data(char *data, uint32_t len)
{
    (void)len;
    strncpy(s_last_published, data, sizeof(s_last_published) - 1);
    s_last_published[sizeof(s_last_published) - 1] = '\0';
    s_publish_count++;
}

/* Voice UART mocks */
static int s_voice_set_count = 0;
static uint8_t s_voice_set_arg = 0;
static int32_t s_voice_set_ret = 0;
static int s_voice_query_count = 0;
static int32_t s_voice_query_ret = 0;
static aiio_ci13162_voice_set_cb_t s_voice_set_cb = NULL;
static aiio_ci13162_voice_get_cb_t s_voice_get_cb = NULL;

int32_t aiio_ci13162_uart_send_voice_set(uint8_t voice_id)
{
    s_voice_set_count++;
    s_voice_set_arg = voice_id;
    return s_voice_set_ret;
}

int32_t aiio_ci13162_uart_query_voice(void)
{
    s_voice_query_count++;
    return s_voice_query_ret;
}

void aiio_ci13162_uart_set_voice_callbacks(aiio_ci13162_voice_set_cb_t set_cb,
                                           aiio_ci13162_voice_get_cb_t get_cb)
{
    s_voice_set_cb = set_cb;
    s_voice_get_cb = get_cb;
}

/* Volume UART mocks */
static int s_volume_send_count = 0;
static uint8_t s_volume_send_arg = 0;
static int32_t s_volume_send_ret = 0;
static int s_volume_query_count = 0;
static int s_mute_send_count = 0;
static int32_t s_mute_send_ret = 0;
static int32_t s_state_valid = -1;
static uint8_t s_state_level = 0;
static uint8_t s_state_muted = 0;
static uint8_t s_voice_id_ret = 0;
static aiio_ci13162_volume_info_cb_t s_vol_cb = NULL;

int32_t aiio_ci13162_uart_send_volume(uint8_t value)
{
    s_volume_send_count++;
    s_volume_send_arg = value;
    return s_volume_send_ret;
}

int32_t aiio_ci13162_uart_query_volume(void)
{
    s_volume_query_count++;
    return 0;
}

void aiio_ci13162_uart_set_volume_callback(aiio_ci13162_volume_info_cb_t callback)
{
    s_vol_cb = callback;
}

int32_t aiio_ci13162_uart_send_mute(void)
{
    s_mute_send_count++;
    return s_mute_send_ret;
}

int32_t aiio_ci13162_uart_get_volume_state(uint8_t *level, uint8_t *muted)
{
    if (s_state_valid != 0) {
        return -1;
    }
    *level = s_state_level;
    *muted = s_state_muted;
    return 0;
}

uint8_t aiio_ci13162_uart_get_voice_id(void)
{
    return s_voice_id_ret;
}

int32_t aiio_ci13162_uart_get_volume(uint8_t *level)
{
    (void)level;
    return -1;
}

/* Gating mocks */
static bool s_ota_held = false;
static bool s_ci_ota_busy = false;
static bool s_ota_busy = false;
static bool s_cwsl_pending = false;

bool aiio_ci13162_uart_ota_held(void) { return s_ota_held; }
bool m_ci13162_ota_is_busy(void) { return s_ci_ota_busy; }
bool m_ota_is_busy(void) { return s_ota_busy; }
bool app_cwsl_is_pending(void) { return s_cwsl_pending; }

static bool s_cwsl_learning = false;
bool app_cwsl_is_learning(void) { return s_cwsl_learning; }

static int s_buzz_type_count = 0;
static uint8_t s_buzz_type_arg = 0xFFU;
int32_t aiio_ci13162_uart_send_buzz_type(uint8_t is_long)
{
    s_buzz_type_count++;
    s_buzz_type_arg = is_long;
    return 0;
}

/* Queue mocks: ring buffer of uint8_t */
static uint8_t s_q_buf[16];
static uint32_t s_q_head = 0;
static uint32_t s_q_count = 0;
static uint32_t s_q_len = 0;

QueueHandle_t xQueueCreateStatic(uint32_t uxQueueLength, uint32_t uxItemSize,
                                 uint8_t *pucQueueStorage, StaticQueue_t *pxQueueBuffer)
{
    (void)uxItemSize; (void)pucQueueStorage; (void)pxQueueBuffer;
    s_q_len = uxQueueLength;
    s_q_head = 0;
    s_q_count = 0;
    return (QueueHandle_t)s_q_buf;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait)
{
    (void)xQueue; (void)xTicksToWait;
    if (s_q_count >= s_q_len) {
        return 0;
    }
    s_q_buf[(s_q_head + s_q_count) % s_q_len] = *(const uint8_t *)pvItemToQueue;
    s_q_count++;
    return 1;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait)
{
    (void)xQueue; (void)xTicksToWait;
    if (s_q_count == 0U) {
        return 0;
    }
    *(uint8_t *)pvBuffer = s_q_buf[s_q_head];
    s_q_head = (s_q_head + 1U) % s_q_len;
    s_q_count--;
    return 1;
}

BaseType_t xQueueReset(QueueHandle_t xQueue)
{
    (void)xQueue;
    s_q_head = 0;
    s_q_count = 0;
    return 1;
}

static TickType_t s_mock_tick = 0;
TickType_t xTaskGetTickCount(void) { return s_mock_tick; }

/* Include the real implementations */
#include "components/application/app_voice.c"
#include "components/application/app_volume.c"
#include "components/third_party/huytv_button/app_btn.h"
#include "components/application/app_button.c"

static void reset_all(void)
{
    s_last_published[0] = '\0';
    s_publish_count = 0;
    s_voice_set_count = 0;
    s_voice_set_arg = 0;
    s_voice_set_ret = 0;
    s_voice_query_count = 0;
    s_voice_query_ret = 0;
    s_volume_send_count = 0;
    s_volume_send_arg = 0;
    s_volume_send_ret = 0;
    s_volume_query_count = 0;
    s_mute_send_count = 0;
    s_mute_send_ret = 0;
    s_state_valid = -1;
    s_state_level = 0;
    s_state_muted = 0;
    s_voice_id_ret = 0;
    s_ota_held = false;
    s_ci_ota_busy = false;
    s_ota_busy = false;
    s_cwsl_pending = false;
    s_cwsl_learning = false;
    s_buzz_type_count = 0;
    s_buzz_type_arg = 0xFFU;
    s_q_head = 0;
    s_q_count = 0;
    s_mock_tick = 0;
    app_voice_init();
    app_volume_init();
}

static void call(void (*fn)(cJSON *), const char *json)
{
    cJSON *root = cJSON_Parse(json);

    assert(root != NULL);
    fn(root);
    cJSON_Delete(root);
}

static void expect_published(const char *expected)
{
    if (strcmp(s_last_published, expected) != 0) {
        printf("expected: %s\n     got: %s\n", expected, s_last_published);
        assert(0);
    }
}


static void test_set_ok(void)
{
    reset_all();
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    assert(s_voice_set_count == 1);
    assert(s_voice_set_arg == 2);
    assert(s_publish_count == 0);
    assert(app_voice_is_pending());
    s_voice_set_cb(1);
    assert(s_publish_count == 1);
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":1}");
    assert(!app_voice_is_pending());
    printf("PASS set_ok\n");
}

static void test_set_chip_rejects(void)
{
    reset_all();
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    s_voice_set_cb(0);
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":64}");
    assert(!app_voice_is_pending());
    printf("PASS set_chip_rejects\n");
}

static void check_invalid(const char *req, const char *expected)
{
    reset_all();
    call(app_voice_set, req);
    assert(s_voice_set_count == 0);
    assert(s_publish_count == 1);
    expect_published(expected);
    assert(!app_voice_is_pending());
}

static void test_set_rejects_invalid_requests(void)
{
    check_invalid("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":0,\"act_id\":0}",
                  "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":0,\"act_id\":0,\"result\":0,\"code\":64}");
    check_invalid("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":5,\"act_id\":0}",
                  "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":5,\"act_id\":0,\"result\":0,\"code\":64}");
    check_invalid("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":\"2\",\"act_id\":0}",
                  "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":0,\"act_id\":0,\"result\":0,\"code\":64}");
    check_invalid("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2.5,\"act_id\":0}",
                  "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":0,\"act_id\":0,\"result\":0,\"code\":64}");
    check_invalid("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":300,\"act_id\":0}",
                  "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":0,\"act_id\":0,\"result\":0,\"code\":64}");
    check_invalid("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":true,\"act_id\":0}",
                  "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":0,\"act_id\":0,\"result\":0,\"code\":64}");
    check_invalid("{\"voiceboxv1\":230,\"u\":55519,\"act_id\":0}",
                  "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":0,\"act_id\":0,\"result\":0,\"code\":64}");
    check_invalid("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2}",
                  "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":64}");
    check_invalid("{\"voiceboxv1\":230,\"voice_id\":2,\"act_id\":0}",
                  "{\"voiceboxv1\":230,\"u\":0,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":64}");
    printf("PASS set_rejects_invalid_requests\n");
}

static void test_get_ok(void)
{
    reset_all();
    call(app_voice_get, "{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0}");
    assert(s_voice_query_count == 1);
    assert(s_publish_count == 0);
    assert(app_voice_is_pending());
    s_voice_get_cb(3);
    expect_published("{\"voiceboxv1\":231,\"u\":55519,\"voice_id\":3,\"act_id\":0,\"result\":1}");
    assert(!app_voice_is_pending());
    printf("PASS get_ok\n");
}

static void test_get_rejects_missing_act_id(void)
{
    reset_all();
    call(app_voice_get, "{\"voiceboxv1\":231,\"u\":55519}");
    assert(s_voice_query_count == 0);
    expect_published("{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0,\"result\":0,\"code\":64}");
    printf("PASS get_rejects_missing_act_id\n");
}

static void start_set_and_timeout(void)
{
    reset_all();
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    s_mock_tick = 1999;
    app_voice_poll();
    assert(s_publish_count == 0);
    s_mock_tick = 2000;
    app_voice_poll();
    assert(s_publish_count == 1);
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":52}");
    assert(app_voice_is_pending());
}

static void test_timeout_then_drain_expires(void)
{
    start_set_and_timeout();
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":3,\"act_id\":0}");
    assert(s_publish_count == 2);
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":3,\"act_id\":0,\"result\":0,\"code\":67}");
    assert(s_voice_set_count == 1);
    s_mock_tick = 4000;
    app_voice_poll();
    assert(!app_voice_is_pending());
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":3,\"act_id\":0}");
    assert(s_voice_set_count == 2);
    assert(s_voice_set_arg == 3);
    assert(app_voice_is_pending());
    printf("PASS timeout_then_drain_expires\n");
}

static void test_late_set_reply_only_ends_drain(void)
{
    start_set_and_timeout();
    s_voice_set_cb(1);
    assert(s_publish_count == 1);
    assert(!app_voice_is_pending());
    printf("PASS late_set_reply_only_ends_drain\n");
}

static void test_get_frame_does_not_end_set_drain(void)
{
    start_set_and_timeout();
    s_voice_get_cb(3);
    assert(s_publish_count == 1);
    assert(app_voice_is_pending());
    printf("PASS get_frame_does_not_end_set_drain\n");
}

static void test_get_timeout_and_drain(void)
{
    reset_all();
    call(app_voice_get, "{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0}");
    s_mock_tick = 2000;
    app_voice_poll();
    assert(s_publish_count == 1);
    expect_published("{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0,\"result\":0,\"code\":52}");
    assert(app_voice_is_pending());
    s_voice_get_cb(3);
    assert(s_publish_count == 1);
    assert(!app_voice_is_pending());

    reset_all();
    s_ci_ota_busy = true;
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":69}");

    reset_all();
    s_ota_busy = true;
    call(app_voice_get, "{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0,\"result\":0,\"code\":69}");
    assert(s_voice_set_count == 0);
    assert(s_voice_query_count == 0);
    printf("PASS get_timeout_and_drain\n");
}

static void test_get_pending_ignores_set_frame(void)
{
    reset_all();
    call(app_voice_get, "{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0}");
    s_voice_set_cb(1);
    assert(s_publish_count == 0);
    assert(app_voice_is_pending());
    s_voice_get_cb(2);
    assert(s_publish_count == 1);
    expect_published("{\"voiceboxv1\":231,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":1}");
    assert(!app_voice_is_pending());
    printf("PASS get_pending_ignores_set_frame\n");
}

static void test_second_set_while_pending_is_busy(void)
{
    reset_all();
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":3,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":3,\"act_id\":0,\"result\":0,\"code\":67}");
    assert(s_voice_set_count == 1);
    printf("PASS second_set_while_pending_is_busy\n");
}

static void test_busy_with_cwsl_and_volume(void)
{
    reset_all();
    s_cwsl_pending = true;
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":67}");
    call(app_voice_get, "{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0,\"result\":0,\"code\":67}");
    assert(s_voice_set_count == 0);
    assert(s_voice_query_count == 0);

    reset_all();
    call(app_volume_set, "{\"voiceboxv1\":225,\"vol\":3,\"u\":1,\"act_id\":0}");
    assert(s_volume_send_count == 1);
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":67}");
    assert(s_voice_set_count == 0);

    reset_all();
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    s_publish_count = 0;
    call(app_volume_set, "{\"voiceboxv1\":225,\"vol\":3,\"u\":1,\"act_id\":0}");
    assert(s_publish_count == 1);
    expect_published("{\"voiceboxv1\":225,\"vol\":3,\"u\":1,\"act_id\":0,\"result\":0}");
    assert(s_volume_send_count == 0);
    printf("PASS busy_with_cwsl_and_volume\n");
}

static void test_ota_and_order(void)
{
    reset_all();
    s_ota_held = true;
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":69}");
    call(app_voice_get, "{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":231,\"u\":55519,\"act_id\":0,\"result\":0,\"code\":69}");
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":5,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":5,\"act_id\":0,\"result\":0,\"code\":64}");
    assert(s_voice_set_count == 0);
    assert(s_voice_query_count == 0);
    printf("PASS ota_and_order\n");
}

static void test_send_fail_and_unsolicited(void)
{
    reset_all();
    s_voice_set_ret = -1;
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":230,\"u\":55519,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":70}");
    assert(!app_voice_is_pending());
    s_publish_count = 0;
    s_voice_set_cb(1);
    s_voice_get_cb(2);
    assert(s_publish_count == 0);
    printf("PASS send_fail_and_unsolicited\n");
}

#define B225(v, r) "{\"voiceboxv1\":225,\"vol\":" #v ",\"u\":0,\"act_id\":1,\"result\":" #r "}"
#define B230OK(v) "{\"voiceboxv1\":230,\"u\":0,\"voice_id\":" #v ",\"act_id\":1,\"result\":1}"
#define B230ERR(v, c) "{\"voiceboxv1\":230,\"u\":0,\"voice_id\":" #v ",\"act_id\":1,\"result\":0,\"code\":" #c "}"

static void set_state(uint8_t level, uint8_t muted)
{
    s_state_valid = 0;
    s_state_level = level;
    s_state_muted = muted;
}

static void vol_case(uint8_t level, uint8_t muted, bool up, uint8_t sent,
                     uint8_t cb_level, uint8_t cb_muted, const char *expected)
{
    reset_all();
    set_state(level, muted);
    assert(app_volume_button_step(up));
    assert(s_volume_send_count == 1);
    assert(s_volume_send_arg == sent);
    assert(s_publish_count == 0);
    assert(app_volume_is_pending());
    s_vol_cb(cb_level, 1, cb_muted);
    assert(s_publish_count == 1);
    expect_published(expected);
    assert(!app_volume_is_pending());
}

static void test_vol_steps(void)
{
    vol_case(4, 0, true, 5, 5, 0, B225(5, 1));
    vol_case(4, 0, false, 3, 3, 0, B225(3, 1));
    vol_case(7, 0, true, 7, 7, 0, B225(7, 1));
    vol_case(1, 0, false, 1, 1, 0, B225(1, 1));
    vol_case(4, 1, true, 1, 1, 0, B225(1, 1));
    printf("PASS vol_up_known/down_known/at_max/at_min/up_muted\n");
}

static void test_vol_down_muted(void)
{
    reset_all();
    set_state(4, 1);
    assert(app_volume_button_step(false));
    assert(s_mute_send_count == 1);
    assert(s_volume_send_count == 0);
    s_vol_cb(4, 1, 1);
    expect_published(B225(0, 1));
    assert(s_publish_count == 1);
    printf("PASS vol_down_muted\n");
}

static void test_vol_unknown_queries_first(void)
{
    reset_all();
    assert(app_volume_button_step(true));
    assert(s_volume_query_count == 1);
    assert(s_volume_send_count == 0);
    s_vol_cb(3, 1, 0);
    assert(s_volume_send_count == 1);
    assert(s_volume_send_arg == 4);
    assert(s_publish_count == 0);
    assert(app_volume_is_pending());
    s_vol_cb(4, 1, 0);
    expect_published(B225(4, 1));
    assert(s_publish_count == 1);
    assert(!app_volume_is_pending());
    printf("PASS vol_unknown_queries_first\n");
}

static void test_vol_query_timeout(void)
{
    reset_all();
    assert(app_volume_button_step(true));
    s_mock_tick = 2000;
    app_volume_poll();
    expect_published(B225(0, 0));
    assert(s_publish_count == 1);
    assert(app_volume_is_pending());
    printf("PASS vol_query_timeout\n");
}

static void test_vol_set_rejected_and_timeout(void)
{
    reset_all();
    set_state(4, 0);
    assert(app_volume_button_step(true));
    s_vol_cb(4, 0, 0);
    expect_published(B225(5, 0));
    assert(s_publish_count == 1);

    reset_all();
    set_state(4, 0);
    assert(app_volume_button_step(true));
    s_mock_tick = 2000;
    app_volume_poll();
    expect_published(B225(5, 0));
    assert(s_publish_count == 1);
    printf("PASS vol_set_rejected/vol_set_timeout\n");
}

static void test_vol_busy(void)
{
    reset_all();
    set_state(4, 0);
    s_cwsl_pending = true;
    assert(!app_volume_button_step(true));
    s_cwsl_pending = false;

    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":5,\"voice_id\":2,\"act_id\":0}");
    s_publish_count = 0;
    assert(!app_volume_button_step(true));
    reset_all();
    set_state(4, 0);

    call(app_volume_set, "{\"voiceboxv1\":225,\"vol\":3,\"u\":1,\"act_id\":0}");
    s_volume_send_count = 0;
    assert(!app_volume_button_step(true));
    assert(s_volume_send_count == 0);
    assert(s_publish_count == 0);

    reset_all();
    set_state(4, 0);
    s_ota_held = true;
    assert(!app_volume_button_step(true));
    assert(s_volume_send_count == 0);
    assert(s_mute_send_count == 0);
    assert(s_volume_query_count == 0);
    assert(s_publish_count == 0);
    printf("PASS vol_busy\n");
}

static void test_vol_tx_fail(void)
{
    reset_all();
    set_state(4, 0);
    s_volume_send_ret = -1;
    assert(app_volume_button_step(true));
    expect_published(B225(5, 0));
    assert(s_publish_count == 1);
    assert(!app_volume_is_pending());
    printf("PASS vol_tx_fail\n");
}

static void test_voice_next_known(void)
{
    reset_all();
    s_voice_id_ret = 2;
    assert(app_voice_button_next());
    assert(s_voice_set_count == 1);
    assert(s_voice_set_arg == 3);
    s_voice_set_cb(1);
    expect_published(B230OK(3));

    reset_all();
    s_voice_id_ret = 4;
    assert(app_voice_button_next());
    assert(s_voice_set_arg == 1);
    s_voice_set_cb(1);
    expect_published(B230OK(1));
    printf("PASS voice_next_known/wraps\n");
}

static void test_voice_next_unknown_queries_first(void)
{
    reset_all();
    assert(app_voice_button_next());
    assert(s_voice_query_count == 1);
    assert(s_voice_set_count == 0);
    s_voice_get_cb(2);
    assert(s_voice_set_count == 1);
    assert(s_voice_set_arg == 3);
    assert(s_publish_count == 0);
    s_voice_set_cb(1);
    expect_published(B230OK(3));
    assert(s_publish_count == 1);
    assert(!app_voice_is_pending());
    printf("PASS voice_next_unknown_queries_first\n");
}

static void test_voice_next_rejected_and_timeouts(void)
{
    reset_all();
    s_voice_id_ret = 1;
    assert(app_voice_button_next());
    s_voice_set_cb(0);
    expect_published(B230ERR(2, 64));

    reset_all();
    assert(app_voice_button_next());
    s_mock_tick = 2000;
    app_voice_poll();
    expect_published(B230ERR(0, 52));
    assert(s_publish_count == 1);
    assert(app_voice_is_pending());
    s_voice_get_cb(3);
    assert(s_publish_count == 1);
    assert(!app_voice_is_pending());

    reset_all();
    s_voice_id_ret = 1;
    assert(app_voice_button_next());
    s_mock_tick = 2000;
    app_voice_poll();
    expect_published(B230ERR(2, 52));
    assert(s_publish_count == 1);
    printf("PASS voice_next_rejected/query_timeout/set_timeout\n");
}

static void test_voice_busy(void)
{
    reset_all();
    s_voice_id_ret = 1;
    s_cwsl_pending = true;
    assert(!app_voice_button_next());
    s_cwsl_pending = false;
    call(app_volume_set, "{\"voiceboxv1\":225,\"vol\":3,\"u\":1,\"act_id\":0}");
    s_publish_count = 0;
    assert(!app_voice_button_next());
    reset_all();
    s_voice_id_ret = 1;
    s_ota_held = true;
    assert(!app_voice_button_next());
    assert(s_voice_set_count == 0);
    assert(s_voice_query_count == 0);
    assert(s_publish_count == 0);
    printf("PASS voice_busy\n");
}

static void test_voice_tx_fail(void)
{
    reset_all();
    s_voice_id_ret = 1;
    s_voice_set_ret = -1;
    assert(app_voice_button_next());
    expect_published(B230ERR(2, 70));
    assert(!app_voice_is_pending());

    reset_all();
    s_voice_query_ret = -1;
    assert(app_voice_button_next());
    expect_published(B230ERR(0, 70));
    assert(!app_voice_is_pending());
    printf("PASS voice_tx_fail\n");
}

static void test_app_busy_during_button(void)
{
    reset_all();
    s_voice_id_ret = 1;
    assert(app_voice_button_next());
    s_publish_count = 0;
    call(app_voice_set, "{\"voiceboxv1\":230,\"u\":5,\"voice_id\":2,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":230,\"u\":5,\"voice_id\":2,\"act_id\":0,\"result\":0,\"code\":67}");

    reset_all();
    set_state(4, 0);
    assert(app_volume_button_step(true));
    s_publish_count = 0;
    call(app_volume_set, "{\"voiceboxv1\":225,\"vol\":3,\"u\":5,\"act_id\":0}");
    expect_published("{\"voiceboxv1\":225,\"vol\":3,\"u\":5,\"act_id\":0,\"result\":0}");
    assert(s_publish_count == 1);
    printf("PASS app_busy_during_button\n");
}

#define EV(i, e) app_button_voicebox_event((i), (e))

static void test_button_post_rules(void)
{
    /* Handle is static: reset it so the pre-init check is deterministic. */
    s_button_queue = NULL;
    assert(!app_button_post(APP_BUTTON_ACTION_VOLUME_UP));
    app_button_init();
    assert(!app_button_post(APP_BUTTON_ACTION_SMART_CONFIG));
    assert(!app_button_post(APP_BUTTON_ACTION_LED_AP_CONFIG));
    assert(app_button_post(APP_BUTTON_ACTION_VOLUME_UP));
    printf("PASS button_post_rules\n");
}

#if VOICEBOX_HW == VOICEBOX_HW_V2
static void test_button_classify_volume_press(void)
{
    assert(EV(0, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_VOLUME_UP);
    assert(EV(2, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_VOLUME_DOWN);
    assert(EV(1, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_NONE);
    assert(EV(0, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_NONE);
    assert(EV(3, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_NONE);
    printf("PASS button_classify_volume_press\n");
}

static void test_button_classify_volume_hold_no_config(void)
{
    uint32_t idx;

    for (idx = 0; idx <= 2U; idx += 2U) {
        app_button_action_t press = (idx == 0U) ? APP_BUTTON_ACTION_VOLUME_UP
                                                : APP_BUTTON_ACTION_VOLUME_DOWN;
        assert(EV(idx, APP_BTN_EVT_PRESSED) == press);
        assert(EV(idx, APP_BTN_EVT_HOLD) == APP_BUTTON_ACTION_NONE);
        assert(EV(idx, APP_BTN_EVT_HOLD_SO_LONG) == APP_BUTTON_ACTION_NONE);
        assert(EV(idx, APP_BTN_EVT_HOLD_HONG_PHIM) == APP_BUTTON_ACTION_NONE);
        assert(EV(idx, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_NONE);
    }
    printf("PASS button_classify_volume_hold_no_config\n");
}
#else
/* V1: one function button at index 0, no volume buttons */
static void test_button_classify_v1_single_button(void)
{
    assert(APP_BUTTON_INDEX_FUNCTION == 0U);
    assert(EV(0, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_NONE);
    assert(EV(0, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_VOICE_NEXT);
    assert(EV(1, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_NONE);
    assert(EV(2, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_NONE);
    assert(EV(1, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_NONE);
    printf("PASS button_classify_v1_single_button\n");
}
#endif

static void test_button_classify_function_short(void)
{
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_NONE);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_VOICE_NEXT);
    printf("PASS button_classify_function_short\n");
}

static void test_button_classify_function_hold(void)
{
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_HOLD) == APP_BUTTON_ACTION_LED_SMART_CONFIG);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_SMART_CONFIG);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_NONE);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_VOICE_NEXT);
    printf("PASS button_classify_function_hold\n");
}

static void test_button_classify_function_hold_long(void)
{
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_HOLD) == APP_BUTTON_ACTION_LED_SMART_CONFIG);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_HOLD_SO_LONG) == APP_BUTTON_ACTION_LED_AP_CONFIG);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_HOLD_HONG_PHIM) == APP_BUTTON_ACTION_NONE);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_AP_CONFIG);
    printf("PASS button_classify_function_hold_long\n");
}

static void test_button_classify_release_without_press(void)
{
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_NONE);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_PRESSED) == APP_BUTTON_ACTION_NONE);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_VOICE_NEXT);
    assert(EV(APP_BUTTON_INDEX_FUNCTION, APP_BTN_EVT_RELEASED) == APP_BUTTON_ACTION_NONE);
    printf("PASS button_classify_release_without_press\n");
}

static void test_button_poll_runs_actions(void)
{
    reset_all();
    app_button_init();
    set_state(4, 0);
    s_voice_id_ret = 2;
    assert(app_button_post(APP_BUTTON_ACTION_VOLUME_UP));
    app_button_poll();
    assert(s_volume_send_count == 1 && s_volume_send_arg == 5);
    s_vol_cb(5, 1, 0);
    set_state(5, 0);
    assert(app_button_post(APP_BUTTON_ACTION_VOLUME_DOWN));
    app_button_poll();
    assert(s_volume_send_count == 2 && s_volume_send_arg == 4);
    s_vol_cb(4, 1, 0);
    assert(app_button_post(APP_BUTTON_ACTION_VOICE_NEXT));
    app_button_poll();
    assert(s_voice_set_count == 1 && s_voice_set_arg == 3);
    s_voice_set_cb(1);
    assert(s_buzz_type_count == 0);
    printf("PASS button_poll_runs_actions\n");
}

static void test_button_poll_busy_beeps(void)
{
    reset_all();
    app_button_init();
    s_cwsl_pending = true;
    assert(app_button_post(APP_BUTTON_ACTION_VOLUME_UP));
    app_button_poll();
    assert(s_buzz_type_count == 1 && s_buzz_type_arg == 1U);
    assert(s_volume_send_count == 0);
    printf("PASS button_poll_busy_beeps\n");
}

static void test_button_poll_busy_learning_silent(void)
{
    reset_all();
    app_button_init();
    s_cwsl_pending = true;
    s_cwsl_learning = true;
    assert(app_button_post(APP_BUTTON_ACTION_VOLUME_DOWN));
    app_button_poll();
    assert(s_buzz_type_count == 0);
    assert(s_volume_send_count == 0 && s_volume_query_count == 0 && s_mute_send_count == 0);
    printf("PASS button_poll_busy_learning_silent\n");
}

static void test_button_second_press_while_pending_beeps(void)
{
    reset_all();
    app_button_init();
    set_state(4, 0);
    assert(app_button_post(APP_BUTTON_ACTION_VOLUME_UP));
    assert(app_button_post(APP_BUTTON_ACTION_VOLUME_UP));
    app_button_poll();
    assert(s_volume_send_count == 1);
    assert(s_buzz_type_count == 1 && s_buzz_type_arg == 1U);
    printf("PASS button_second_press_while_pending_beeps\n");
}

int main(void)
{
    test_set_ok();
    test_set_chip_rejects();
    test_set_rejects_invalid_requests();
    test_get_ok();
    test_get_rejects_missing_act_id();
    test_timeout_then_drain_expires();
    test_late_set_reply_only_ends_drain();
    test_get_frame_does_not_end_set_drain();
    test_get_timeout_and_drain();
    test_get_pending_ignores_set_frame();
    test_second_set_while_pending_is_busy();
    test_busy_with_cwsl_and_volume();
    test_ota_and_order();
    test_send_fail_and_unsolicited();
    test_vol_steps();
    test_vol_down_muted();
    test_vol_unknown_queries_first();
    test_vol_query_timeout();
    test_vol_set_rejected_and_timeout();
    test_vol_busy();
    test_vol_tx_fail();
    test_voice_next_known();
    test_voice_next_unknown_queries_first();
    test_voice_next_rejected_and_timeouts();
    test_voice_busy();
    test_voice_tx_fail();
    test_app_busy_during_button();
    test_button_post_rules();
#if VOICEBOX_HW == VOICEBOX_HW_V2
    test_button_classify_volume_press();
    test_button_classify_volume_hold_no_config();
#else
    test_button_classify_v1_single_button();
#endif
    test_button_classify_function_short();
    test_button_classify_function_hold();
    test_button_classify_function_hold_long();
    test_button_classify_release_without_press();
    test_button_poll_runs_actions();
    test_button_poll_busy_beeps();
    test_button_poll_busy_learning_silent();
    test_button_second_press_while_pending_beeps();
#if VOICEBOX_HW == VOICEBOX_HW_V2
    printf("ALL 46 VOICE TESTS PASSED\n");
#else
    printf("ALL 45 VOICE TESTS PASSED (V1)\n");
#endif
    return 0;
}
