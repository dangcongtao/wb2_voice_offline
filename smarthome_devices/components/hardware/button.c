#include "hardware.h"
#include "system_typedef.h"

uint32_t button_arr[] = {BUTTON1, BUTTON2, BUTTON3, BUTTON4};

void hw_ts04_write(uint8_t value)
{
    gpio_set_level(TS04, value);
}

void hw_button_config(uint32_t index)
{
    // os_trace("hw_button_config: %ld\r\n", index);

    if (index >= max_number_channel)
        return;

    if (device_type == WSWITCH) {
    gpio_set_output(TS04);
    hw_ts04_write(TS04_ACTIVE_LEVEL);
    }

    gpio_set_input(button_arr[index]);
}

uint32_t hw_btn_read(uint32_t index)
{
    // os_trace("(%ld)hw_btn_read: %ld: %ld\r\n", sys_get_tick_ms(), index, (uint32_t)gpio_get_level(button_arr[index]));

    if (index >= max_number_channel)
        return BTN_IDLE_LEVEL; /* error */

    return (uint32_t)gpio_get_level(button_arr[index]);
}

void hw_btn_change_btn_pin(void)
{
    if (device_type == WSWITCH || device_type == WSDATIC || device_type == LHSWITCH) {
        switch (max_number_channel)
        {
        case 2:
            button_arr[1] = BUTTON3;
            break;
        default:
            break;
        }
    } else if (device_type == SSWITCH) {
        button_arr[0] = BUTTON_SSW;
    }
}
