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
    #if PROJECT_SHOCK
    if (device_type == SWSHOCK) {
        gpio_set_output(LED_SWSHOCK_SW_LEAK_WHITE);
        gpio_set_output(LED_SWSHOCK_SW_LEAK_YELLOW);
        hw_led_sw_not_warning();
    }
    #endif
}

void hw_led_stt_on(void)
{
    led_stt_val = LED_STT_ON;
    gpio_set_level(led_stt_pin, led_stt_val);
}

void hw_led_stt_off(void)
{
    led_stt_val = LED_STT_OFF;
    gpio_set_level(led_stt_pin, led_stt_val);
}

void hw_toggle_led_stt(void)
{
    led_stt_val = !led_stt_val;
    gpio_set_level(led_stt_pin, led_stt_val);
}

#if PROJECT_SHOCK
static uint8_t led_warning_stt = 0;

void hw_led_sw_not_warning(void)
{
    gpio_set_level(LED_SWSHOCK_SW_LEAK_WHITE, LED_STT_ON);
    gpio_set_level(LED_SWSHOCK_SW_LEAK_YELLOW, LED_STT_OFF);
}

void hw_led_sw_warning(void)
{
    led_warning_stt = !led_warning_stt;
    if (led_warning_stt) {
        gpio_set_level(LED_SWSHOCK_SW_LEAK_WHITE, LED_STT_OFF);
        gpio_set_level(LED_SWSHOCK_SW_LEAK_YELLOW, LED_STT_ON);
    }else {
        gpio_set_level(LED_SWSHOCK_SW_LEAK_WHITE, LED_STT_OFF);
        gpio_set_level(LED_SWSHOCK_SW_LEAK_YELLOW, LED_STT_OFF);
    }
}

#endif
