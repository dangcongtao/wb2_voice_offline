#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <bl_gpio.h>

#define gpio_get_level      bl_gpio_input_get_value
#define gpio_set_level      bl_gpio_output_set

#define LED_STT 17
#define LED_STT_ON      0
#define LED_STT_OFF     1

#define MAX_NUM_BUTTON  4
#define BTN_IDLE_LEVEL  1
#define BUTTON1 4
#define BUTTON2 2
#define BUTTON3 5
#define BUTTON4 1

#define MAX_NUM_SWITCH          4
#define SWITCH_IDLE_LEVEL       0
#define SWITCH_ACTIVE_LEVEL     1
#define SWITCH1 22
#define SWITCH2 0
#define SWITCH3 20
#define SWITCH4 21

extern uint32_t max_number_channel;
extern uint32_t device_type;

/* timer */
uint32_t sys_get_tick_ms(void);

/* button */
void hw_button_config(uint32_t index);
uint32_t hw_btn_read(uint32_t index);

/* led */
void hw_led_stt_cfg(void);
void hw_led_stt_on(void);
void hw_led_stt_off(void);
void hw_toggle_led_stt(void);

/* switch */
void hw_switch_cfg(uint8_t index);
void hw_write_switch_pin(uint8_t index, uint8_t val);
uint8_t hw_read_switch_pin(uint8_t index);

/* hardware init. should call this api first */
void gpio_set_output(int pin);
void gpio_set_input(int pin);

#endif