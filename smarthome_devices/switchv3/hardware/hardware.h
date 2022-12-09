#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <bl_gpio.h>

#define gpio_set_ouput_pin(uint32_t pin) \
    do { \
        bl_gpio_enable_input(pin, 0, 0); \
        bl_gpio_enable_output(GPIO_LED_PIN, 0, 0); \
    } while(0);
    
#define gpio_set_input_pin(uint32_t pin) \
    do { \
        bl_gpio_enable_input(pin, 0, 0); \
    } while(0);

#define gpio_get_level      bl_gpio_input_get_value
#define gpio_set_level      bl_gpio_output_set

#define SWITCH_IDLE_LEVEL   0
#define LED_ON      0
#define LED_OFF     1

#define LED_STT 12

#define BUTTON1 1
#define BUTTON2 5
#define BUTTON3 8

#define SWITCH1 14
#define SWITCH2 17
#define SWITCH3 3

uint32_t hw_get_tick_ms(void);

void hw_led_stt_cfg(void);
void hw_led_stt_on(void);
void hw_led_stt_off(void);
void hw_toggle_led_stt(void);

void hw_switch_cfg(uint8_t index);
void hw_write_switch_pin(uint8_t index, uint8_t val);
uint8_t hw_read_switch_pin(uint8_t index);

#endif