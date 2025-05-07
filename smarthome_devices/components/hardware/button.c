#include "hardware.h"
#include "system_typedef.h"

#include "hosal_adc.h"

static uint8_t val_button = 0;

void hw_button_config(uint32_t index)
{
}

void hw_btn_set_val_button(uint8_t val)
{
    // os_trace("%s val: %u\r\n", __func__, val);
    val_button = val;
}

uint32_t hw_btn_read(uint32_t index)
{
    // int val = hosal_adc_value_get(&adc_temperature, BUTTON_ADC_CHANNEL, 100);
    // os_trace("%s idx: %lu, val_adc: %d\r\n", __func__, index, val);
    return (val_button >> index) & 1;
}