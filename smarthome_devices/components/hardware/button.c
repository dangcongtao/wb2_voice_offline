#include "hardware.h"
#include "system_typedef.h"

#include "hosal_adc.h"

static uint8_t val_button = 0;

void hw_button_config(uint32_t index)
{
    if (device_type == VOICEBOX)
    {
        if (index == 0)
        {
            gpio_set_input(BTN_CFG_VOICEBOX);
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
        if (index == 0)
        {
            uint8_t gpio_val = (uint8_t)gpio_get_level(BTN_CFG_VOICEBOX);
            uint8_t touch_val = (val_button >> BUTTON_POWER_INDEX) & 1U;
            /* Active low: 0 means pressed. Active if GPIO pulled low OR touch pad active (1) */
            if ((gpio_val == 0U) || (touch_val == 1U))
            {
                return 0U; /* pressed */
            }
            return 1U; /* idle */
        }
        return 1U;
    }
    return (val_button >> index) & 1;
}