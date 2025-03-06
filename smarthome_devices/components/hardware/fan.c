#include "hardware.h"
#include "hw_uart.h"

void hw_fan_init(void)
{
    bl_gpio_enable_output(SWITCH1, 1, 0);
    bl_gpio_enable_output(SWITCH2, 1, 0);
    bl_gpio_enable_output(SWITCH3, 1, 0);
    bl_gpio_enable_output(SWITCH_SWING, 1, 0);

    bl_gpio_output_set(SWITCH1, SWITCH_IDLE_LEVEL);
    bl_gpio_output_set(SWITCH2, SWITCH_IDLE_LEVEL);
    bl_gpio_output_set(SWITCH3, SWITCH_IDLE_LEVEL);
    bl_gpio_output_set(SWITCH_SWING, SWITCH_IDLE_LEVEL);

    hw_uart_init();
}