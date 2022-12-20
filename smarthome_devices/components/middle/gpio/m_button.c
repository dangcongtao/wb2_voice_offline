#include "m_button.h"
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

static m_button_t s_button_obj;
uint32_t ignore_event_release[MAX_NUM_BUTTON] = {0};

static const char *event_name[] = {"pressed", "release",
                                   "hold", "hold so long",
                                   "double click", "tripple click",
                                   "idle", "idle break", "invalid"};
void on_button_event_cb(int button_pin, int event, void *data)
{
    os_trace("Button %d, event %s\r\n", button_pin, event_name[event]);

    // if (m_db_get_instance()->_m_device._switch_info.channel_lock_state[button_pin] == 1)
    //     if (m_wifi_get_instance().wifi_get_state_cb() == WIFI_DEV_ACTIVE)
    //         return;

    switch (event)
    {
    case APP_BTN_EVT_PRESSED:
    {
    }
    break;

    case APP_BTN_EVT_RELEASED:
    {
        if (ignore_event_release[button_pin])
        {
            ignore_event_release[button_pin] = 0;
            return;
        }
        /* just support buttons, not support sensor */
        m_switch_get_instance()._toggle_switch_cb(button_pin, 1, true, true);
    }
    break;

    case APP_BTN_EVT_HOLD:
    {
        /* factory reset database */
        m_led_stt_get_instance()._set_led_stt_smart_config_cb();
        // m_wifi_get_instance().start_smart_config_cb();
        // m_db_get_instance()->_reset_db_cb();
        ignore_event_release[button_pin] = 1;
    }
    break;

    case APP_BTN_EVT_HOLD_SO_LONG:
    {
        if (ignore_event_release[button_pin] == 2)
            return;

        /* factory reset database */
        m_led_stt_get_instance()._set_led_stt_ap_config_cb();
        // m_wifi_get_instance().start_ap_config_cb();
        ignore_event_release[button_pin] = 2;
    }
    break;

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
    for (uint32_t i = 0; i < MAX_NUM_BUTTON; i++)
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

    app_btn_initialize(&conf);
    app_btn_register_callback(APP_BTN_EVT_PRESSED, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_RELEASED, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_HOLD, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_HOLD_SO_LONG, on_button_event_cb, NULL);
}

/* private */
static void button_scan_task(void *param)
{
    os_trace("button_scan_task\n");
    while (true)
    {
        // m_switch_press_release_loop();
        // m_switch_cyclic_timer_loop();
        app_btn_scan(NULL);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* public */
void m_button_init(void)
{
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