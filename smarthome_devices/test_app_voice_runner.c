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
int32_t aiio_ci13162_uart_send_volume(uint8_t value)
{
    (void)value;
    s_volume_send_count++;
    return 0;
}

int32_t aiio_ci13162_uart_query_volume(void)
{
    return 0;
}

void aiio_ci13162_uart_set_volume_callback(aiio_ci13162_volume_info_cb_t callback)
{
    (void)callback;
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

static TickType_t s_mock_tick = 0;
TickType_t xTaskGetTickCount(void) { return s_mock_tick; }

/* Include the real implementations */
#include "components/application/app_voice.c"
#include "components/application/app_volume.c"

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
    s_ota_held = false;
    s_ci_ota_busy = false;
    s_ota_busy = false;
    s_cwsl_pending = false;
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
    printf("ALL 14 VOICE TESTS PASSED\n");
    return 0;
}
