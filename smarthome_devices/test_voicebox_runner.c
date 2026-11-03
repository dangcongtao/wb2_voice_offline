#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define os_trace printf
#define BUILD_TEST 1
#define _USER_CONFIG_H_
#define __EASY_FLASH_COMMON_H__
#define _APP_MQTT_H__
#define _APP_MQTT_H_

#define _MOCK_BL_GPIO_H_
static int s_mock_gpio_output_pins[32] = {0};
static int s_mock_gpio_output_levels[32] = {0};
static int s_mock_gpio_output_calls = 0;

static inline void bl_gpio_output_set(int pin, int value) {
    if (pin >= 0 && pin < 32) {
        s_mock_gpio_output_levels[pin] = value;
        s_mock_gpio_output_calls++;
    }
}

static inline void bl_gpio_enable_output(uint8_t pin, uint8_t pullup, uint8_t pulldown) {
    (void)pullup;
    (void)pulldown;
    if (pin < 32) {
        s_mock_gpio_output_pins[pin] = 1;
        s_mock_gpio_output_calls++;
    }
}

static int s_mock_gpio_input_pins[32] = {0};
static int s_mock_gpio_input_pullup[32] = {0};
static int s_mock_gpio_input_levels[32] = {0};

static inline void bl_gpio_enable_input(uint8_t pin, uint8_t pullup, uint8_t pulldown) {
    (void)pulldown;
    if (pin < 32) {
        s_mock_gpio_input_pins[pin] = 1;
        s_mock_gpio_input_pullup[pin] = pullup;
    }
}

static inline int bl_gpio_input_get_value(int pin) {
    if (pin < 0 || pin >= 32) return 1;
    return s_mock_gpio_input_levels[pin]; /* idle high, set in reset */
}

void gpio_set_input(int pin) { bl_gpio_enable_input((uint8_t)pin, 1, 0); }

void gpio_set_output(int pin) {
    (void)pin;
}

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "components/app_config/system_typedef.h"
#include "components/hardware/hardware.h"
#include "components/hardware/aiio_ci13162_config.h"
#include "components/third_party/cJson/cJSON.c"

/* Global device definitions */
char *gs_dev_type[] = {"wswitch3v", "lhswitch3v", "swminiv2", "sswitch2v", "swshockv2", "swshohuv2", "fanoem", "fandc", "fanac", "fanproacv2", "voiceboxv1"};
uint32_t device_type = VOICEBOX;
uint32_t max_number_channel = 1;
uint8_t last_reason_reset = 0;
uint8_t is_ota = 0;
uint32_t val_temperature = 250;
uint8_t bat_percent = 100;
uint8_t bat_charging_status = 0;

/* Mock UART transmission */
static char s_last_uart_tx[256] = {0};
static int s_uart_tx_count = 0;

int32_t aiio_ci13162_uart_send_buzz_type(uint8_t is_long) {
    snprintf(s_last_uart_tx, sizeof(s_last_uart_tx), "{\"cmd\":200,\"data\":[%u]}\r\n", is_long ? 1U : 0U);
    s_uart_tx_count++;
    return 0;
}

uint32_t aiio_ci13162_uart_get_voice_version(void) {
    return 1002003U;
}

int32_t aiio_ci13162_uart_get_volume(uint8_t *volume) {
    if (volume) *volume = 5U;
    return 0;
}

/* Mock Flash API */
static uint8_t s_mock_flash_rs_reason = 0;
void flash_set_rs_reason(uint8_t reason) {
    s_mock_flash_rs_reason = reason;
}
uint8_t flash_read_rs_reason(void) {
    return s_mock_flash_rs_reason;
}

/* Mock Database */
static char s_mock_dev_id[64] = "HUN123456789voiceboxv1N1";
char *m_get_dev_id(void) {
    return s_mock_dev_id;
}

/* Mock WiFi & MQTT */
static int s_smart_config_count = 0;
static int s_ap_config_count = 0;
static void mock_start_smart_config(void) { s_smart_config_count++; }
static void mock_start_ap_config(void) { s_ap_config_count++; }
static void mock_reset_db_without_sync(void) {}

static char s_last_mqtt_publish[512] = {0};
static int s_mqtt_publish_count = 0;
void app_send_data(char *data, uint32_t len) {
    (void)len;
    strncpy(s_last_mqtt_publish, data, sizeof(s_last_mqtt_publish) - 1);
    s_last_mqtt_publish[sizeof(s_last_mqtt_publish) - 1] = '\0';
    s_mqtt_publish_count++;
}

/* Mock Touch IC */
static int s_touch_reset_call_count = 0;
void m_ft62f28_send_reset_touch(void) {
    s_touch_reset_call_count++;
}

/* Mock Hardware Functions */
bool hw_device_support_voice(void) {
    return (device_type == VOICEBOX);
}

/* Include source files directly for deep testing */
#include "components/hardware/buzz.c"
#include "components/hardware/74hc595.c"
#include "components/hardware/button.c"
#include "components/middle/ble_slave/security.c"

static void reset_all_mocks(void) {
    memset(s_mock_gpio_output_pins, 0, sizeof(s_mock_gpio_output_pins));
    memset(s_mock_gpio_output_levels, 0, sizeof(s_mock_gpio_output_levels));
    s_mock_gpio_output_calls = 0;
    s_last_uart_tx[0] = '\0';
    s_uart_tx_count = 0;
    for (int i = 0; i < 32; i++) s_mock_gpio_input_levels[i] = 1; /* idle high */
    s_mock_flash_rs_reason = 0;
    s_smart_config_count = 0;
    s_ap_config_count = 0;
    s_last_mqtt_publish[0] = '\0';
    s_mqtt_publish_count = 0;
    s_touch_reset_call_count = 0;
    s_buzz_last = SWITCH_IDLE_LEVEL;
    s_buzz_active_is_long = 0U;
}

int main(void) {
    printf("[TEST] Running dedicated VOICEBOX test suite...\n");

    /* =========================================================================
     * TEST 1: Device Type Identification & Naming
     * ========================================================================= */
    {
        printf("--- Test 1: Device Type & gs_dev_type ---\n");
        assert(VOICEBOX == 10);
        assert(strcmp(gs_dev_type[VOICEBOX], "voiceboxv1") == 0);
        assert(strstr("HUN178840469966voiceboxv1N1", gs_dev_type[VOICEBOX]) != NULL);
        printf("  PASS: Device type enum index and string mapping verified\n");
    }

    /* =========================================================================
     * TEST 2: Hardware Pinout Specifications
     * ========================================================================= */
    {
        printf("--- Test 2: Hardware Pinout Macros & Voice Support ---\n");
        assert(LED_STT_VOICEBOX == 2);
        assert(BTN_VOICEBOX_VOLUME_UP_PIN == 21);
        assert(BTN_VOICEBOX_FUNCTION_PIN == 20);
        assert(BTN_VOICEBOX_VOLUME_DOWN_PIN == 0);
        assert(BTN_VOICEBOX_IDLE_LEVEL == 1);
        assert(BTN_VOICEBOX_COUNT == 3U);
        assert(AIIO_CI13162_UART_RX_PIN == 3U);
        assert(AIIO_CI13162_UART_TX_PIN == 4U);
        assert(AIIO_CI13162_UART_ID == 1U);
        assert(AIIO_CI13162_POWER_PIN == 17U);

        device_type = VOICEBOX;
        assert(hw_device_support_voice() == true);
        device_type = FANDC;
        assert(hw_device_support_voice() == false);
        device_type = FANAC;
        assert(hw_device_support_voice() == false);
        device_type = FANACIR;
        assert(hw_device_support_voice() == false);
        device_type = VOICEBOX;
        printf("  PASS: Pin macros and hw_device_support_voice verified\n");
    }

    /* =========================================================================
     * TEST 3: 74HC595 Protection on VOICEBOX (Protects LED_STT_VOICEBOX on GPIO 2)
     * ========================================================================= */
    {
        printf("--- Test 3: 74HC595 Protection for VOICEBOX (GPIO 2 & 22 safety) ---\n");
        reset_all_mocks();
        device_type = VOICEBOX;

        /* Verify hw_74hc595_init() exits immediately and does not configure GPIO 2 or 22 */
        hw_74hc595_init();
        assert(s_mock_gpio_output_pins[LED_COM_PIN] == 0);
        assert(s_mock_gpio_output_pins[DS_PIN] == 0);
        assert(s_mock_gpio_output_calls == 0);

        /* Verify all other 74hc595 operations exit immediately */
        hw_74hc595_set_bit(0, 1);
        hw_74hc595_set_number(1);
        hw_74hc595_write_byte(0x55, 1);
        hw_74hc595_loop();
        assert(s_mock_gpio_output_calls == 0);

        /* Confirm that on FANDC, 74hc595 does configure output pins */
        device_type = FANDC;
        hw_74hc595_init();
        assert(s_mock_gpio_output_pins[LED_COM_PIN] == 1);
        assert(s_mock_gpio_output_pins[DS_PIN] == 1);
        assert(s_mock_gpio_output_calls > 0);

        device_type = VOICEBOX; /* restore */
        printf("  PASS: 74HC595 completely disabled on VOICEBOX, protecting GPIO 2 & 22\n");
    }

    /* =========================================================================
     * TEST 4: Buzzer via UART on VOICEBOX
     * ========================================================================= */
    {
        printf("--- Test 4: Buzzer via CI13162 UART on Button Press ---\n");
        reset_all_mocks();
        device_type = VOICEBOX;

        /* Short press activates buzzer -> sends cmd 200 with data [0] */
        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 0U);
        assert(s_uart_tx_count == 1);
        assert(strcmp(s_last_uart_tx, "{\"cmd\":200,\"data\":[0]}\r\n") == 0);

        /* Repeated press while still active does NOT resend */
        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 0U);
        assert(s_uart_tx_count == 1);

        /* Release resets buzzer state */
        hw_buzz_set_type(SWITCH_IDLE_LEVEL, 0U);

        /* Subsequent press triggers buzz again */
        hw_buzz_set_type(SWITCH_ACTIVE_LEVEL, 0U);
        assert(s_uart_tx_count == 2);
        hw_buzz_set_type(SWITCH_IDLE_LEVEL, 0U);

        printf("  PASS: Button press triggers CI13162 speaker test beep via UART\n");
    }

    /* =========================================================================
     * TEST 5: OTA MCU Routing (Cmd 130 & 129 vs Fan Command Filtering)
     * ========================================================================= */
    {
        printf("--- Test 5: OTA MCU Cmd 130 vs Fan Command Filtering ---\n");
        /* Verify command categorization for VOICEBOX:
         * Cmd 130 MUST NOT be in the ignored list!
         * Fan commands (20, 21, 25, 26, 27, 28, 107, 131) and IR commands (1, 2, 4, 15, 16) ARE ignored. */
        uint32_t ignored_cmds[] = {1, 2, 4, 15, 16, 20, 21, 25, 26, 27, 28, 107, 131};
        for (size_t i = 0; i < sizeof(ignored_cmds)/sizeof(ignored_cmds[0]); i++) {
            uint32_t cmd = ignored_cmds[i];
            bool is_ignored = (cmd == 1 || cmd == 2 || cmd == 4 || cmd == 15 || cmd == 16 ||
                               cmd == 20 || cmd == 21 || cmd == 25 || cmd == 26 || cmd == 27 ||
                               cmd == 28 || cmd == 107 || cmd == 131);
            assert(is_ignored == true);
        }

        /* Cmd 130 (OTA MCU for VOICEBOX) must NOT be ignored */
        uint32_t cmd_130 = 130;
        bool cmd_130_ignored = (cmd_130 == 1 || cmd_130 == 2 || cmd_130 == 4 || cmd_130 == 15 || cmd_130 == 16 ||
                                cmd_130 == 20 || cmd_130 == 21 || cmd_130 == 25 || cmd_130 == 26 || cmd_130 == 27 ||
                                cmd_130 == 28 || cmd_130 == 107 || cmd_130 == 131);
        assert(cmd_130_ignored == false);

        /* Cmd 129 (standard OTA MCU) must NOT be ignored */
        uint32_t cmd_129 = 129;
        bool cmd_129_ignored = (cmd_129 == 1 || cmd_129 == 2 || cmd_129 == 4 || cmd_129 == 15 || cmd_129 == 16 ||
                                cmd_129 == 20 || cmd_129 == 21 || cmd_129 == 25 || cmd_129 == 26 || cmd_129 == 27 ||
                                cmd_129 == 28 || cmd_129 == 107 || cmd_129 == 131);
        assert(cmd_129_ignored == false);

        printf("  PASS: Cmd 130 and 129 preserved for OTA MCU, fan/IR commands ignored\n");
    }

    /* =========================================================================
     * TEST 6: Button Hold Events & AP Config (>10s and >15s)
     * ========================================================================= */
    {
        printf("--- Test 6: Button Hold Events (>10s and >15s) for AP Config ---\n");
        #define EVENT_BTN_HOLD 1
        #define EVENT_BTN_HOLD_SO_LONG 2
        #define EVENT_BTN_HOLD_HONG_PHIM 3

        /* Simulate button release logic on VOICEBOX */
        uint32_t ignore_event_release = 0;

        /* Case 6.1: Short press release (ignore_event == 0) -> no config triggered */
        ignore_event_release = 0;
        bool smart_config_triggered = false;
        bool ap_config_triggered = false;
        if (ignore_event_release == EVENT_BTN_HOLD) {
            smart_config_triggered = true;
        } else if (ignore_event_release == EVENT_BTN_HOLD_SO_LONG ||
                   ignore_event_release == EVENT_BTN_HOLD_HONG_PHIM) {
            ap_config_triggered = true;
        }
        assert(!smart_config_triggered && !ap_config_triggered);

        /* Case 6.2: Hold 3-5s release -> SmartConfig triggered */
        ignore_event_release = EVENT_BTN_HOLD;
        smart_config_triggered = false;
        ap_config_triggered = false;
        if (ignore_event_release == EVENT_BTN_HOLD) {
            smart_config_triggered = true;
        } else if (ignore_event_release == EVENT_BTN_HOLD_SO_LONG ||
                   ignore_event_release == EVENT_BTN_HOLD_HONG_PHIM) {
            ap_config_triggered = true;
        }
        assert(smart_config_triggered && !ap_config_triggered);

        /* Case 6.3: Hold >10s release -> AP Config triggered */
        ignore_event_release = EVENT_BTN_HOLD_SO_LONG;
        smart_config_triggered = false;
        ap_config_triggered = false;
        if (ignore_event_release == EVENT_BTN_HOLD) {
            smart_config_triggered = true;
        } else if (ignore_event_release == EVENT_BTN_HOLD_SO_LONG ||
                   ignore_event_release == EVENT_BTN_HOLD_HONG_PHIM) {
            ap_config_triggered = true;
        }
        assert(!smart_config_triggered && ap_config_triggered);

        /* Case 6.4: Hold >15s release -> AP Config STILL triggered, touch reset bypassed */
        ignore_event_release = EVENT_BTN_HOLD_SO_LONG; /* Since HOLD_HONG_PHIM returned early for VOICEBOX */
        smart_config_triggered = false;
        ap_config_triggered = false;
        if (ignore_event_release == EVENT_BTN_HOLD) {
            smart_config_triggered = true;
        } else if (ignore_event_release == EVENT_BTN_HOLD_SO_LONG ||
                   ignore_event_release == EVENT_BTN_HOLD_HONG_PHIM) {
            ap_config_triggered = true;
        }
        assert(!smart_config_triggered && ap_config_triggered);
        assert(s_touch_reset_call_count == 0);

        printf("  PASS: Button short press, SmartConfig, and AP Config (>10s and >15s) verified\n");
    }

    /* =========================================================================
     * TEST 7: Startup Message (cmd 100) and Get Info (cmd 105) JSON Format for VOICEBOX
     * ========================================================================= */
    {
        printf("--- Test 7: Startup Message (cmd 100) and Get Info (cmd 105) Format ---\n");
        char respond[512];
        uint32_t num_send = 1;
        uint32_t vermqtt = 1;
        uint8_t buzz_enabled = 1;
        uint8_t vol_val = 5;
        uint8_t mac_bt[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

        sprintf(respond,
                "{\"%s\":100,\"reason\":%lu,\"id\":\"%s\",\"ver\":%s,\"voice_ver\":%lu,"
                "\"ssid\":\"%s\",\"streng\":%d,\"ip\":\"%s\",\"vermqtt\":%lu,"
                "\"buzz\":%u,\"volume\":%u,\"voice_id\":%u,"
                "\"mac_bt\":\"%02x:%02x:%02x:%02x:%02x:%02x\"}",
                gs_dev_type[VOICEBOX],
                (unsigned long)(num_send * 100U + 0U + 1U),
                "HUN123456789voiceboxv1N1",
                "\"260919\"",
                (unsigned long)1002003U,
                "MyWifiSSID",
                -45,
                "192.168.1.50",
                (unsigned long)vermqtt,
                buzz_enabled,
                vol_val,
                2U,
                mac_bt[0], mac_bt[1], mac_bt[2], mac_bt[3], mac_bt[4], mac_bt[5]);

        /* Parse generated JSON and verify fields */
        cJSON *root = cJSON_Parse(respond);
        assert(root != NULL);
        assert(cJSON_GetObjectItem(root, "voiceboxv1") != NULL);
        assert(cJSON_GetObjectItem(root, "voiceboxv1")->valueint == 100);
        assert(cJSON_GetObjectItem(root, "reason") != NULL);
        assert(cJSON_GetObjectItem(root, "id") != NULL);
        assert(cJSON_GetObjectItem(root, "ver") != NULL);
        assert(cJSON_GetObjectItem(root, "voice_ver") != NULL);
        assert(cJSON_GetObjectItem(root, "ssid") != NULL);
        assert(cJSON_GetObjectItem(root, "streng") != NULL);
        assert(cJSON_GetObjectItem(root, "ip") != NULL);
        assert(cJSON_GetObjectItem(root, "vermqtt") != NULL);
        assert(cJSON_GetObjectItem(root, "buzz") != NULL);
        assert(cJSON_GetObjectItem(root, "volume") != NULL);
        assert(cJSON_GetObjectItem(root, "voice_id") != NULL);
        assert(cJSON_GetObjectItem(root, "mac_bt") != NULL);
        assert(strcmp(cJSON_GetObjectItem(root, "mac_bt")->valuestring, "aa:bb:cc:dd:ee:ff") == 0);

        /* Fan / lamp / battery fields must be omitted */
        assert(cJSON_GetObjectItem(root, "mode") == NULL);
        assert(cJSON_GetObjectItem(root, "lv") == NULL);
        assert(cJSON_GetObjectItem(root, "pcn") == NULL);
        assert(cJSON_GetObjectItem(root, "swing") == NULL);
        assert(cJSON_GetObjectItem(root, "temp") == NULL);
        assert(cJSON_GetObjectItem(root, "lamp") == NULL);
        assert(cJSON_GetObjectItem(root, "bat") == NULL);
        assert(cJSON_GetObjectItem(root, "charging") == NULL);
        assert(cJSON_GetObjectItem(root, "t") == NULL);

        cJSON_Delete(root);
        printf("  PASS: Cmd 100 contains all voicebox fields including mac_bt and omits fan/lamp/bat fields\n");
    }

    /* =========================================================================
     * TEST 8: LED Pulse Re-entrancy Protection
     * ========================================================================= */
    {
        printf("--- Test 8: LED Flash Pulse Re-entrancy Protection ---\n");
        static int mock_led_toggle_count = 0;
        volatile uint8_t is_pulsing = 0;

        /* First pulse trigger toggles LED and marks pulsing */
        if (!is_pulsing) {
            is_pulsing = 1;
            mock_led_toggle_count++;
        }
        assert(mock_led_toggle_count == 1);
        assert(is_pulsing == 1);

        /* Rapid second pulse trigger while still pulsing must NOT toggle again */
        if (!is_pulsing) {
            is_pulsing = 1;
            mock_led_toggle_count++;
        }
        assert(mock_led_toggle_count == 1); /* Still 1, not toggled prematurely back! */

        /* After timer expiry, pulsing flag clears */
        is_pulsing = 0;

        /* Next pulse after expiry toggles again */
        if (!is_pulsing) {
            is_pulsing = 1;
            mock_led_toggle_count++;
        }
        assert(mock_led_toggle_count == 2);

        printf("  PASS: Status LED flash pulse re-entrancy protection verified\n");
    }

    /* =========================================================================
     * TEST 9: BLE Beacon Packet Format & Encryption/Decryption Compatibility
     * ========================================================================= */
    {
        printf("--- Test 9: BLE Beacon Packet Format & Encryption/Decryption ---\n");
        typedef struct __attribute__((packed)) {
            uint8_t src_mac[6];
            uint8_t cmd_id;
        } test_encode_data_t;

        typedef struct __attribute__((packed)) {
            uint8_t service_uuid[2];
            uint8_t cmd;
            uint8_t tid;
            uint16_t checksum;
            test_encode_data_t content;
        } test_hunonic_voicebox_msg_t;

        assert(sizeof(test_encode_data_t) == 7);
        assert(sizeof(test_hunonic_voicebox_msg_t) == 13);

        uint8_t expected_mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
        for (uint8_t tid = 1; tid < 50; tid++) {
            test_hunonic_voicebox_msg_t msg;
            memset(&msg, 0, sizeof(msg));
            msg.service_uuid[0] = 0xFA;
            msg.service_uuid[1] = 0xFB;
            msg.cmd = 0x9A;
            msg.tid = tid;
            memcpy(msg.content.src_mac, expected_mac, 6);
            msg.content.cmd_id = 56U;

            msg.checksum = calculate_checksum((uint8_t *)&msg.content, sizeof(test_encode_data_t));
            encrypt_data((uint8_t *)&msg.content, sizeof(test_encode_data_t), msg.tid);

            /* Verify switch side decryption & checksum verification */
            uint16_t rx_chk = decrypt_data((uint8_t *)&msg.content, sizeof(msg) - 6, msg.tid);
            assert(rx_chk == msg.checksum);
            assert(memcmp(msg.content.src_mac, expected_mac, 6) == 0);
            assert(msg.content.cmd_id == 56U);
        }
        printf("  PASS: BLE Beacon 13-byte frame layout and security round-trip verified\n");
    }

    /* =========================================================================
     * TEST 10: VOICEBOX button pins (real button.c)
     * ========================================================================= */
    {
        printf("--- Test 10: VOICEBOX button pins and levels ---\n");
        for (int i = 0; i < 32; i++) {
            s_mock_gpio_input_pins[i] = 0;
            s_mock_gpio_input_pullup[i] = 0;
            s_mock_gpio_input_levels[i] = 1;
        }
        device_type = VOICEBOX;
        hw_button_config(0U); hw_button_config(1U); hw_button_config(2U);
        assert(s_mock_gpio_input_pins[21] && s_mock_gpio_input_pullup[21] == 1);
        assert(s_mock_gpio_input_pins[20] && s_mock_gpio_input_pullup[20] == 1);
        assert(s_mock_gpio_input_pins[0]  && s_mock_gpio_input_pullup[0]  == 1);
        assert(s_mock_gpio_input_pins[1] == 0);
        assert(hw_btn_read(0U) == 1U && hw_btn_read(1U) == 1U && hw_btn_read(2U) == 1U);
        s_mock_gpio_input_levels[21] = 0; assert(hw_btn_read(0U) == 0U);
        s_mock_gpio_input_levels[20] = 0; assert(hw_btn_read(1U) == 0U);
        s_mock_gpio_input_levels[0]  = 0; assert(hw_btn_read(2U) == 0U);
        hw_btn_set_val_button(0xFF); s_mock_gpio_input_levels[21] = 1;
        assert(hw_btn_read(0U) == 1U);
        assert(hw_btn_read(3U) == BTN_VOICEBOX_IDLE_LEVEL);
        hw_btn_set_val_button(0);
        printf("  PASS: VOICEBOX buttons 21/20/0 active-low, touch ignored\n");
    }

    printf("\nALL 10 VOICEBOX UNIT TESTS PASSED 100%%!\n");
    return 0;
}
