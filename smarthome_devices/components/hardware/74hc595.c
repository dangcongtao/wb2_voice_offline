#include "hardware.h"
#include "bl_timer.h"

void hw_74hc595_init(void)
{
    bl_gpio_enable_output(LED_COM_PIN, 1, 0);
    bl_gpio_enable_output(SHCP_PIN, 1, 0);
    bl_gpio_enable_output(STCP_PIN, 1, 0);
    bl_gpio_enable_output(DS_PIN, 1, 0);
    bl_gpio_enable_output(OE_PIN, 1, 0);
    bl_gpio_enable_output(OE_PIN_2, 1, 0);

    bl_gpio_output_set(SHCP_PIN, 0);
    bl_gpio_output_set(STCP_PIN, 0);
    bl_gpio_output_set(DS_PIN, 0);
    bl_gpio_output_set(OE_PIN, 1);
    bl_gpio_output_set(OE_PIN_2, 0);
}

void hw_74hc595_write_byte(uint16_t data, uint8_t num_hc595)
{
    bl_gpio_output_set(SHCP_PIN, 0);
    bl_gpio_output_set(STCP_PIN, 0);
    uint16_t i;
    uint16_t mask = 0x80;
    if (num_hc595 == 2)
    {
        mask = 0x8000;
    }
    for (i = 0; i < 8 * num_hc595; i++)
    {
        bl_gpio_output_set(DS_PIN, (data & mask) ? 1 : 0);
        data <<= 1;
        bl_gpio_output_set(SHCP_PIN, 1);
        bl_gpio_output_set(SHCP_PIN, 0);
    }
    bl_gpio_output_set(STCP_PIN, 1);
    bl_gpio_output_set(STCP_PIN, 0);
    bl_gpio_output_set(OE_PIN, 0);
    // if (num_hc595 == 2)
    // {
    //     bl_gpio_output_set(OE_PIN_2, 0);
    // }
}
