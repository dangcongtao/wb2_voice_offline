#include "hardware.h"
#include "bl_timer.h"

void hw_74hc595_init(void)
{
    bl_gpio_enable_output(SHCP_PIN, 1, 0);
    bl_gpio_enable_output(STCP_PIN, 1, 0);
    bl_gpio_enable_output(DS_PIN, 1, 0);
    bl_gpio_enable_output(OE_PIN, 1, 0);

    bl_gpio_output_set(SHCP_PIN, 1);
    bl_gpio_output_set(STCP_PIN, 1);
    bl_gpio_output_set(DS_PIN, 0);
    bl_gpio_output_set(OE_PIN, 0);
}

void hw_74hc595_write_byte(uint8_t data)
{
    bl_gpio_output_set(SHCP_PIN, 0);
    bl_gpio_output_set(STCP_PIN, 0);
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        bl_gpio_output_set(DS_PIN, (data & 0x80) ? 1 : 0);
        data <<= 1;
        bl_gpio_output_set(SHCP_PIN, 1);
        bl_timer_delay_us(4);
        bl_gpio_output_set(SHCP_PIN, 0);
    }
    bl_gpio_output_set(STCP_PIN, 1);
    bl_timer_delay_us(4);
    bl_gpio_output_set(STCP_PIN, 0);
    
    bl_gpio_output_set(SHCP_PIN, 1);
    bl_gpio_output_set(STCP_PIN, 1);
}