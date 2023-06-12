#include "hardware.h"
#include "system_typedef.h"

static uint32_t switch_arr[] = {SWITCH1, SWITCH2, SWITCH3, SWITCH4};
static uint8_t switch_state[4] = {SWITCH_IDLE_LEVEL};

void hw_switch_cfg(uint8_t index)
{
    gpio_set_output(switch_arr[index]);

    switch_state[index] = SWITCH_IDLE_LEVEL;
    gpio_set_level(switch_arr[index], switch_state[index]);
}

void hw_write_switch_pin(uint8_t index, uint8_t val)
{
    if (index >= max_number_channel)
        return;
    // os_trace("hw_write_switch_pin: %d: %d-%d\n", index, switch_arr[index], val);


    switch_state[index] = val;
    bl_gpio_output_set(switch_arr[index], switch_state[index]); /* set relay */
    if (device_type == WSWITCH || device_type == WSDATIC || device_type == LHSWITCH)
    {
        if (max_number_channel == 1 && index == 0)
            bl_gpio_output_set(switch_arr[1], val); /* bug in hardware pcb so need do it for fake
                                    led indicator */
        else if (max_number_channel == 2 && index == 1)
            bl_gpio_output_set(switch_arr[2], val); /* bug in hardware pcb so need do it for fake
                                    led indicator */
    }
}

uint8_t hw_read_switch_pin(uint8_t index)
{
    if (index >= max_number_channel)
        return SWITCH_IDLE_LEVEL;

    // os_trace("hw_read_switch_pin: %d: %d\n", index, switch_state[index]);

    return switch_state[index];
}

void hw_switch_change_pin_type(void)
{
    if (device_type == SSWITCH) {
       switch_arr[0] = SWITCH2;
    }
}