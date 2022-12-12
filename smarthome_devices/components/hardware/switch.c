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
    if (index >= MAX_NUM_SWITCH)
        return;
    // os_trace("hw_write_switch_pin: %d: %d-%d\n", index, switch_arr[index], val);


    switch_state[index] = val;
    bl_gpio_output_set(switch_arr[index], switch_state[index]); /* set relay */
}

uint8_t hw_read_switch_pin(uint8_t index)
{
    if (index >= MAX_NUM_SWITCH)
        return SWITCH_IDLE_LEVEL;

    // os_trace("hw_read_switch_pin: %d: %d\n", index, switch_state[index]);

    return switch_state[index];
}