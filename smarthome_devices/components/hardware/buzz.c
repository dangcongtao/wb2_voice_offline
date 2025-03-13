#include "hardware.h"

void hw_buzz_set(uint8_t buzz_set)
{
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
    if (device_type == FANAC)
    {
        os_trace("hw_buzz_init\r\n");
        gpio_set_output(BUZZ_PIN);
        gpio_set_level(BUZZ_PIN, SWITCH_IDLE_LEVEL);
    }
}