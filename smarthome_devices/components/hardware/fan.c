#include "hardware.h"

void hw_fan_init(void)
{
    bl_gpio_enable_output(SWITCH1, 1, 0);
    bl_gpio_enable_output(SWITCH2, 1, 0);
    bl_gpio_enable_output(SWITCH3, 1, 0);
    bl_gpio_enable_output(SWITCH_SWING, 1, 0);

    bl_gpio_output_set(SWITCH1, LED_STT_OFF);
    bl_gpio_output_set(SWITCH2, LED_STT_OFF);
    bl_gpio_output_set(SWITCH3, LED_STT_OFF);
    bl_gpio_output_set(SWITCH_SWING, LED_STT_OFF);
}

void hw_fan_set_speed(uint8_t speed)
{
    switch (speed)
    {
    case 0:
        bl_gpio_output_set(SWITCH1, LED_STT_OFF);
        bl_gpio_output_set(SWITCH2, LED_STT_OFF);
        bl_gpio_output_set(SWITCH3, LED_STT_OFF);
        break;
    case 1:
        bl_gpio_output_set(SWITCH2, LED_STT_OFF);
        bl_gpio_output_set(SWITCH3, LED_STT_OFF);
        bl_gpio_output_set(SWITCH1, LED_STT_ON);
        break;
    case 2:
        bl_gpio_output_set(SWITCH1, LED_STT_OFF);
        bl_gpio_output_set(SWITCH3, LED_STT_OFF);
        bl_gpio_output_set(SWITCH2, LED_STT_ON);
        break;
    case 3:
        bl_gpio_output_set(SWITCH2, LED_STT_OFF);
        bl_gpio_output_set(SWITCH1, LED_STT_OFF);
        bl_gpio_output_set(SWITCH3, LED_STT_ON);
        break;
    default:
        break;
    }
}

void hw_fan_set_swing(uint8_t swing)
{
    if (swing)
    {
        bl_gpio_output_set(SWITCH_SWING, LED_STT_OFF);
    }
    else
    {
        bl_gpio_output_set(SWITCH_SWING, LED_STT_ON);
    }
}