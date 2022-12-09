#include "m_button.h"
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_log.h"
#include "esp_system.h"
#include "driver/hw_timer.h"

#include "hardware.h"
#include "m_switch.h"
#include "m_switch_event.h"
#include "m_led_stt.h"
#include "../third_party/huytv_button/app_btn.h"
#include "m_database.h"
#include "app_config.h"
#include "m_wifi.h"


static m_button_t s_button_obj;
uint32_t ignore_event_release[APP_BTN_MAX_BTN_SUPPORT] = {0};

static const char *event_name[] = {"pressed", "release",
                                   "hold", "hold so long",
                                   "double click", "tripple click",
                                   "idle", "idle break", "invalid"};
void on_button_event_cb(int button_pin, int event, void *data)
{
    os_trace("Button %d, event %s\r\n", button_pin, event_name[event]);

    if (m_db_get_instance()->_m_device._switch_info.channel_lock_state[button_pin] == 1)
        if (m_wifi_get_instance().wifi_get_state_cb() == WIFI_DEV_ACTIVE)
            return;
    
    switch (event)
    {
    case APP_BTN_EVT_PRESSED:
    {
    }
    break;

    case APP_BTN_EVT_RELEASED:
    {
        if (ignore_event_release[button_pin]) {
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
        m_wifi_get_instance().start_smart_config_cb();
        m_db_get_instance()->_reset_db_cb();
        ignore_event_release[button_pin] = 1;
    }
    break;

    case APP_BTN_EVT_HOLD_SO_LONG:
    {
        if (ignore_event_release[button_pin] == 2)
            return;
            
        /* factory reset database */
        m_led_stt_get_instance()._set_led_stt_ap_config_cb();
        m_wifi_get_instance().start_ap_config_cb();
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

static void m_button_init(m_button_t m_button_obj)
{
    static app_btn_hw_config_t m_hw_button_initialize_value[APP_BTN_MAX_BTN_SUPPORT];
    
    memset(&s_button_obj, 0, sizeof(s_button_obj));
    s_button_obj = m_button_obj;

    s_button_obj._button_timer_init_cb();

    for (uint32_t i = 0; i < s_button_obj._btn_max_number_channel; i++)
    {
        m_hw_button_initialize_value[i].idle_level = s_button_obj._btn_idle_level;
        m_hw_button_initialize_value[i].last_state = s_button_obj._button_read_cb(i); // read the very first level at initialization
        m_hw_button_initialize_value[i].pin = i;
    }

    app_btn_config_t conf;
    conf.config = &m_hw_button_initialize_value[0];
    conf.btn_count = s_button_obj._btn_max_number_channel;
    conf.get_tick_cb = s_button_obj._button_get_tick_ms_cb;
    conf.btn_initialize = s_button_obj._button_conf_cb;
    conf.btn_read = s_button_obj._button_read_cb;

    app_btn_initialize(&conf);
    app_btn_register_callback(APP_BTN_EVT_PRESSED, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_RELEASED, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_HOLD, on_button_event_cb, NULL);
    app_btn_register_callback(APP_BTN_EVT_HOLD_SO_LONG, on_button_event_cb, NULL);
}

/* private */
static void button_scan_task(void *param)
{
    while (true) {
        m_switch_press_release_loop();
        m_switch_cyclic_timer_loop();
        app_btn_scan(NULL);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* public */
void m_button_config(m_button_t m_button_obj)
{
    m_button_init(m_button_obj);

    xTaskCreate(button_scan_task,       /* Function that implements the task. */
                "led_button_task",          /* Text name for the task. */
                1024 * 3,      /* Stack size in words, not bytes. */
                NULL,    /* Parameter passed into the task. */
                4,/* Priority at which the task is created. */
                NULL );
}

m_button_t m_button_get_instace(void)
{
    return s_button_obj;
}