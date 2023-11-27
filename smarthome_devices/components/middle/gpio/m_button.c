#include "m_button.h"
#include "system_typedef.h"
#include "app_btn.h"
#include "hardware.h"

#include <stdint.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>

#include "m_switch.h"
#include "m_led_stt.h"
#include "m_wifi.h"
#include "m_database.h"
#include "event_driven.h"

#define TIME_OUT_RESET_TS04_WITHOUT_PRESS   2400

extern m_state_t current_state;;
extern uint32_t max_number_channel;
extern uint8_t is_ota;
static m_button_t s_button_obj;
static uint32_t time_cnt_down_reset_TS04 = TIME_OUT_RESET_TS04_WITHOUT_PRESS;
uint32_t ignore_event_release[MAX_NUM_BUTTON] = {0, 0, 0, 0};

#define EVENT_BTN_HOLD              1
#define EVENT_BTN_HOLD_SO_LONG      2
#define EVENT_BTN_HOLD_HONG_PHIM    3

static const char *event_name[] = {"pressed", "release",
                                   "hold", "hold so long", "hold hong phim",
                                   "double click", "tripple click",
                                   "idle", "idle break", "invalid"};

extern void app_send_offline_message(uint8_t src);
extern int is_wifi_connected(void);

void on_button_event_cb(int button_pin, int event, void *data)
{
    os_trace("Button %d, event %s\r\n", button_pin, event_name[event]);

    if (device_type == SWMINI && button_pin == 2) {

    } else if (m_db_get_instance()->_m_device._switch_info->channel_lock_state[button_pin] == 1) {
        if (dev_is_online()) {
            os_trace("channel %d is locked!\r\n", button_pin);
            return;
        }
    }

    switch (event)
    {
    case APP_BTN_EVT_PRESSED:
    {
        if (device_type == SWMINI && button_pin < 2) {
            m_switch_get_instance()._toggle_switch_cb(button_pin, 1, true, true);
        }
    }
    break;

    case APP_BTN_EVT_RELEASED:
    {
        time_cnt_down_reset_TS04 = TIME_OUT_RESET_TS04_WITHOUT_PRESS;
        if (ignore_event_release[button_pin] == EVENT_BTN_HOLD_HONG_PHIM)
        {
            sync_led_stt_when_reset_ts04();
            ignore_event_release[button_pin] = 0;
            return;
        }
        else if (ignore_event_release[button_pin] == EVENT_BTN_HOLD)
        {
            app_send_offline_message(1);
            vTaskDelay(pdMS_TO_TICKS(100));

            m_db_get_instance()->_reset_db_without_sync_cb();
            m_wifi_get_instance().start_smart_config_cb();
            ignore_event_release[button_pin] = 0;
            return;
        }

        else if (ignore_event_release[button_pin] == EVENT_BTN_HOLD_SO_LONG)
        {
            app_send_offline_message(1);
            vTaskDelay(pdMS_TO_TICKS(100));
            
            m_db_get_instance()->_reset_db_without_sync_cb();
            m_wifi_get_instance().start_ap_config_cb();
            ignore_event_release[button_pin] = 0;
            return;
        }

        /* just support buttons, not support sensor */
        if (device_type != SWMINI || (device_type == SWMINI && button_pin < 2)) {
            m_switch_get_instance()._toggle_switch_cb(button_pin, 1, true, true);
        }
        ignore_event_release[button_pin] = 0;
    }
    break;

    case APP_BTN_EVT_HOLD:
    {
        if (device_type != SWMINI || (device_type == SWMINI && button_pin == 2)) {
            m_led_stt_get_instance()._set_led_stt_smart_config_cb();
            ignore_event_release[button_pin] = EVENT_BTN_HOLD;
        }
    }
    break;

    case APP_BTN_EVT_HOLD_SO_LONG:
    {
        if (device_type != SWMINI || (device_type == SWMINI && button_pin == 2)) {
            m_led_stt_get_instance()._set_led_stt_ap_config_cb();
            ignore_event_release[button_pin] = EVENT_BTN_HOLD_SO_LONG;
        }
    }
    break;

    case APP_BTN_EVT_HOLD_HONG_PHIM:
    {
        if (ignore_event_release[button_pin] == EVENT_BTN_HOLD_HONG_PHIM)
            return;
        hw_ts04_write(TS04_IDLE_LEVEL);

        vTaskDelay(pdMS_TO_TICKS(100));

        hw_ts04_write(TS04_ACTIVE_LEVEL);
        ignore_event_release[button_pin] = EVENT_BTN_HOLD_HONG_PHIM;
    }

    case APP_BTN_EVT_DOUBLE_CLICK:
    {
        /* double click */
    }
    break;

    case APP_BTN_EVT_TRIPLE_CLICK:
    {
        /* triplle click */
    }
    break;

    default:
        break;
    }
}

static void m_button_config(void)
{
    static app_btn_hw_config_t m_hw_button_initialize_value[MAX_NUM_BUTTON];
    for (uint32_t i = 0; i < max_number_channel; i++)
    {
        m_hw_button_initialize_value[i].idle_level = BTN_IDLE_LEVEL;
        m_hw_button_initialize_value[i].last_state = BTN_IDLE_LEVEL;
        m_hw_button_initialize_value[i].pin = i;
    }

    app_btn_config_t conf;
    conf.config = &m_hw_button_initialize_value[0];
    conf.btn_count = max_number_channel;
    conf.get_tick_cb = sys_get_tick_ms;
    conf.btn_initialize = hw_button_config;
    conf.btn_read = hw_btn_read;

    if (device_type == SWMINI) {
        conf.btn_count = max_number_channel + 1;
        m_hw_button_initialize_value[2].idle_level = BTN_IDLE_LEVEL;
        m_hw_button_initialize_value[2].last_state = BTN_IDLE_LEVEL;
        m_hw_button_initialize_value[2].pin = 2;
    }

    app_btn_initialize(&conf);
    app_btn_register_callback(APP_BTN_EVT_PRESSED, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_RELEASED, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_HOLD, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_HOLD_SO_LONG, on_button_event_cb, NULL);
    if (device_type == WSWITCH || is_shock_device(device_type)) {
        app_btn_register_callback(APP_BTN_EVT_HOLD_HONG_PHIM, on_button_event_cb, NULL);
    }
}

/* private */
static void button_scan_task(void *param)
{
    static uint32_t tick_ota_led = 0;
    os_trace("button_scan_task\n");
    while (true)
    {
#if PROJECT_SHOCK
    if (is_shock_device(device_type)) {
        hw_read_leak_loop();
    }
        if (!is_leakage_current())
{
#endif
        m_switch_press_release_loop();
        m_switch_cyclic_timer_loop();
#if PROJECT_SHOCK
}
#endif
        app_btn_scan(NULL);

        if (device_type == WSWITCH || is_shock_device(device_type)) {
            if (time_cnt_down_reset_TS04 == 0) {
                uint8_t s_check = 0;
                for (uint8_t i = 0; i < max_number_channel; i++)
                {
                    s_check += ignore_event_release[i];
                }
                if (s_check == 0) {
                    os_trace("reset ts04 every 2 minutes\r\n");
                    hw_ts04_write(TS04_IDLE_LEVEL);

                    vTaskDelay(pdMS_TO_TICKS(100));

                    hw_ts04_write(TS04_ACTIVE_LEVEL);
                }
                time_cnt_down_reset_TS04 = TIME_OUT_RESET_TS04_WITHOUT_PRESS;
                os_trace("reset time count down ts04 reset\r\n");
            } else{
                time_cnt_down_reset_TS04--;
            }
        }

        blink_switch_config_loop();

        if (!dev_is_doing_config()) {
            if (is_ota == STATE_OTA_PROCESS) {
                if (tick_ota_led % 500 == 0) {
                    tick_ota_led = 0;
                    m_led_stt_get_instance()._toggle_led_stt_cb();
                }
                tick_ota_led += 50;
            } 
            else if (is_ota == STATE_OTA_SUCCESS)
            {
                if (tick_ota_led >= 100) {
                    tick_ota_led = 0;
                    m_led_stt_get_instance()._toggle_led_stt_cb();
                }
                tick_ota_led += 50;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* public */
void m_button_init(void)
{
    hw_btn_change_btn_pin();
    m_button_config();
    
    os_trace("m_button_init: %ld\r\n", max_number_channel);
    xTaskCreate(button_scan_task,  /* Function that implements the task. */
                "led_button_task", /* Text name for the task. */
                1024 * 3,          /* Stack size in words, not bytes. */
                NULL,              /* Parameter passed into the task. */
                4,                 /* Priority at which the task is created. */
                NULL);
}

m_button_t m_button_get_instace(void)
{
    return s_button_obj;
}