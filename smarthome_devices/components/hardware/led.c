#include "hardware.h"
#include "system_typedef.h"

static uint8_t led_stt_val = LED_STT_ON;
uint8_t led_stt_pin = LED_STT;

void hw_led_stt_cfg(void)
{
    if (device_type == SSWITCH) {
       led_stt_pin = LED_STT_SSW;
    }
    gpio_set_output(led_stt_pin);
}

void hw_led_stt_on(void)
{
    if (device_type == SSWITCH) {
       led_stt_val = LED_STT_OFF;
    } else {
        led_stt_val = LED_STT_ON;
    }
    gpio_set_level(led_stt_pin, led_stt_val);
}

void hw_led_stt_off(void)
{
    if (device_type == SSWITCH) {
       led_stt_val = LED_STT_ON;
    } else {
        led_stt_val = LED_STT_OFF;
    }
    gpio_set_level(led_stt_pin, led_stt_val);
}

void hw_toggle_led_stt(void)
{
    led_stt_val = !led_stt_val;
    gpio_set_level(led_stt_pin, led_stt_val);
}
