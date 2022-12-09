#include "hardware.h"

static uint8_t led_stt_val = LED_STT_ON;

void hw_led_stt_cfg(void)
{
    gpio_set_output(LED_STT);
}

void hw_led_stt_on(void)
{
    led_stt_val = LED_STT_ON;
    gpio_set_level(LED_STT, led_stt_val);
}

void hw_led_stt_off(void)
{
    led_stt_val = LED_STT_OFF;
    gpio_set_level(LED_STT, led_stt_val);
}

void hw_toggle_led_stt(void)
{
    led_stt_val = !led_stt_val;
    gpio_set_level(LED_STT, led_stt_val);
}
