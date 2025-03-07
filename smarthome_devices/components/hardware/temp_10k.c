#include "hardware.h"
#include "hosal_adc.h"
#include <math.h>

#define R1 10000       /* Ohm */
#define VCC (float)3.1 /* V */

static hosal_adc_dev_t adc_temperature = {
    .cb = NULL,
    .config = {
        .mode = HOSAL_ADC_ONE_SHOT,
        .pin = TEMP_ADC_PIN,
        .sampling_freq = 340,
    },
    .dma_chan = 1,
    .p_arg = NULL,
    .port = 1,
};

void hw_temp_config(void)
{
    hosal_adc_init(&adc_temperature);
    hosal_adc_add_channel(&adc_temperature, TEMP_ADC_CHANNEL);
}

static void steinhart_hart(unsigned int resistence, uint32_t *value)
{
    double temperature = 0;
    double a = 0.0011303;
    double b = 0.0002339;
    double c = 0.00000008863;

    temperature = log(resistence);
    double var1 = b * temperature;
    double var2 = c * temperature * temperature * temperature;
    temperature = 1 / (a + var1 + var2);

    // Convert in Celsius
    temperature = temperature - 273.15;
    // os_trace("Temperature: %f\r\n", temperature);

    *value = (uint32_t)(temperature * 10);
}

uint32_t hw_temp_read(void)
{
    static uint32_t time_stamp;
    uint32_t result = 0;

    if (sys_get_tick_ms() - time_stamp < 10000)
    {
        return result;
    }
    time_stamp = sys_get_tick_ms();

    int val = hosal_adc_value_get(&adc_temperature, TEMP_ADC_CHANNEL, 100);
    if (val < 0)
        return result;

    double voltage = (double)val / 1000;
    double resistance = (voltage * R1) / (VCC - voltage);
    steinhart_hart(resistance, &result);
    os_trace("ADC value: %d, resistance: %lf, temp: %lu\r\n", val, resistance, result);

    return result;
}
