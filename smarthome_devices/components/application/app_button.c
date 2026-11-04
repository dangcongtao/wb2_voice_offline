#include "app_button.h"

#include <stdbool.h>
#include <stdint.h>

#include <FreeRTOS.h>
#include <queue.h>

#include "aiio_ci13162_uart.h"
#include "app_btn.h"
#include "app_cwsl.h"
#include "app_voice.h"
#include "app_volume.h"

#define APP_BUTTON_COUNT 3U
#define APP_BUTTON_QUEUE_LEN 4U

typedef enum
{
    APP_BUTTON_HOLD_NONE = 0,
    APP_BUTTON_HOLD_HOLD,
    APP_BUTTON_HOLD_SO_LONG,
} app_button_hold_t;

static app_button_hold_t s_button_hold[APP_BUTTON_COUNT];
static bool s_function_down = false;
static QueueHandle_t s_button_queue = NULL;
static StaticQueue_t s_button_queue_buf;
static uint8_t s_button_queue_storage[APP_BUTTON_QUEUE_LEN * sizeof(uint8_t)];

static app_button_action_t app_button_function_event(int event)
{
    app_button_hold_t hold = s_button_hold[APP_BUTTON_INDEX_FUNCTION];
    bool down;

    switch (event)
    {
    case APP_BTN_EVT_PRESSED:
        s_function_down = true;
        return APP_BUTTON_ACTION_NONE;
    case APP_BTN_EVT_HOLD:
        s_button_hold[APP_BUTTON_INDEX_FUNCTION] = APP_BUTTON_HOLD_HOLD;
        return APP_BUTTON_ACTION_LED_SMART_CONFIG;
    case APP_BTN_EVT_HOLD_SO_LONG:
        s_button_hold[APP_BUTTON_INDEX_FUNCTION] = APP_BUTTON_HOLD_SO_LONG;
        return APP_BUTTON_ACTION_LED_AP_CONFIG;
    case APP_BTN_EVT_RELEASED:
        s_button_hold[APP_BUTTON_INDEX_FUNCTION] = APP_BUTTON_HOLD_NONE;
        down = s_function_down;
        s_function_down = false;
        if (hold == APP_BUTTON_HOLD_HOLD)
        {
            return APP_BUTTON_ACTION_SMART_CONFIG;
        }
        if (hold == APP_BUTTON_HOLD_SO_LONG)
        {
            return APP_BUTTON_ACTION_AP_CONFIG;
        }
        // A release with no prior press (e.g. boot-time scan) must not change the voice.
        return down ? APP_BUTTON_ACTION_VOICE_NEXT : APP_BUTTON_ACTION_NONE;
    default:
        return APP_BUTTON_ACTION_NONE;
    }
}

app_button_action_t app_button_voicebox_event(uint32_t index, int event)
{
    if (index == APP_BUTTON_INDEX_FUNCTION)
    {
        return app_button_function_event(event);
    }
    if (event != APP_BTN_EVT_PRESSED)
    {
        return APP_BUTTON_ACTION_NONE;
    }
    if (index == APP_BUTTON_INDEX_VOLUME_UP)
    {
        return APP_BUTTON_ACTION_VOLUME_UP;
    }
    if (index == APP_BUTTON_INDEX_VOLUME_DOWN)
    {
        return APP_BUTTON_ACTION_VOLUME_DOWN;
    }
    return APP_BUTTON_ACTION_NONE;
}

void app_button_init(void)
{
    s_button_queue = xQueueCreateStatic(APP_BUTTON_QUEUE_LEN,
                                        sizeof(uint8_t),
                                        s_button_queue_storage,
                                        &s_button_queue_buf);
}

bool app_button_post(app_button_action_t action)
{
    uint8_t item = (uint8_t)action;

    if ((action != APP_BUTTON_ACTION_VOLUME_UP) &&
        (action != APP_BUTTON_ACTION_VOLUME_DOWN) &&
        (action != APP_BUTTON_ACTION_VOICE_NEXT))
    {
        return false;
    }
    if (s_button_queue == NULL)
    {
        return false;
    }
    return xQueueSend(s_button_queue, &item, 0) == pdTRUE;
}

void app_button_poll(void)
{
    uint8_t item;

    if (s_button_queue == NULL)
    {
        return;
    }
    while (xQueueReceive(s_button_queue, &item, 0) == pdTRUE)
    {
        bool handled;

        switch ((app_button_action_t)item)
        {
        case APP_BUTTON_ACTION_VOLUME_UP:
            handled = app_volume_button_step(true);
            break;
        case APP_BUTTON_ACTION_VOLUME_DOWN:
            handled = app_volume_button_step(false);
            break;
        case APP_BUTTON_ACTION_VOICE_NEXT:
            handled = app_voice_button_next();
            break;
        default:
            handled = true;
            break;
        }
        if (!handled && !app_cwsl_is_learning())
        {
            (void)aiio_ci13162_uart_send_buzz_type(1U);
        }
    }
}
