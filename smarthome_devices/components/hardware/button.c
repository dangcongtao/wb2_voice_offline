#include "hardware.h"
#include "system_typedef.h"

#include "hosal_adc.h"

static int val = 0;
static uint8_t val_button = 0;

static hosal_adc_dev_t adc_temperature = {
    .cb = NULL,
    .config = {
        .mode = HOSAL_ADC_ONE_SHOT,
        .pin = BUTTON_ADC_PIN,
        .sampling_freq = 340,
    },
    .dma_chan = 0,
    .p_arg = NULL,
    .port = 0,
};

void hw_button_config(uint32_t index)
{
    // os_trace("hw_button_config: %ld\r\n", index);

    if ((device_type != SWMINI && index >= max_number_channel) || (device_type == SWMINI && index >= max_number_channel + 1))
        return;

    // gpio_set_input(button_arr[index]);
    hosal_adc_init(&adc_temperature);
    hosal_adc_add_channel(&adc_temperature, BUTTON_ADC_CHANNEL);
}

void hw_btn_read_adc(void)
{
    int val_adc = hosal_adc_value_get(&adc_temperature, BUTTON_ADC_CHANNEL, 100);
    // os_trace("%s idx: %lu, val_adc: %d\r\n", __func__, index, val);
    if (val_adc > 0)
    {
        val = val_adc;
    }
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