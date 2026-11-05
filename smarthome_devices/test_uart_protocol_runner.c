#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "hosal_uart.h"
#include "bl_gpio.h"
#include "bl602_uart.h"
#include "bl602_glb.h"
#include "bl602_hbn.h"
#include "bl_sys.h"
#include "bl_wdt.h"

#define AIIO_CI13162_UART_ENABLE 1
#include "components/hardware/aiio_ci13162_uart.h"

#define os_trace printf

/* Guard unwanted SDK headers */
#define _USER_CONFIG_H_
#define __EASY_FLASH_COMMON_H__
#define _APP_MQTT_H__
#define _APP_MQTT_H_

#include "components/app_config/system_typedef.h"

#define SWITCH_IDLE_LEVEL 0
#define SWITCH_ACTIVE_LEVEL 1
#define BUZZ_PIN 17

uint32_t device_type = FANDC;
char *gs_dev_type[] = {"wswitch3v", "lhswitch3v", "swminiv2", "sswitch2v", "swshockv2", "swshohuv2", "fanoem", "fandc", "fanac", "fanproacv2", "voiceboxv1"};

static char s_last_app_send_data[512] = {0};
static int s_app_send_data_count = 0;
void app_send_data(char *data, uint32_t length) {
    if (length < sizeof(s_last_app_send_data)) {
        memcpy(s_last_app_send_data, data, length);
        s_last_app_send_data[length] = '\0';
    } else {
        strncpy(s_last_app_send_data, data, sizeof(s_last_app_send_data) - 1);
        s_last_app_send_data[sizeof(s_last_app_send_data) - 1] = '\0';
    }
    s_app_send_data_count++;
}

static int s_mock_voice_support = 1;
bool hw_device_support_voice(void) {
    if (s_mock_voice_support == -1) {
        return (device_type == VOICEBOX);
    }
    return s_mock_voice_support != 0;
}

static uint8_t s_mock_gpio_buzz = 0;
void gpio_set_level(int pin, int level) {
    (void)pin;
    s_mock_gpio_buzz = (uint8_t)level;
}
void gpio_set_output(int pin) { (void)pin; }
void hw_74hc595_set_bit(uint8_t bit, uint8_t level) {
    (void)bit;
    s_mock_gpio_buzz = level;
}

#define AIIO_CI13162_UART_ENABLE 1
#define BUILD_TEST 1
volatile bool sys_log_all_enable = false;

/* Captured UART TX buffer */
static char s_uart_tx_buf[512] = {0};
static int s_uart_tx_count = 0;

static int mock_uart_tx(const void *data, uint32_t length) {
    if (length < sizeof(s_uart_tx_buf)) {
        memcpy(s_uart_tx_buf, data, length);
        s_uart_tx_buf[length] = '\0';
    }
    s_uart_tx_count++;
    return (int)length;
}

static hosal_uart_dev_t s_last_send_dev, s_last_ioctl_dev, s_last_init_dev;
static int s_init_calls, s_send_calls, s_ioctl_calls;
int hosal_uart_send(hosal_uart_dev_t *dev, const void *data, uint32_t length) {
    s_last_send_dev = *dev; s_send_calls++;
    return mock_uart_tx(data, length);
}
int hosal_uart_receive(hosal_uart_dev_t *dev, void *data, uint32_t length) {
    (void)dev; (void)data; (void)length;
    return 0;
}
int hosal_uart_ioctl(hosal_uart_dev_t *dev, int ctl, void *arg) {
    (void)ctl; (void)arg;
    s_last_ioctl_dev = *dev; s_ioctl_calls++;
    return 0;
}
int hosal_uart_init(hosal_uart_dev_t *dev) { s_last_init_dev = *dev; s_init_calls++; return 0; }
int hosal_uart_init_only_tx(hosal_uart_dev_t *dev) { (void)dev; return 0; }
int hosal_uart_finalize(hosal_uart_dev_t *dev) { (void)dev; return 0; }
int hosal_uart_callback_set(hosal_uart_dev_t *dev, int c, int (*p)(void *), void *a) {
    (void)dev; (void)c; (void)p; (void)a; return 0;
}

void vTaskDelay(TickType_t xTicksToDelay) { (void)xTicksToDelay; }
static TickType_t s_mock_tick = 0;
TickType_t xTaskGetTickCount(void) { return s_mock_tick; }
size_t xStreamBufferReceive(StreamBufferHandle_t xStreamBuffer, void *pvRxData, size_t xBufferLengthBytes, TickType_t xTicksToWait) {
    (void)xStreamBuffer; (void)pvRxData; (void)xBufferLengthBytes; (void)xTicksToWait; return 0;
}
size_t xStreamBufferReset(StreamBufferHandle_t xStreamBuffer) { (void)xStreamBuffer; return 0; }
size_t xStreamBufferSendFromISR(StreamBufferHandle_t xStreamBuffer, const void *pvTxData, size_t xDataLengthBytes, BaseType_t *pxHigherPriorityTaskWoken) {
    (void)xStreamBuffer; (void)pvTxData; (void)xDataLengthBytes; (void)pxHigherPriorityTaskWoken; return 0;
}
StreamBufferHandle_t xStreamBufferCreateStatic(size_t a, size_t b, uint8_t *c, StaticStreamBuffer_t *d) {
    (void)a; (void)b; (void)c; (void)d; return (StreamBufferHandle_t)1;
}
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *p) { (void)p; return (SemaphoreHandle_t)1; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t t) { (void)s; (void)t; return pdTRUE; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t s) { (void)s; return pdTRUE; }
TaskHandle_t xTaskCreateStatic(void (*f)(void *), const char *n, const uint32_t s, void *p, uint32_t pr, StackType_t *sb, StaticTask_t *tb) {
    (void)f; (void)n; (void)s; (void)p; (void)pr; (void)sb; (void)tb; return (TaskHandle_t)1;
}
static int s_reset_por_count = 0;
void bl_sys_reset_por(void) {
    s_reset_por_count++;
}

/* Mocks for application callbacks */
static uint8_t s_last_cwsl_notify_cmd_id = 0;
static uint8_t s_last_cwsl_notify_cwsl = 0;
static int s_cwsl_notify_count = 0;
void app_cwsl_notify_voice_cmd(uint8_t cmd_id, uint8_t cwsl) {
    s_last_cwsl_notify_cmd_id = cmd_id;
    s_last_cwsl_notify_cwsl = cwsl;
    s_cwsl_notify_count++;
}

static uint8_t s_last_vol_report_level = 0;
static uint8_t s_last_vol_report_cmd_id = 0;
static uint8_t s_last_vol_report_cwsl = 0;
static int s_vol_report_count = 0;
void app_volume_report_voice(uint8_t level, uint8_t cmd_id, uint8_t cwsl) {
    s_last_vol_report_level = level;
    s_last_vol_report_cmd_id = cmd_id;
    s_last_vol_report_cwsl = cwsl;
    s_vol_report_count++;
}

static uint8_t s_last_fan_respond_mode = 0;
static uint8_t s_last_fan_respond_speed = 0;
static uint8_t s_last_fan_respond_cmd_id = 0;
static uint8_t s_last_fan_respond_cwsl = 0;
static int s_fan_respond_count = 0;
void app_fan_respond_control_voice(uint8_t mode, uint8_t speed, uint8_t pcn, uint8_t swing, uint8_t light, uint32_t uid, uint8_t act_id, uint8_t cmd_id, uint8_t cwsl) {
    (void)pcn; (void)swing; (void)light; (void)uid; (void)act_id;
    s_last_fan_respond_mode = mode;
    s_last_fan_respond_speed = speed;
    s_last_fan_respond_cmd_id = cmd_id;
    s_last_fan_respond_cwsl = cwsl;
    s_fan_respond_count++;
}

static uint32_t s_last_timer_set = 0;
static uint8_t s_last_timer_cmd_id = 0;
static uint8_t s_last_timer_cwsl = 0;
static int s_timer_voice_count = 0;
void m_fan_set_time_off_voice(uint32_t time_set, uint8_t cmd_id, uint8_t cwsl) {
    s_last_timer_set = time_set;
    s_last_timer_cmd_id = cmd_id;
    s_last_timer_cwsl = cwsl;
    s_timer_voice_count++;
}

#include "components/middle/fan/m_fan.h"

static m_fan_info_t s_mock_fan_info;
m_fan_info_t *m_fan_get_instance(void) {
    return &s_mock_fan_info;
}

volatile uint8_t prevent_buzz = 0;
static int s_fan_set_speed_call_count = 0;
static uint8_t s_last_fan_set_speed_mode = 0;
static uint8_t s_last_fan_set_speed_speed = 0;

void m_fan_set_speed(uint8_t mode_set, uint8_t speed_set) {
    s_fan_set_speed_call_count++;
    s_last_fan_set_speed_mode = mode_set;
    s_last_fan_set_speed_speed = speed_set;
    s_mock_fan_info.mode = mode_set;
    s_mock_fan_info.speed = speed_set;
}

void m_fan_set_swing(uint8_t swing) {
    s_mock_fan_info.swing = swing;
}

/* Include aiio_ci13162_uart.c and buzz.c */
#include "components/hardware/aiio_ci13162_uart.c"
#include "components/hardware/buzz.c"

#define MOCK_Q_CMD ((QueueHandle_t)(uintptr_t)1)
#define MOCK_Q_ERR ((QueueHandle_t)(uintptr_t)2)

QueueHandle_t xQueueCreateStatic(uint32_t a, uint32_t b, uint8_t *c, StaticQueue_t *d) {
    (void)a; (void)c; (void)d;
    if (b == sizeof(aiio_ci13162_message_t)) {
        return MOCK_Q_CMD;
    }
    return MOCK_Q_ERR;
}

/* Queue mock for aiio_ci13162_message_t */
static aiio_ci13162_message_t s_test_q[16];
static int s_test_q_head = 0;
static int s_test_q_tail = 0;

/* Queue mock for aiio_ci13162_error_event_t */
static aiio_ci13162_error_event_t s_err_q[16];
static int s_err_q_head = 0;
static int s_err_q_tail = 0;

BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    (void)xTicksToWait;
    if (xQueue == MOCK_Q_ERR || xQueue == s_aiio_ci13162_error_queue) {
        if (s_err_q_head == s_err_q_tail) {
            return pdFALSE;
        }
        memcpy(pvBuffer, &s_err_q[s_err_q_tail], sizeof(aiio_ci13162_error_event_t));
        s_err_q_tail = (s_err_q_tail + 1) % 16;
        return pdPASS;
    }
    if (s_test_q_head == s_test_q_tail) {
        return pdFALSE;
    }
    memcpy(pvBuffer, &s_test_q[s_test_q_tail], sizeof(aiio_ci13162_message_t));
    s_test_q_tail = (s_test_q_tail + 1) % 16;
    return pdPASS;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) {
    (void)xTicksToWait;
    if (xQueue == MOCK_Q_ERR || xQueue == s_aiio_ci13162_error_queue) {
        int next = (s_err_q_head + 1) % 16;
        if (next == s_err_q_tail) return pdFALSE;
        memcpy(&s_err_q[s_err_q_head], pvItemToQueue, sizeof(aiio_ci13162_error_event_t));
        s_err_q_head = next;
        return pdPASS;
    }
    int next = (s_test_q_head + 1) % 16;
    if (next == s_test_q_tail) return pdFALSE;
    memcpy(&s_test_q[s_test_q_head], pvItemToQueue, sizeof(aiio_ci13162_message_t));
    s_test_q_head = next;
    return pdPASS;
}

BaseType_t xQueueReset(QueueHandle_t xQueue) {
    if (xQueue == NULL || xQueue == MOCK_Q_CMD || xQueue == s_aiio_ci13162_command_queue) {
        s_test_q_head = 0;
        s_test_q_tail = 0;
    }
    if (xQueue == NULL || xQueue == MOCK_Q_ERR || xQueue == s_aiio_ci13162_error_queue) {
        s_err_q_head = 0;
        s_err_q_tail = 0;
    }
    return pdPASS;
}

static int s_resp_set_cb_count;
static uint8_t s_resp_set_cb_last;
static void test_resp_set_cb(uint8_t applied) {
    s_resp_set_cb_count++;
    s_resp_set_cb_last = applied;
}

static int s_voice_set_cb_count;
static uint8_t s_voice_set_cb_last;
static int s_voice_get_cb_count;
static uint8_t s_voice_get_cb_last;
static void test_voice_set_cb(uint8_t applied) {
    s_voice_set_cb_count++;
    s_voice_set_cb_last = applied;
}
static void test_voice_get_cb(uint8_t voice_id) {
    s_voice_get_cb_count++;
    s_voice_get_cb_last = voice_id;
}

static int s_volume_cb_count;
static uint8_t s_volume_cb_level;
static uint8_t s_volume_cb_result;
static uint8_t s_volume_cb_muted;
static void test_volume_cb(uint8_t level, uint8_t result, uint8_t muted) {
    s_volume_cb_count++;
    s_volume_cb_level = level;
    s_volume_cb_result = result;
    s_volume_cb_muted = muted;
}

static aiio_ci13162_parse_result_t parse(const char *line) {
    aiio_ci13162_message_t msg;
    return aiio_ci13162_parse_line(line, &msg);
}

static void queue_and_process(uint8_t command, uint8_t value) {
    aiio_ci13162_message_t m = {.command = command, .data_count = 1U, .data = {value}};
    xQueueSend(NULL, &m, 0U);
    aiio_ci13162_uart_process();
}

static void queue_and_process_volume_info(uint8_t level, uint8_t result, uint8_t muted) {
    aiio_ci13162_message_t m = {.command = AIIO_CI13162_CMD_VOLUME_INFO, .data_count = 3U,
                                .data = {level, result, muted}};
    xQueueSend(NULL, &m, 0U);
    aiio_ci13162_uart_process();
}

static void queue_and_process_volume_change(uint8_t level, uint8_t cmd_id, uint8_t cwsl) {
    aiio_ci13162_message_t m = {.command = AIIO_CI13162_CMD_VOLUME_CHANGE, .data_count = 1U,
                                .data = {level}, .cmd_id = cmd_id, .cwsl = cwsl};
    xQueueSend(NULL, &m, 0U);
    aiio_ci13162_uart_process();
}

static void queue_and_process_version(uint32_t version) {
    aiio_ci13162_message_t m = {.command = AIIO_CI13162_CMD_VERSION, .data_count = 1U, .version = version};
    xQueueSend(NULL, &m, 0U);
    aiio_ci13162_uart_process();
}

static void reset_test_state(void) {
    s_mock_tick = 0;
    s_aiio_ci13162_volume_muted = 0U;
    s_aiio_ci13162_volume_valid = 0U;
    s_volume_cb_count = 0;
    s_volume_cb_level = 0U;
    s_volume_cb_result = 0U;
    s_volume_cb_muted = 0U;
    s_aiio_ci13162_voice_id = 0U;
    s_aiio_ci13162_voice_set_sent = 0U;
    s_aiio_ci13162_voice_query_scheduled = 0U;
    s_aiio_ci13162_voice_query_attempts = 0U;
    aiio_ci13162_uart_set_voice_callbacks(NULL, NULL);
    s_voice_set_cb_count = 0;
    s_voice_set_cb_last = 0U;
    s_voice_get_cb_count = 0;
    s_voice_get_cb_last = 0U;
    s_aiio_ci13162_voice_version_valid = 0U;
    s_aiio_ci13162_version_query_scheduled = 0U;
    s_aiio_ci13162_version_query_paused = 0U;
    memset(&s_mock_fan_info, 0, sizeof(s_mock_fan_info));
    s_last_cwsl_notify_cmd_id = 0;
    s_last_cwsl_notify_cwsl = 0;
    s_cwsl_notify_count = 0;
    s_vol_report_count = 0;
    s_fan_respond_count = 0;
    s_timer_voice_count = 0;
    s_fan_set_speed_call_count = 0;
    s_uart_tx_count = 0;
    s_uart_tx_buf[0] = '\0';
    s_last_app_send_data[0] = '\0';
    s_app_send_data_count = 0;
    s_reset_por_count = 0;
    s_buzz_last = SWITCH_IDLE_LEVEL;
    s_buzz_active_is_long = 0U;
    xQueueReset(NULL);
    s_aiio_ci13162_command_queue = MOCK_Q_CMD;
    s_aiio_ci13162_error_queue = MOCK_Q_ERR;
}

int main(void) {
    printf("[TEST] Running native CI13162 UART & Protocol test suite...\n");

    /* TEST 1: Run built-in parser and assembler tests */
    {
        printf("--- Test 1: Built-in Parser & Assembler Tests ---\n");
        reset_test_state();
        aiio_ci13162_run_parser_tests();
        aiio_ci13162_run_assembler_tests();
        printf("  PASS: Built-in parser and assembler tests succeeded\n");
    }

    /* TEST 1b: CI13162 driver owns UART1 and leaves UART0 (log) alone */
    {
        printf("--- Test 1b: UART1 config ---\n");
        reset_test_state();
        s_mock_voice_support = 1;
        g_mock_uart_fun_sel_count = 0;
        assert(aiio_ci13162_uart_init() == 0);
        assert(s_init_calls == 1);
        /* SIG7 must be handed back to UART0_RXD so UART1_RXD is selected only on SIG(rx % 8). */
        assert(g_mock_uart_fun_sel_count == 1);
        assert(g_mock_uart_fun_sel_last_sig == GLB_UART_SIG_7);
        assert(g_mock_uart_fun_sel_last_fun == GLB_UART_SIG_FUN_UART0_RXD);
        assert(s_last_init_dev.config.uart_id == 1);
        assert(s_last_init_dev.config.tx_pin == 4);
        assert(s_last_init_dev.config.rx_pin == 3);
        assert(s_last_init_dev.config.baud_rate == 115200);
        assert(s_last_init_dev.config.mode == HOSAL_UART_MODE_INT);
        aiio_ci13162_uart_send_volume(3);
        assert(s_send_calls > 0);
        assert(s_last_send_dev.config.uart_id == 1);
        assert(aiio_ci13162_uart_ota_acquire() == 0);
        assert(aiio_ci13162_uart_ota_set_baud(921600) == 0);
        assert(s_ioctl_calls > 0);
        assert(s_last_ioctl_dev.config.uart_id == 1);
        aiio_ci13162_uart_ota_release();
        assert(g_mock_logall_disable_count == 0);
        assert(g_mock_logall_enable_count == 0);
        g_mock_gpio_last_out_pin = -1;
        g_mock_gpio_last_out_val = -1;
        aiio_ci13162_uart_hold_tx_idle();
        assert(g_mock_gpio_last_out_pin == 4);
        assert(g_mock_gpio_last_out_val == 1);
        assert(g_mock_uart_disable_count == 0);
        printf("  PASS: CI13162 uses UART1 only\n");
    }

    /* TEST 2: Duplicate key rejection */
    {
        printf("--- Test 2: Duplicate Key Rejection ---\n");
        aiio_ci13162_message_t msg;
        assert(aiio_ci13162_parse_line("{\"cmd\":1,\"cmd\":1,\"data\":[1],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":1,\"data\":[1],\"data\":[1],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":1,\"data\":[1],\"cmd_id\":7,\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":1,\"data\":[1],\"cmd_id\":7,\"cwsl\":255,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        printf("  PASS: Duplicate keys properly rejected\n");
    }

    /* TEST 3: Metadata rules */
    {
        printf("--- Test 3: Metadata Rules ---\n");
        aiio_ci13162_message_t msg;
        assert(aiio_ci13162_parse_line("{\"cmd\":1,\"data\":[0]}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":1,\"data\":[1],\"cmd_id\":7}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":1,\"data\":[1],\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":2,\"data\":[1]}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":7,\"data\":[0]}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[1]}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":13,\"data\":[0]}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);

        assert(aiio_ci13162_parse_line("{\"cmd\":0,\"data\":[],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":3,\"data\":[260813],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":5,\"data\":[32,7,0],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":6,\"data\":[7,5,0],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":9,\"data\":[3,1],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":10,\"data\":[1],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":14,\"data\":[0],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(aiio_ci13162_parse_line("{\"cmd\":200,\"data\":[0],\"cmd_id\":7,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        printf("  PASS: Metadata presence rules strictly enforced\n");
    }

    /* TEST 4: CMD_STATE (14) silent drop verification */
    {
        printf("--- Test 4: CMD_STATE Silent Drop ---\n");
        reset_test_state();

        s_mock_fan_info.mode = M_FAN_MODE_ON;
        s_mock_fan_info.speed = 2U;
        aiio_ci13162_message_t query_msg = {
            .command = AIIO_CI13162_CMD_STATE,
            .data_count = 1U,
            .data = {0U}
        };
        xQueueSend(NULL, &query_msg, 0U);
        aiio_ci13162_uart_process();

        /* MCU silently ignores CMD_STATE query: no TX, no fan state change, no MQTT response */
        assert(s_uart_tx_count == 0);
        assert(s_uart_tx_buf[0] == '\0');
        assert(s_mock_fan_info.mode == M_FAN_MODE_ON);
        assert(s_mock_fan_info.speed == 2U);
        assert(s_fan_set_speed_call_count == 0);
        assert(s_fan_respond_count == 0);
        assert(s_cwsl_notify_count == 0);

        /* Fan OFF state */
        s_mock_fan_info.mode = M_FAN_MODE_OFF;
        s_mock_fan_info.speed = M_FAN_SPEED_OFF;
        s_uart_tx_count = 0;
        s_uart_tx_buf[0] = '\0';
        xQueueSend(NULL, &query_msg, 0U);
        aiio_ci13162_uart_process();

        assert(s_uart_tx_count == 0);
        assert(s_uart_tx_buf[0] == '\0');
        assert(s_mock_fan_info.mode == M_FAN_MODE_OFF);
        assert(s_mock_fan_info.speed == M_FAN_SPEED_OFF);
        assert(s_fan_set_speed_call_count == 0);
        assert(s_fan_respond_count == 0);

        /* Fan PCN state */
        s_mock_fan_info.mode = M_FAN_MODE_PCN;
        s_uart_tx_count = 0;
        s_uart_tx_buf[0] = '\0';
        xQueueSend(NULL, &query_msg, 0U);
        aiio_ci13162_uart_process();

        assert(s_uart_tx_count == 0);
        assert(s_uart_tx_buf[0] == '\0');
        assert(s_mock_fan_info.mode == M_FAN_MODE_PCN);
        assert(s_fan_set_speed_call_count == 0);
        assert(s_fan_respond_count == 0);

        printf("  PASS: CMD_STATE silently dropped (TX empty, no state change, no MQTT)\n");
    }

    /* TEST 5: Voice TURN off with timer active cancels timer with metadata */
    {
        printf("--- Test 5: Voice TURN Off Cancels Timer ---\n");
        reset_test_state();

        s_mock_fan_info.mode = M_FAN_MODE_ON;
        s_mock_fan_info.speed = 2U;
        s_mock_fan_info.time_set = 7200000U;

        aiio_ci13162_message_t turn_off_msg = {
            .command = AIIO_CI13162_CMD_TURN,
            .data_count = 1U,
            .data = {0U},
            .cmd_id = 9U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &turn_off_msg, 0U);
        aiio_ci13162_uart_process();

        assert(s_timer_voice_count == 1);
        assert(s_last_timer_set == 0U);
        assert(s_last_timer_cmd_id == 9U);
        assert(s_last_timer_cwsl == 255U);

        assert(s_fan_set_speed_call_count == 1);
        assert(s_last_fan_set_speed_mode == M_FAN_MODE_OFF);

        assert(s_fan_respond_count == 1);
        assert(s_last_fan_respond_cmd_id == 9U);
        assert(s_last_fan_respond_cwsl == 255U);

        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 9U);
        assert(s_last_cwsl_notify_cwsl == 255U);

        printf("  PASS: Voice turn off canceled timer with metadata without duplicate publishes\n");
    }

    /* TEST 6: Voice commands maintain reporting when state is unchanged */
    {
        printf("--- Test 6: Report Unchanged State on Voice Commands ---\n");
        reset_test_state();

        s_mock_fan_info.mode = M_FAN_MODE_OFF;
        s_mock_fan_info.speed = M_FAN_SPEED_OFF;

        aiio_ci13162_message_t turn_off_msg = {
            .command = AIIO_CI13162_CMD_TURN,
            .data_count = 1U,
            .data = {0U},
            .cmd_id = 9U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &turn_off_msg, 0U);
        aiio_ci13162_uart_process();

        assert(s_fan_set_speed_call_count == 0);
        assert(s_fan_respond_count == 1);
        assert(s_last_fan_respond_cmd_id == 9U);
        assert(s_last_fan_respond_cwsl == 255U);
        assert(s_cwsl_notify_count == 1);

        printf("  PASS: Unchanged state reported on voice command\n");
    }

    /* TEST 7: Voice volume change carries metadata and does not complete pending SET/GET */
    {
        printf("--- Test 7: Voice Volume Change ---\n");
        reset_test_state();

        aiio_ci13162_message_t vol_msg = {
            .command = AIIO_CI13162_CMD_VOLUME_CHANGE,
            .data_count = 1U,
            .data = {5U},
            .cmd_id = 49U,
            .cwsl = 0U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &vol_msg, 0U);
        aiio_ci13162_uart_process();

        assert(s_vol_report_count == 1);
        assert(s_last_vol_report_level == 5U);
        assert(s_last_vol_report_cmd_id == 49U);
        assert(s_last_vol_report_cwsl == 0U);
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 49U);
        assert(s_last_cwsl_notify_cwsl == 0U);

        printf("  PASS: Volume change metadata passed through correctly\n");
    }

    /* TEST 8: Buzzer set type, upgrade from short to long, and duplicate long suppression */
    {
        printf("--- Test 8: Buzzer Hardware & API ---\n");
        reset_test_state();

        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 0U);
        assert(s_uart_tx_count == 1);
        assert(strcmp(s_uart_tx_buf, "{\"cmd\":200,\"data\":[0]}\r\n") == 0);

        hw_buzz_set_type(SWITCH_IDLE_LEVEL, 0U);

        s_uart_tx_count = 0;
        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 1U);
        assert(s_uart_tx_count == 1);
        assert(strcmp(s_uart_tx_buf, "{\"cmd\":200,\"data\":[1]}\r\n") == 0);

        /* Duplicate long buzz */
        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 1U);
        assert(s_uart_tx_count == 1);

        hw_buzz_set_type(SWITCH_IDLE_LEVEL, 0U);

        /* Upgrade from short to long */
        s_uart_tx_count = 0;
        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 0U);
        assert(s_uart_tx_count == 1);
        assert(strcmp(s_uart_tx_buf, "{\"cmd\":200,\"data\":[0]}\r\n") == 0);

        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 1U);
        assert(s_uart_tx_count == 2);
        assert(strcmp(s_uart_tx_buf, "{\"cmd\":200,\"data\":[1]}\r\n") == 0);

        printf("  PASS: Buzzer short/long, upgrade, and duplicate suppression verified\n");
    }

    /* TEST 9: Voice Command Synonyms and Learned Commands */
    {
        printf("--- Test 9: Voice Synonyms and Learned Commands ---\n");
        reset_test_state();

        /* Synonym turn on (cmd_id 8) */
        aiio_ci13162_message_t turn_on_syn = {
            .command = AIIO_CI13162_CMD_TURN,
            .data_count = 1U,
            .data = {1U},
            .cmd_id = 8U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &turn_on_syn, 0U);
        aiio_ci13162_uart_process();
        assert(s_fan_respond_count == 1);
        assert(s_last_fan_respond_cmd_id == 8U);
        assert(s_last_fan_respond_cwsl == 255U);
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 8U);

        /* Learned command turn off (cmd_id 9, cwsl 3) */
        reset_test_state();
        aiio_ci13162_message_t turn_off_learned = {
            .command = AIIO_CI13162_CMD_TURN,
            .data_count = 1U,
            .data = {0U},
            .cmd_id = 9U,
            .cwsl = 3U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &turn_off_learned, 0U);
        aiio_ci13162_uart_process();
        assert(s_fan_respond_count == 1);
        assert(s_last_fan_respond_cmd_id == 9U);
        assert(s_last_fan_respond_cwsl == 3U);
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 9U);
        assert(s_last_cwsl_notify_cwsl == 3U);

        /* Synonym timer (cmd_id 24: 1 tieng) */
        reset_test_state();
        aiio_ci13162_message_t timer_syn = {
            .command = AIIO_CI13162_CMD_TIMER,
            .data_count = 1U,
            .data = {1U},
            .cmd_id = 24U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &timer_syn, 0U);
        aiio_ci13162_uart_process();
        assert(s_timer_voice_count == 1);
        assert(s_last_timer_set == 3600U);
        assert(s_last_timer_cmd_id == 24U);
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 24U);

        printf("  PASS: Synonyms and learned commands correctly handled\n");
    }

    /* TEST 10: CMD_STATE Silent Drop across Natural, Auto Modes and Raw Packet */
    {
        printf("--- Test 10: CMD_STATE Silent Drop in Natural & Auto Modes ---\n");
        reset_test_state();
        aiio_ci13162_message_t query_msg = {
            .command = AIIO_CI13162_CMD_STATE,
            .data_count = 1U,
            .data = {0U}
        };

        /* Natural mode (4) */
        s_mock_fan_info.mode = M_FAN_MODE_NATURAL;
        s_mock_fan_info.speed = 1U;
        s_uart_tx_count = 0;
        s_uart_tx_buf[0] = '\0';
        xQueueSend(NULL, &query_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_uart_tx_count == 0);
        assert(s_uart_tx_buf[0] == '\0');
        assert(s_mock_fan_info.mode == M_FAN_MODE_NATURAL);
        assert(s_mock_fan_info.speed == 1U);
        assert(s_fan_set_speed_call_count == 0);
        assert(s_fan_respond_count == 0);

        /* Auto / Smart mode (5) */
        s_mock_fan_info.mode = M_FAN_MODE_AUTO;
        s_mock_fan_info.speed = 2U;
        s_uart_tx_count = 0;
        s_uart_tx_buf[0] = '\0';
        xQueueSend(NULL, &query_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_uart_tx_count == 0);
        assert(s_uart_tx_buf[0] == '\0');
        assert(s_mock_fan_info.mode == M_FAN_MODE_AUTO);
        assert(s_mock_fan_info.speed == 2U);
        assert(s_fan_set_speed_call_count == 0);
        assert(s_fan_respond_count == 0);

        /* Parse raw {"cmd":14,"data":[0]} frame and process via queue */
        aiio_ci13162_message_t parsed_msg;
        assert(aiio_ci13162_parse_line("{\"cmd\":14,\"data\":[0]}", &parsed_msg) == AIIO_CI13162_PARSE_OK);
        assert(parsed_msg.command == AIIO_CI13162_CMD_STATE);
        assert(parsed_msg.data_count == 1U);
        assert(parsed_msg.data[0] == 0U);

        s_uart_tx_count = 0;
        s_uart_tx_buf[0] = '\0';
        xQueueSend(NULL, &parsed_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_uart_tx_count == 0);
        assert(s_uart_tx_buf[0] == '\0');
        assert(s_fan_respond_count == 0);

        /* Verify handle_line drops CMD_STATE directly without queueing */
        xQueueReset(NULL);
        s_uart_tx_count = 0;
        s_uart_tx_buf[0] = '\0';
        char raw_line[] = "{\"cmd\":14,\"data\":[0]}";
        aiio_ci13162_handle_line(raw_line, strlen(raw_line));
        aiio_ci13162_message_t queue_check_msg;
        assert(xQueueReceive(s_aiio_ci13162_command_queue, &queue_check_msg, 0U) == pdFALSE);
        assert(s_uart_tx_count == 0);
        assert(s_uart_tx_buf[0] == '\0');
        assert(s_fan_respond_count == 0);

        /* Verify full byte-stream assembler drops CMD_STATE directly without queueing */
        aiio_ci13162_line_assembler_t stream_assembler;
        memset(&stream_assembler, 0, sizeof(stream_assembler));
        const char *stream_bytes = "{\"cmd\":14,\"data\":[0]}\r\n";
        for (size_t b_idx = 0; b_idx < strlen(stream_bytes); b_idx++) {
            aiio_ci13162_process_byte(&stream_assembler, (uint8_t)stream_bytes[b_idx]);
        }
        assert(xQueueReceive(s_aiio_ci13162_command_queue, &queue_check_msg, 0U) == pdFALSE);
        assert(s_uart_tx_count == 0);
        assert(s_uart_tx_buf[0] == '\0');
        assert(s_fan_respond_count == 0);

        printf("  PASS: Natural, Auto modes, raw line and byte stream silently drop CMD_STATE without queueing, TX or MQTT\n");
    }

    /* TEST 11: Volume Boundary Cases (Mute 0, Min 1, Max 7, Unmute) */
    {
        printf("--- Test 11: Volume Boundaries ---\n");
        reset_test_state();

        /* Mute cmd_id 54 -> data 0 */
        aiio_ci13162_message_t mute_msg = {
            .command = AIIO_CI13162_CMD_VOLUME_CHANGE,
            .data_count = 1U,
            .data = {0U},
            .cmd_id = 54U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &mute_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_vol_report_count == 1);
        assert(s_last_vol_report_level == 0U);
        assert(s_last_vol_report_cmd_id == 54U);

        /* Min volume cmd_id 50 -> data 1 */
        reset_test_state();
        aiio_ci13162_message_t min_vol_msg = {
            .command = AIIO_CI13162_CMD_VOLUME_CHANGE,
            .data_count = 1U,
            .data = {1U},
            .cmd_id = 50U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &min_vol_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_vol_report_count == 1);
        assert(s_last_vol_report_level == 1U);
        assert(s_last_vol_report_cmd_id == 50U);

        /* Max volume cmd_id 52 -> data 7 */
        reset_test_state();
        aiio_ci13162_message_t max_vol_msg = {
            .command = AIIO_CI13162_CMD_VOLUME_CHANGE,
            .data_count = 1U,
            .data = {7U},
            .cmd_id = 52U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &max_vol_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_vol_report_count == 1);
        assert(s_last_vol_report_level == 7U);
        assert(s_last_vol_report_cmd_id == 52U);

        /* Unmute cmd_id 55 -> data 4 */
        reset_test_state();
        aiio_ci13162_message_t unmute_msg = {
            .command = AIIO_CI13162_CMD_VOLUME_CHANGE,
            .data_count = 1U,
            .data = {4U},
            .cmd_id = 55U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &unmute_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_vol_report_count == 1);
        assert(s_last_vol_report_level == 4U);
        assert(s_last_vol_report_cmd_id == 55U);

        printf("  PASS: Volume boundary levels and unmute verified\n");
    }

    /* TEST 12: Consecutive Voice Commands and Non-Voice Source Isolation */
    {
        printf("--- Test 12: Consecutive Commands & Non-Voice Source Isolation ---\n");
        reset_test_state();

        /* Two consecutive voice commands in queue */
        aiio_ci13162_message_t cmd1 = {
            .command = AIIO_CI13162_CMD_MODE,
            .data_count = 1U,
            .data = {2U},
            .cmd_id = 13U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        aiio_ci13162_message_t cmd2 = {
            .command = AIIO_CI13162_CMD_TIMER,
            .data_count = 1U,
            .data = {2U},
            .cmd_id = 26U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &cmd1, 0U);
        aiio_ci13162_uart_process();

        assert(s_fan_respond_count == 1);
        assert(s_last_fan_respond_cmd_id == 13U);
        assert(s_last_fan_respond_cwsl == 255U);

        xQueueSend(NULL, &cmd2, 0U);
        aiio_ci13162_uart_process();

        assert(s_timer_voice_count == 1);
        assert(s_last_timer_set == 7200U);
        assert(s_last_timer_cmd_id == 26U);
        assert(s_last_timer_cwsl == 255U);

        printf("  PASS: Consecutive commands executed cleanly with distinct metadata\n");
    }

    /* TEST 13: Voice error reporting via MQTT cmd 299 */
    {
        printf("--- Test 13: Voice Error Reporting to MQTT (cmd 299) ---\n");
        reset_test_state();

        /* Scenario A: INVALID_DATA - Volume command with out-of-range data 8 */
        char bad_vol_line[] = "{\"cmd\":13,\"data\":[8],\"cmd_id\":49,\"cwsl\":255}";
        aiio_ci13162_handle_line(bad_vol_line, strlen(bad_vol_line));

        /* Application task processes events */
        aiio_ci13162_uart_process();

        assert(s_app_send_data_count == 1);
        assert(strstr(s_last_app_send_data, "\"fandc\":299") != NULL);
        assert(strstr(s_last_app_send_data, "\"reason\":\"INVALID_DATA\"") != NULL);
        assert(strstr(s_last_app_send_data, "\\\"cmd\\\":13") != NULL);
        assert(strstr(s_last_app_send_data, "\\\"data\\\":[8]") != NULL);

        /* Scenario B: UNKNOWN_COMMAND */
        reset_test_state();
        char unknown_cmd_line[] = "{\"cmd\":8,\"data\":[0]}";
        aiio_ci13162_handle_line(unknown_cmd_line, strlen(unknown_cmd_line));
        aiio_ci13162_uart_process();

        assert(s_app_send_data_count == 1);
        assert(strstr(s_last_app_send_data, "\"fandc\":299") != NULL);
        assert(strstr(s_last_app_send_data, "\"reason\":\"UNKNOWN_COMMAND\"") != NULL);

        /* Scenario C: INVALID_FORMAT - corrupt syntax */
        reset_test_state();
        char malformed_line[] = "{\"cmd\":1,\"corrupt";
        aiio_ci13162_handle_line(malformed_line, strlen(malformed_line));
        aiio_ci13162_uart_process();

        assert(s_app_send_data_count == 1);
        assert(strstr(s_last_app_send_data, "\"fandc\":299") != NULL);
        assert(strstr(s_last_app_send_data, "\"reason\":\"INVALID_FORMAT\"") != NULL);

        /* Scenario D: Valid command does NOT report error */
        reset_test_state();
        char valid_line[] = "{\"cmd\":1,\"data\":[1],\"cmd_id\":7,\"cwsl\":255}";
        aiio_ci13162_handle_line(valid_line, strlen(valid_line));
        aiio_ci13162_uart_process();

        assert(s_app_send_data_count == 0); /* No error MQTT published! */
        assert(s_fan_respond_count == 1);   /* Regular voice action executed! */

        printf("  PASS: Rejected voice commands properly reported via MQTT cmd 299\n");
    }

    /* TEST 14: VOICEBOX (voiceboxv1) CI13162 Voice Support, Scene Handling, Fan Bypass, and Buzzer */
    {
        printf("--- Test 14: VOICEBOX CI13162 Protocol & Fan Bypass ---\n");
        reset_test_state();
        s_mock_voice_support = -1; /* Use real voice support logic */

        /* 14.1: Verify hw_device_support_voice() */
        device_type = VOICEBOX;
        assert(hw_device_support_voice() == true);
        device_type = FANACIR;
        assert(hw_device_support_voice() == false);
        device_type = FANDC;
        assert(hw_device_support_voice() == false);
        device_type = FANAC;
        assert(hw_device_support_voice() == false);

        /* Set to VOICEBOX for remaining subtests */
        device_type = VOICEBOX;
        s_mock_voice_support = 1;

        /* 14.2: Buzzer over UART for VOICEBOX */
        reset_test_state();
        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 0U);
        assert(s_uart_tx_count == 1);
        assert(strcmp(s_uart_tx_buf, "{\"cmd\":200,\"data\":[0]}\r\n") == 0);
        hw_buzz_set_type(SWITCH_IDLE_LEVEL, 0U);

        /* Long buzzer */
        reset_test_state();
        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 1U);
        assert(s_uart_tx_count == 1);
        assert(strcmp(s_uart_tx_buf, "{\"cmd\":200,\"data\":[1]}\r\n") == 0);
        hw_buzz_set_type(SWITCH_IDLE_LEVEL, 0U);

        /* 14.3: Scene command executes via app_cwsl_notify_voice_cmd without fan touch */
        reset_test_state();
        aiio_ci13162_message_t scene_msg = {
            .command = AIIO_CI13162_CMD_SCENE,
            .data_count = 1U,
            .data = {1U}, /* scene 1 */
            .cmd_id = 56U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &scene_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 56U);
        assert(s_last_cwsl_notify_cwsl == 255U);
        assert(s_fan_respond_count == 0);
        assert(s_fan_set_speed_call_count == 0);

        /* 14.4: Control commands (TURN, MODE, TIMER) forward to app_cwsl_notify_voice_cmd on VOICEBOX without fan touch */
        reset_test_state();
        aiio_ci13162_message_t fan_turn_msg = {
            .command = AIIO_CI13162_CMD_TURN,
            .data_count = 1U,
            .data = {1U},
            .cmd_id = 7U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &fan_turn_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 7U);
        assert(s_last_cwsl_notify_cwsl == 255U);
        assert(s_fan_respond_count == 0);
        assert(s_fan_set_speed_call_count == 0);
        assert(s_app_send_data_count == 0);

        reset_test_state();
        aiio_ci13162_message_t fan_mode_msg = {
            .command = AIIO_CI13162_CMD_MODE,
            .data_count = 1U,
            .data = {1U},
            .cmd_id = 19U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &fan_mode_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 19U);
        assert(s_fan_respond_count == 0);

        reset_test_state();
        aiio_ci13162_message_t fan_timer_msg = {
            .command = AIIO_CI13162_CMD_TIMER,
            .data_count = 1U,
            .data = {1U},
            .cmd_id = 25U,
            .cwsl = 255U,
            .has_metadata = 1U
        };
        xQueueSend(NULL, &fan_timer_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 25U);
        assert(s_fan_respond_count == 0);

        /* 14.5: Reset command executes bl_sys_reset_por on VOICEBOX */
        reset_test_state();
        aiio_ci13162_message_t reset_msg = {
            .command = AIIO_CI13162_CMD_RESET,
            .data_count = 0U,
            .has_metadata = 0U
        };
        xQueueSend(NULL, &reset_msg, 0U);
        aiio_ci13162_uart_process();
        assert(s_reset_por_count == 1);
        assert(s_cwsl_notify_count == 0);
        assert(s_fan_respond_count == 0);

        device_type = FANDC; /* restore */
        printf("  PASS: VOICEBOX voice support, buzzer, scene execution, fan bypass and reset verified\n");
    }

    /* TEST 15: Scene 1..15 parser */
    {
        printf("--- Test 15: Scene 1..15 parser ---\n");
        aiio_ci13162_message_t msg;
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[11],\"cmd_id\":66,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[15],\"cmd_id\":70,\"cwsl\":3}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(msg.cmd_id == 70U && msg.cwsl == 3U && msg.data[0] == 15U);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[16],\"cmd_id\":71,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[15],\"cmd_id\":69,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":6,\"data\":[70,1,0]}", &msg) == AIIO_CI13162_PARSE_OK);
        /* CWSL_INFO answers QUERY for every cmd_id of the voice model, 1..103 */
        assert(aiio_ci13162_parse_line("{\"cmd\":6,\"data\":[71,1,0]}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(msg.data[0] == 71U);
        assert(aiio_ci13162_parse_line("{\"cmd\":6,\"data\":[103,1,0]}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(aiio_ci13162_parse_line("{\"cmd\":6,\"data\":[104,1,0]}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        printf("  PASS: Scene 1..15 parser verified\n");
    }
    /* TEST 16: CWSL LEARN with value */
    {
        printf("--- Test 16: CWSL LEARN with value ---\n");
        reset_test_state();
        uint8_t learn3[3] = {56U, 255U, 47U};
        uint8_t learn2[2] = {56U, 255U};
        assert(aiio_ci13162_uart_send_cwsl(1U, learn3, 3U) == 0);
        assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":4,\"data\":[1,56,255,47]}\r\n") == 0);
        assert(aiio_ci13162_uart_send_cwsl(1U, learn2, 2U) == 0);
        assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":4,\"data\":[1,56,255]}\r\n") == 0);
        assert(aiio_ci13162_uart_send_cwsl(2U, learn3, 3U) == -1);
        assert(aiio_ci13162_uart_send_cwsl(1U, learn3, 4U) == -1);
        printf("  PASS: CWSL LEARN with value verified\n");
    }

    /* TEST 17: CMD_RESP_SET (cmd 15) */
    {
        printf("--- Test 17: CMD_RESP_SET (cmd 15) ---\n");
        reset_test_state();
        assert(aiio_ci13162_uart_send_resp_set(56U, 47U) == 0);
        assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":15,\"data\":[56,47]}\r\n") == 0);
        s_aiio_ci13162_ota_held = 1U;
        assert(aiio_ci13162_uart_send_resp_set(56U, 47U) == -1);
        s_aiio_ci13162_ota_held = 0U;
        aiio_ci13162_message_t msg;
        assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[1]}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(msg.command == AIIO_CI13162_CMD_RESP_SET && msg.data_count == 1U && msg.data[0] == 1U);
        assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[0]}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[2]}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[]}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":15,\"data\":[1],\"cmd_id\":56,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_FORMAT);
        s_resp_set_cb_count = 0;
        aiio_ci13162_uart_set_resp_set_callback(test_resp_set_cb);
        aiio_ci13162_message_t m = {.command = AIIO_CI13162_CMD_RESP_SET, .data_count = 1U, .data = {0U}};
        xQueueSend(NULL, &m, 0U);
        aiio_ci13162_uart_process();
        assert(s_resp_set_cb_count == 1 && s_resp_set_cb_last == 0U);
        aiio_ci13162_uart_set_resp_set_callback(NULL);
        printf("  PASS: CMD_RESP_SET verified\n");
    }

    /* TEST 18: CMD_VOICE_SET / CMD_VOICE_GET (cmd 17/18) */
    {
        printf("--- Test 18: CMD_VOICE_SET / CMD_VOICE_GET (cmd 17/18) ---\n");
        reset_test_state();
        assert(aiio_ci13162_uart_send_voice_set(2U) == 0);
        assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":17,\"data\":[2]}\r\n") == 0);
        assert(aiio_ci13162_uart_send_voice_set(0U) == -1);
        assert(aiio_ci13162_uart_send_voice_set(5U) == -1);
        assert(aiio_ci13162_uart_query_voice() == 0);
        assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":18,\"data\":[]}\r\n") == 0);
        s_aiio_ci13162_ota_held = 1U;
        assert(aiio_ci13162_uart_send_voice_set(2U) == -1);
        assert(aiio_ci13162_uart_query_voice() == -1);
        s_aiio_ci13162_ota_held = 0U;
        /* parser */
        assert(parse("{\"cmd\":17,\"data\":[1]}") == AIIO_CI13162_PARSE_OK);
        assert(parse("{\"cmd\":17,\"data\":[0]}") == AIIO_CI13162_PARSE_OK);
        assert(parse("{\"cmd\":17,\"data\":[2]}") == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(parse("{\"cmd\":17,\"data\":[]}") == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(parse("{\"cmd\":17,\"data\":[1],\"cmd_id\":1,\"cwsl\":255}") == AIIO_CI13162_PARSE_INVALID_FORMAT);
        assert(parse("{\"cmd\":18,\"data\":[1]}") == AIIO_CI13162_PARSE_OK);
        assert(parse("{\"cmd\":18,\"data\":[4]}") == AIIO_CI13162_PARSE_OK);
        assert(parse("{\"cmd\":18,\"data\":[0]}") == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(parse("{\"cmd\":18,\"data\":[5]}") == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(parse("{\"cmd\":18,\"data\":[]}") == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(parse("{\"cmd\":18,\"data\":[1,2]}") == AIIO_CI13162_PARSE_INVALID_DATA);
        /* dispatch + cache */
        aiio_ci13162_uart_set_voice_callbacks(test_voice_set_cb, test_voice_get_cb);
        assert(aiio_ci13162_uart_get_voice_id() == 0U);
        assert(aiio_ci13162_uart_send_voice_set(3U) == 0);
        queue_and_process(AIIO_CI13162_CMD_VOICE_SET, 1U);  /* reply may come any time later */
        assert(s_voice_set_cb_count == 1 && s_voice_set_cb_last == 1U);
        assert(aiio_ci13162_uart_get_voice_id() == 3U);
        queue_and_process(AIIO_CI13162_CMD_VOICE_SET, 1U);  /* unsolicited: no value was sent */
        assert(s_voice_set_cb_count == 2 && aiio_ci13162_uart_get_voice_id() == 3U);
        assert(aiio_ci13162_uart_send_voice_set(2U) == 0);
        queue_and_process(AIIO_CI13162_CMD_VOICE_SET, 0U);
        assert(s_voice_set_cb_last == 0U && aiio_ci13162_uart_get_voice_id() == 3U);
        queue_and_process(AIIO_CI13162_CMD_VOICE_GET, 4U);
        assert(s_voice_get_cb_count == 1 && s_voice_get_cb_last == 4U);
        assert(aiio_ci13162_uart_get_voice_id() == 4U);
        printf("  PASS: CMD_VOICE_SET/GET verified\n");
    }

    /* TEST 19: boot voice query after version */
    {
        printf("--- Test 19: boot voice query after version ---\n");
        reset_test_state();
        queue_and_process_version(261102U);
        int base = s_uart_tx_count;
        aiio_ci13162_uart_poll();
        assert(s_uart_tx_count == base + 1);
        assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":18,\"data\":[]}\r\n") == 0);
        s_mock_tick = 999;  aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 1);
        s_mock_tick = 1000; aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 2);
        s_mock_tick = 2000; aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 3);
        s_mock_tick = 3000; aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 3);   /* max 3 */
        queue_and_process(AIIO_CI13162_CMD_VOICE_GET, 2U);
        assert(aiio_ci13162_uart_get_voice_id() == 2U);

        /* stops once the cache is valid */
        reset_test_state();
        queue_and_process_version(261102U);
        aiio_ci13162_uart_poll();
        queue_and_process(AIIO_CI13162_CMD_VOICE_GET, 3U);
        base = s_uart_tx_count;
        s_mock_tick = 1000; aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base);

        /* paused (CWSL session) holds the query without spending attempts */
        reset_test_state();
        aiio_ci13162_uart_set_version_query_paused(1U);
        queue_and_process_version(261102U);
        base = s_uart_tx_count;
        aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base);
        aiio_ci13162_uart_set_version_query_paused(0U);
        aiio_ci13162_uart_poll(); assert(s_uart_tx_count == base + 1);

        /* no version reply -> no voice query */
        reset_test_state();
        base = s_uart_tx_count;
        s_mock_tick = 5000; aiio_ci13162_uart_poll();
        assert(s_uart_tx_count == base);
        assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":18,\"data\":[]}\r\n") != 0);
        printf("  PASS: boot voice query verified\n");
    }

    /* TEST 20: volume muted state + mute send */
    {
        printf("--- Test 20: volume muted state + mute send ---\n");
        reset_test_state();
        assert(aiio_ci13162_uart_send_mute() == 0);
        assert(strcmp((char *)s_uart_tx_buf, "{\"cmd\":8,\"data\":[8]}\r\n") == 0);
        s_aiio_ci13162_ota_held = 1U; assert(aiio_ci13162_uart_send_mute() == -1); s_aiio_ci13162_ota_held = 0U;
        uint8_t lv = 9U, mu = 9U;
        s_aiio_ci13162_volume_valid = 0U;
        assert(aiio_ci13162_uart_get_volume_state(&lv, &mu) == -1);
        assert(aiio_ci13162_uart_get_volume_state(NULL, &mu) == -1);
        aiio_ci13162_uart_set_volume_callback(test_volume_cb);
        queue_and_process_volume_info(3U, 1U, 1U);
        assert(s_volume_cb_count == 1 && s_volume_cb_level == 3U && s_volume_cb_result == 1U && s_volume_cb_muted == 1U);
        assert(aiio_ci13162_uart_get_volume_state(&lv, &mu) == 0 && lv == 3U && mu == 1U);
        queue_and_process_volume_change(4U, 49U, 255U);
        assert(aiio_ci13162_uart_get_volume_state(&lv, &mu) == 0 && lv == 4U && mu == 0U);
        queue_and_process_volume_change(0U, 54U, 255U);
        assert(aiio_ci13162_uart_get_volume_state(&lv, &mu) == 0 && lv == 0U && mu == 1U);
        aiio_ci13162_uart_set_volume_callback(NULL);
        printf("  PASS: volume muted state + mute send verified\n");
    }

    /* TEST 21: Home command 71..103 (cmd 12) */
    {
        printf("--- Test 21: Home command 71..103 (cmd 12) ---\n");
        aiio_ci13162_message_t msg;
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[71],\"cmd_id\":71,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[102],\"cmd_id\":102,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(msg.command == AIIO_CI13162_CMD_SCENE && msg.data[0] == 102U && msg.cmd_id == 102U && msg.cwsl == 255U);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[103],\"cmd_id\":103,\"cwsl\":3}", &msg) == AIIO_CI13162_PARSE_OK);
        assert(msg.cmd_id == 103U && msg.cwsl == 3U);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[104],\"cmd_id\":104,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[70],\"cmd_id\":70,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[16],\"cmd_id\":16,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[0],\"cmd_id\":0,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[102],\"cmd_id\":103,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[102],\"cmd_id\":157,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_INVALID_DATA);
        assert(aiio_ci13162_parse_line("{\"cmd\":12,\"data\":[15],\"cmd_id\":70,\"cwsl\":255}", &msg) == AIIO_CI13162_PARSE_OK);

        /* 21.2: valid home command line reaches app_cwsl_notify_voice_cmd on VOICEBOX, no 299 */
        uint8_t saved_device_type = device_type;
        reset_test_state();
        device_type = VOICEBOX;
        char home_line[] = "{\"cmd\":12,\"data\":[102],\"cmd_id\":102,\"cwsl\":255}";
        aiio_ci13162_handle_line(home_line, strlen(home_line));
        aiio_ci13162_uart_process();
        assert(s_app_send_data_count == 0);
        assert(s_cwsl_notify_count == 1);
        assert(s_last_cwsl_notify_cmd_id == 102U);
        assert(s_last_cwsl_notify_cwsl == 255U);

        /* 21.3: home command above 103 is still rejected via MQTT 299 */
        reset_test_state();
        char bad_home_line[] = "{\"cmd\":12,\"data\":[104],\"cmd_id\":104,\"cwsl\":255}";
        aiio_ci13162_handle_line(bad_home_line, strlen(bad_home_line));
        aiio_ci13162_uart_process();
        assert(s_app_send_data_count == 1);
        assert(strstr(s_last_app_send_data, "\"voiceboxv1\":299") != NULL);
        assert(strstr(s_last_app_send_data, "\"reason\":\"INVALID_DATA\"") != NULL);
        assert(s_cwsl_notify_count == 0);
        device_type = saved_device_type;
        printf("  PASS: Home command 71..103 parser and dispatch verified\n");
    }

    printf("\nALL 21 PROTOCOL & HARDWARE TESTS PASSED 100%%!\n");
    return 0;
}
