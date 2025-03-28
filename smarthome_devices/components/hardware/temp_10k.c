#include "hardware.h"
#include "hosal_adc.h"
#include "lpf.h"
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
    os_trace("Temperature: %f\r\n", temperature);

    *value = (uint32_t)(temperature * 10);
}

#define DIFF(a, b) (a > b ? (a - b) : (b - a))

uint32_t val_temperature = 0;
uint32_t hw_temp_read(void)
{
    static uint32_t time_stamp;
    static uint32_t last_time_send;
    static uint32_t old_temp;
    static uint8_t is_the_first_time = 0;
    static lpf_data_t m_real_temperature;

    if (!time_stamp)
    {
    }
    else if (sys_get_tick_ms() - time_stamp < 10000)
    {
        return 0;
    }
    time_stamp = sys_get_tick_ms();

    int val = hosal_adc_value_get(&adc_temperature, TEMP_ADC_CHANNEL, 100);
    if (val < 0)
        return 0;

    double voltage = (double)val / 1000;
    double resistance = (voltage * R1) / (VCC - voltage);
    steinhart_hart(resistance, &val_temperature);
    if (!is_the_first_time)
    {
        is_the_first_time = 1;
        m_real_temperature.estimate_value = val_temperature * 1000;
        m_real_temperature.gain = 10; /* 5% */
        os_trace("ADC 0 value: %d, resistance: %lf, temp: %lu\r\n", val, resistance, val_temperature);
    }
    else if (val_temperature < 700 && val_temperature > 100)
    {
        val_temperature *= 1000;
        lpf_update_estimate(&m_real_temperature, &val_temperature);
        val_temperature = m_real_temperature.estimate_value / 1000;
        os_trace("ADC 1 value: %d, resistance: %lf, temp: %lu\r\n", val, resistance, val_temperature);
    }
    else
    {
        val_temperature = m_real_temperature.estimate_value / 1000;
        os_trace("ADC 2 value: %d, resistance: %lf, temp: %lu\r\n", val, resistance, val_temperature);
        return 0;
    }

    if (DIFF(old_temp, val_temperature) > 5 || sys_get_tick_ms() - last_time_send > 60000)
    {
        last_time_send = sys_get_tick_ms();
        old_temp = val_temperature;
        return 1;
    }
    return 0;
}
