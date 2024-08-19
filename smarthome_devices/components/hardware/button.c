#include "hardware.h"
#include "system_typedef.h"

uint32_t button_arr[] = {BUTTON1, BUTTON2, BUTTON3, BUTTON4};

void hw_button_config(uint32_t index)
{
    // os_trace("hw_button_config: %ld\r\n", index);

    if ((device_type != SWMINI && index >= max_number_channel) || (device_type == SWMINI && index >= max_number_channel + 1))
        return;

    gpio_set_input(button_arr[index]);
}

uint32_t hw_btn_read(uint32_t index)
{
    // os_trace("(%ld)hw_btn_read: %ld: %ld\r\n", sys_get_tick_ms(), index, (uint32_t)gpio_get_level(button_arr[index]));

    if ((device_type != SWMINI && index >= max_number_channel) || (device_type == SWMINI && index >= max_number_channel + 1))
        return BTN_IDLE_LEVEL; /* error */

    return (uint32_t)gpio_get_level(button_arr[index]);
}