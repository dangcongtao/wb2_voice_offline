#include "hardware.h"
#include "system_typedef.h"

static uint8_t val_button = 0;

/** Map a VOICEBOX button index to its GPIO pin, or -1 if out of range. */
static int hw_button_voicebox_pin(uint32_t index)
{
    switch (index)
    {
    case 0U:
        return BTN_VOICEBOX_VOLUME_UP_PIN;
    case 1U:
        return BTN_VOICEBOX_FUNCTION_PIN;
    case 2U:
        return BTN_VOICEBOX_VOLUME_DOWN_PIN;
    default:
        return -1;
    }
}

void hw_button_config(uint32_t index)
{
    if (device_type == VOICEBOX)
    {
        int pin = hw_button_voicebox_pin(index);
        if (pin >= 0)
        {
            gpio_set_input(pin);
        }
    }
}

void hw_btn_set_val_button(uint8_t val)
{
    // os_trace("%s val: %u\r\n", __func__, val);
    val_button = val;
}

uint32_t hw_btn_read(uint32_t index)
{
    if (device_type == VOICEBOX)
    {
        int pin = hw_button_voicebox_pin(index);
        if (pin >= 0)
        {
            return (uint32_t)gpio_get_level(pin);
        }
        return BTN_VOICEBOX_IDLE_LEVEL;
    }
    return (val_button >> index) & 1;
}