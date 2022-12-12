#include "hardware.h"
#include "system_typedef.h"

uint32_t switch_arr[] = {SWITCH1, SWITCH2, SWITCH3, SWITCH4};

void hw_switch_cfg(uint8_t index)
{
    gpio_set_output(switch_arr[index]);
    gpio_set_level(switch_arr[index], SWITCH_IDLE_LEVEL); /* set relay */
}

void hw_write_switch_pin(uint8_t index, uint8_t val)
{
    if (index >= MAX_NUM_SWITCH)
        return;

    gpio_set_level(switch_arr[index], val); /* set relay */
}

uint8_t hw_read_switch_pin(uint8_t index)
{
    if (index >= MAX_NUM_SWITCH)
        return SWITCH_IDLE_LEVEL;

    return (uint8_t)gpio_get_level(switch_arr[index]);
}