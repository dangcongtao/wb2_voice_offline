#include "hardware.h"
#include "system_typedef.h"

uint32_t button_arr[] = {BUTTON1, BUTTON2, BUTTON3, BUTTON4};

static uint32_t btn_tick_ms = 0;

void hw_button_config(uint32_t index)
{
    os_trace("hw_button_config: %ld\r\n", index);

    if (index >= MAX_NUM_BUTTON)
        return;

    gpio_set_input(button_arr[index]);
}

uint32_t hw_btn_read(uint32_t index)
{
    // os_trace("(%ld)hw_btn_read: %ld: %ld\r\n", sys_get_tick_ms(), index, (uint32_t)gpio_get_level(button_arr[index]));

    if (index >= MAX_NUM_BUTTON)
        return 0; /* error */

    return (uint32_t)gpio_get_level(button_arr[index]);
}
