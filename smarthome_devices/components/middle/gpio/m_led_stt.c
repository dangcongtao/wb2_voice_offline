
/*************** New handler ****************/
#include "m_led_stt.h"
#include "hardware.h"
#include "system_typedef.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>

#define TWO_PULSE_COUNT 2
#define FOUR_PULSE_COUNT 4

static m_led_stt_t s_led_stt_obj;
static TimerHandle_t xTimers;
static uint16_t timeline_status[] = {200, 200, 200, 200, 200, 200, 200, 200};

static void m_start_timer(uint32_t time_ms);
static void m_stop_timer(void);

static void vTimerCallback(TimerHandle_t xTimer)
{
    s_led_stt_obj._toggle_led_stt_cb();
    s_led_stt_obj._led_handle_count = (s_led_stt_obj._led_handle_count + 1) % s_led_stt_obj._max_number_handle;
    m_stop_timer();
    m_start_timer(timeline_status[s_led_stt_obj._led_handle_count]);
}

static void m_start_timer(uint32_t time_ms)
{
    if (xTimers != NULL)
    {
        xTimerChangePeriod(xTimers, pdMS_TO_TICKS(time_ms), 10);
        xTimerStart(xTimers, 10);
    }
}

static void m_stop_timer(void)
{
    if (xTimers != NULL)
    {
        xTimerStop(xTimers, 10);
    }
}

static void set_led_stt_smart_config(void)
{
    if (s_led_stt_obj._led_stt_type != LED_STYLE_SMARTCONFIG)
    {
        os_trace("set_led_stt_smart_config\r\n");
        /* Stop timer */
        m_stop_timer();

        /* set time */
        timeline_status[0] = timeline_status[1] = 200;
        s_led_stt_obj._led_handle_count = 0;
        s_led_stt_obj._max_number_handle = TWO_PULSE_COUNT;
        s_led_stt_obj._led_stt_type = LED_STYLE_SMARTCONFIG;

        /* start timer */
        m_start_timer(timeline_status[0]);
    }
}

static void set_led_stt_ap_config(void)
{
    if (s_led_stt_obj._led_stt_type != LED_STYLE_APCONFIG)
    {
        os_trace("set_led_stt_ap_config\r\n");
        /* Stop timer */
        m_stop_timer();

        /* turn led off */
        s_led_stt_obj._set_led_stt_off_cb();

        /* set time */
        timeline_status[0] = 500;
        timeline_status[1] = timeline_status[2] = timeline_status[3] = 200;
        s_led_stt_obj._led_handle_count = 0;
        s_led_stt_obj._max_number_handle = FOUR_PULSE_COUNT;
        s_led_stt_obj._led_stt_type = LED_STYLE_APCONFIG;

        /* start timer */
        m_start_timer(timeline_status[0]);
    }
}

static void set_led_stt_update_ota(void)
{
    if (s_led_stt_obj._led_stt_type != LED_STYLE_UPDATE)
    {
        os_trace("set_led_stt_update_ota\r\n");
        /* Stop timer */
        m_stop_timer();

        /* turn led off */
        s_led_stt_obj._set_led_stt_off_cb();

        /* set time */
        timeline_status[0] = timeline_status[1] = 500;
        s_led_stt_obj._led_handle_count = 0;
        s_led_stt_obj._max_number_handle = TWO_PULSE_COUNT;
        s_led_stt_obj._led_stt_type = LED_STYLE_UPDATE;

        /* start timer */
        m_start_timer(timeline_status[0]);
    }
}

static void set_led_stt_not_config(void)
{
    if ((device_type == WSWITCH || device_type == WSDATIC))
    {
        if (s_led_stt_obj._led_stt_type != LED_STYLE_ON)
        {
            os_trace("set_led_stt_not_config\r\n");
            /* Stop timer */
            m_stop_timer();
            s_led_stt_obj._set_led_stt_on_cb();
            s_led_stt_obj._led_stt_type = LED_STYLE_ON;
        }
    }
    else
    {
        /* Stop timer */
        m_stop_timer();

        /* turn led off */
        s_led_stt_obj._set_led_stt_off_cb();

        /* set time */
        timeline_status[0] = 5000;
        timeline_status[1] = 200;
        s_led_stt_obj._led_handle_count = 0;
        s_led_stt_obj._max_number_handle = TWO_PULSE_COUNT;
        s_led_stt_obj._led_stt_type = LED_STYLE_UPDATE;

        /* start timer */
        m_start_timer(timeline_status[0]);
    }
}

static void set_led_stt_disconnect_mqtt(void)
{
    if ((device_type == WSWITCH || device_type == WSDATIC) || device_type == DATICBSV2)
    {
        if (s_led_stt_obj._led_stt_type != LED_STYLE_OFF)
        {
            os_trace("set_led_stt_disconnect_mqtt\r\n");
            /* Stop timer */
            m_stop_timer();

            /* turn led off */
            s_led_stt_obj._set_led_stt_off_cb();

            /* set time */
            timeline_status[0] = 2000;
            timeline_status[1] = 2000;
            s_led_stt_obj._led_handle_count = 0;
            s_led_stt_obj._max_number_handle = TWO_PULSE_COUNT;
            s_led_stt_obj._led_stt_type = LED_STYLE_MQTT_DISCONN;

            /* start timer */
            m_start_timer(timeline_status[0]);
        }
    }
    else
    {
        if (s_led_stt_obj._led_stt_type != LED_STYLE_ON)
        {
            os_trace("set_led_stt_not_config\r\n");
            /* Stop timer */
            m_stop_timer();
            s_led_stt_obj._set_led_stt_on_cb();
            s_led_stt_obj._led_stt_type = LED_STYLE_ON;
        }
    }
}

static void set_led_stt_device_active(void)
{
    os_trace("set_led_stt_device_active\r\n");
    if ((device_type == WSWITCH || device_type == WSDATIC))
    {
        if (s_led_stt_obj._led_stt_type != LED_STYLE_OFF)
        {
            m_stop_timer();
            s_led_stt_obj._set_led_stt_off_cb();
            s_led_stt_obj._led_stt_type = LED_STYLE_OFF;
        }
    }
    else
    {
        if (s_led_stt_obj._led_stt_type != LED_STYLE_ON)
        {
            m_stop_timer();
            s_led_stt_obj._set_led_stt_on_cb();
            s_led_stt_obj._led_stt_type = LED_STYLE_ON;
        }
    }
}

/* public */
void m_led_stt_init(void)
{
    /* harware dependen */
    s_led_stt_obj._led_stt_type = LED_STYLE_UNDEFINED;
    s_led_stt_obj._max_number_handle = TWO_PULSE_COUNT;
    s_led_stt_obj._init_led_stt_cb = hw_led_stt_cfg;
    s_led_stt_obj._set_led_stt_on_cb = hw_led_stt_on;
    s_led_stt_obj._set_led_stt_off_cb = hw_led_stt_off;
    s_led_stt_obj._toggle_led_stt_cb = hw_toggle_led_stt;

    s_led_stt_obj._led_handle_count = 0;
    s_led_stt_obj._set_led_stt_wifi_not_connect_cb = set_led_stt_not_config;
    s_led_stt_obj._set_led_stt_ap_config_cb = set_led_stt_ap_config;
    s_led_stt_obj._set_led_stt_smart_config_cb = set_led_stt_smart_config;
    s_led_stt_obj._set_led_stt_mqtt_disconnect_cb = set_led_stt_disconnect_mqtt;
    s_led_stt_obj._set_led_stt_mqtt_subscribed_cb = set_led_stt_device_active;
    s_led_stt_obj._set_led_stt_update_ota_cb = set_led_stt_update_ota;

    /* GPIO Leds init */
    s_led_stt_obj._init_led_stt_cb();

    /* init timer handler led */
    xTimers = xTimerCreate("timer_handler_led_stt",
                           pdMS_TO_TICKS(1),
                           pdTRUE,
                           (void *)0,
                           vTimerCallback);

    /* start timer */
    m_start_timer(timeline_status[0]);

    /* Test */
    set_led_stt_not_config();
}

m_led_stt_t m_led_stt_get_instance(void)
{
    return s_led_stt_obj;
}