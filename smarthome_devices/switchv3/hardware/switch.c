#include "hardware.h"

gpio_num_t switch_arr[4] = {SWITCH1, SWITCH2, SWITCH3, SWITCH4};

void hw_switch_cfg(uint8_t index)
{
    gpio_set_ouput_pin(index);
    gpio_set_level(switch_arr[index], LOW); /* set relay */
}

void hw_write_switch_pin(uint8_t index, uint8_t val)
{
    if (index >= max_number_channel)
        return;

    gpio_set_level(switch_arr[index], val); /* set relay */

    if (device_type == WSWITCH) {
        if (max_number_channel == 1 && index == 0) {
            /* bug in hardware pcb so need do it for fake led indicator */
            gpio_set_level(switch_arr[1], val);
        }
        else if (max_number_channel == 2 && index == 1) {
            /* bug in hardware pcb so need do it for fake led indicator */
            gpio_set_level(switch_arr[2], val);
        }
    }
}

uint8_t hw_read_switch_pin(uint8_t index)
{
    if (index >= max_number_channel)
        return SWITCH_IDLE_LEVEL;

    return (uint8_t)gpio_get_level(switch_arr[index]);
}
