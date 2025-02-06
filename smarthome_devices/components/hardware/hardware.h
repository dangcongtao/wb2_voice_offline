#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <bl_gpio.h>
#include "app_config.h"
#include "system_typedef.h"

#define gpio_get_level      bl_gpio_input_get_value
#define gpio_set_level      bl_gpio_output_set

#define LED_STT 5
#define LED_STT_SSW     1
#define LED_STT_MINI    7

#define LED_STT_ON      0
#define LED_STT_OFF     1

#define MAX_NUM_BUTTON  4
#define BTN_IDLE_LEVEL  0
#define BUTTON1    11
#define BUTTON2    5
#define BUTTON3   21
#define BUTTON4   20

#define BUTTON_ADC_PIN             11
#define BUTTON_ADC_CHANNEL             10

#define BTN_CFG_MINI    22
#define BUTTON_SSW  4

#define MAX_NUM_SWITCH          4
#define SWITCH_IDLE_LEVEL       0
#define SWITCH_ACTIVE_LEVEL     1
#define SWITCH1   14
#define SWITCH2   17 
#define SWITCH3    3
#define SWITCH_SWING    12

#define SWITCH1_MINI    3
#define SWITCH2_MINI    14

#define TS04    22
#define TS04_SWSHOCK    20
#define TS04_IDLE_LEVEL     0
#define TS04_ACTIVE_LEVEL   1

#define IR_RX   7
#define IR_RX_POS   1
#define IRpin   0xFF

#define LED_COM_PIN 2
#define SHCP_PIN    21
#define STCP_PIN    20
#define DS_PIN    22
#define OE_PIN    0

extern uint32_t max_number_channel;
extern uint32_t device_type;

/* timer */
uint32_t sys_get_tick_ms(void);

/* button */
void hw_button_config(uint32_t index);
uint32_t hw_btn_read(uint32_t index);
void hw_btn_read_adc(void);
void hw_ts04_write(uint8_t value);

/* led */
void hw_led_stt_cfg(void);
void hw_led_stt_on(void);
void hw_led_stt_off(void);
void hw_toggle_led_stt(void);

/* switch */
void hw_write_switch_pin(uint8_t index, uint8_t val);
uint8_t hw_read_switch_pin(uint8_t index);
void hw_switch_change_pin_type(void);

/* fan */
void hw_fan_set_speed(uint8_t speed);
void hw_fan_set_swing(uint8_t swing);
void hw_fan_init(void);

/* 7 seg*/
void hw_74hc595_init(void);
void hw_74hc595_write_byte(uint8_t data);

/* hardware init. should call this api first */
void gpio_set_output(int pin);
void gpio_set_input(int pin);

#endif