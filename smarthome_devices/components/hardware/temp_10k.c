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
    .dma_chan = 2,
    .p_arg = NULL,
    .port = 2,
};

static hosal_adc_dev_t adc_battery = {
    .cb = NULL,
    .config = {
        .mode = HOSAL_ADC_ONE_SHOT,
        .pin = BAT_LV_PIN,
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
    if (device_type == FANDC)
    {
        hosal_adc_init(&adc_battery);
        hosal_adc_add_channel(&adc_battery, BAT_LV_ADC_CHANNEL);

        gpio_set_input(BAT_CHARGE_STATUS_PIN);
    }
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
uint32_t val_battery = 0;
uint8_t bat_percent = 0;
uint8_t bat_charging_status = BAT_CHARGE_STATUS_DISCHARGING;

uint32_t hw_bat_read(void)
{
    static uint32_t time_stamp;
    static uint32_t last_time_send = 0;
    static uint8_t old_bat_val;
    static uint8_t is_the_first_time = 0;
    static lpf_data_t m_real_bat_val_mV;

    if (device_type != FANDC)
    {
        return 0;
    }

    if (!time_stamp)
    {
    }
    else if (sys_get_tick_ms() - time_stamp < 5000)
    {
        return 0;
    }
    time_stamp = sys_get_tick_ms();

    uint32_t need_send = 0;

    if (gpio_get_level(BAT_CHARGE_STATUS_PIN) != bat_charging_status)
    {
        bat_charging_status = gpio_get_level(BAT_CHARGE_STATUS_PIN);
        os_trace("CHARGE STATUS: %d\r\n", !bat_charging_status);
        need_send = 1;
    }

    int val = hosal_adc_value_get(&adc_battery, BAT_LV_ADC_CHANNEL, 100);
    os_trace("BATTERY: %d\r\n", val);
    if (val < 0)
        return need_send;

    val_battery = val;
    if (!is_the_first_time)
    {
        is_the_first_time = 1;
        m_real_bat_val_mV.estimate_value = val_battery * 1000;
        m_real_bat_val_mV.gain = 10; /* 5% */
    }
    else
    {
        val_battery *= 1000;
        lpf_update_estimate(&m_real_bat_val_mV, &val_battery);
        val_battery = m_real_bat_val_mV.estimate_value / 1000;
    }

    if (val_battery > 2500)
    {
        if (val_battery > 2970)
        {
            bat_percent = 100;
        }
        else
        {
            bat_percent = (val_battery - 2500) * 100 / 500;
        }
    }
    else
    {
        bat_percent = 1;
    }

    os_trace("ADC 1 value: %d, bat: %lu, bat_percent: %d\r\n", val, val_battery, bat_percent);

    if (bat_percent < 30 && (DIFF(old_bat_val, bat_percent) > 3 || sys_get_tick_ms() - last_time_send > 60000))
    {
        os_trace("SENT BATTERY: %lu, bat_percent: %u\r\n", val_battery, bat_percent);
        last_time_send = sys_get_tick_ms();
        old_bat_val = bat_percent;
        need_send = 1;
    }
    os_trace("need_send: %lu\r\n", need_send);
    return need_send;
}

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

    hosal_adc_add_channel(&adc_temperature, TEMP_ADC_CHANNEL);
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

    if (DIFF(old_temp, val_temperature) > 3 || sys_get_tick_ms() - last_time_send > 5 * 60000)
    {
        last_time_send = sys_get_tick_ms();
        old_temp = val_temperature;
        return 1;
    }
    return 0;
}
