#include "hardware.h"
#include "aiio_ci13162_uart.h"

static uint8_t s_buzz_last = SWITCH_IDLE_LEVEL;

void hw_buzz_set(uint8_t buzz_set)
{
    if (hw_device_support_voice())
    {
        if ((buzz_set == SWITCH_ACTIVE_LEVEL) && (s_buzz_last != SWITCH_ACTIVE_LEVEL))
        {
            int32_t ret = aiio_ci13162_uart_send_buzz();
            if (ret != 0)
            {
                os_trace("hw_buzz_set: send_buzz failed (%ld)\r\n", (long)ret);
            }
        }
        s_buzz_last = buzz_set;
        return;
    }

    if (device_type == FANAC)
    {
        os_trace("hw_buzz_set %u\r\n", buzz_set);
        gpio_set_level(BUZZ_PIN, buzz_set);
    }
    else
    {
        hw_74hc595_set_bit(0, buzz_set);
    }
}

void hw_buzz_init(void)
{
    if (hw_device_support_voice())
    {
        return;
    }

    if (device_type == FANAC)
    {
        os_trace("hw_buzz_init\r\n");
        gpio_set_output(BUZZ_PIN);
        gpio_set_level(BUZZ_PIN, SWITCH_IDLE_LEVEL);
    }
}