#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <bl_gpio.h>
#include "app_config.h"
#include "system_typedef.h"

#define gpio_get_level bl_gpio_input_get_value
#define gpio_set_level bl_gpio_output_set

#define LED_STT 14
#define LED_STT_VOICEBOX 2
#define LED_STT_SSW 1
#define LED_STT_MINI 0xFF

#define LED_STT_ON 0
#define LED_STT_OFF 1

#define LED_INDICATE_OFF 0
#define LED_INDICATE_ON 1

#define MAX_NUM_BUTTON 6
#define BTN_IDLE_LEVEL 0
#define BUTTON1 0xFF
#define BUTTON2 0xFF
#define BUTTON3 0xFF
#define BUTTON4 0xFF

#define BUTTON_ADC_PIN 11
#define BUTTON_ADC_CHANNEL 10

#define TEMP_ADC_PIN 11
#define TEMP_ADC_CHANNEL 10

#define BUTTON_POWER_INDEX 0
#define BUTTON_SWING_INDEX 1
#define BUTTON_NATURAL_INDEX 2

#define BTN_CFG_MINI 22
#if !defined(VOICEBOX_HW) || !defined(VOICEBOX_HW_V1)
#error "VOICEBOX_HW is not defined: include app_config.h first"
#endif
#if VOICEBOX_HW == VOICEBOX_HW_V1
/* VOICEBOX V1: one function button (config / next voice) at index 0 */
#define BTN_VOICEBOX_FUNCTION_PIN 1
#define BTN_VOICEBOX_COUNT 1U
#define BTN_VOICEBOX_PINS {BTN_VOICEBOX_FUNCTION_PIN}
#else
/* VOICEBOX V2: index 0 = volume up, 1 = function (config / next voice),
 * 2 = volume down */
#define BTN_VOICEBOX_VOLUME_UP_PIN 21
#define BTN_VOICEBOX_FUNCTION_PIN 20
#define BTN_VOICEBOX_VOLUME_DOWN_PIN 0
#define BTN_VOICEBOX_COUNT 3U
#define BTN_VOICEBOX_PINS                \
    {BTN_VOICEBOX_VOLUME_UP_PIN,         \
     BTN_VOICEBOX_FUNCTION_PIN,          \
     BTN_VOICEBOX_VOLUME_DOWN_PIN}
#endif
/* Pressed = 0, internal pull-up on every VOICEBOX button */
#define BTN_VOICEBOX_IDLE_LEVEL 1
#define BUTTON_SSW 0xFF

#define MAX_NUM_SWITCH 4
#define SWITCH_IDLE_LEVEL 0
#define SWITCH_ACTIVE_LEVEL 1
#define SWITCH1 0xFF
#define SWITCH2 0xFF
#define SWITCH3 0xFF
#define SWITCH_SWING 0xFF

#define SWITCH1_MINI 0xFF
#define SWITCH2_MINI 14

#define TS04 0xFF
#define TS04_SWSHOCK 0xFF
#define TS04_IDLE_LEVEL 0
#define TS04_ACTIVE_LEVEL 1

#define BAT_LV_PIN 12
#define BAT_LV_ADC_CHANNEL 0
#define BAT_CHARGE_STATUS_PIN  17

#define BAT_CHARGE_STATUS_CHARGING 0
#define BAT_CHARGE_STATUS_DISCHARGING 1

#define LED_COM_PIN 2
#define SHCP_PIN 21
#define STCP_PIN 20
#define DS_PIN 22
#define OE_PIN 0
#define OE_PIN_2 0xFF

/* use for fan AC */
#define BUZZ_PIN 17

extern uint32_t max_number_channel;
extern uint32_t device_type;

/* timer */
uint32_t sys_get_tick_ms(void);

/* button */
void hw_button_config(uint32_t index);
uint32_t hw_btn_read(uint32_t index);
void hw_ts04_write(uint8_t value);
void hw_btn_set_val_button(uint8_t val);

/* led */
void hw_led_stt_cfg(void);
void hw_led_stt_on(void);
void hw_led_stt_off(void);
void hw_toggle_led_stt(void);

/* switch */
void hw_write_switch_pin(uint8_t index, uint8_t val);
uint8_t hw_read_switch_pin(uint8_t index);
void hw_switch_change_pin_type(void);

/* 7 seg*/
void hw_74hc595_init(void);
void hw_74hc595_set_bit(uint8_t bit, uint8_t val);
void hw_74hc595_set_number(uint8_t num);
void hw_74hc595_loop(void);

/* buzz */
void hw_buzz_set(uint8_t buzz_set);
void hw_buzz_set_type(uint8_t buzz_set, uint8_t is_long);
void hw_buzz_init(void);

/** @brief Check whether the current device supports voice IC.
 *
 *  @return     Voice IC support status.
 *  @retval     true   Device supports voice IC (voiceboxv1 only).
 *  @retval     false  Device does not support voice IC.
 */
bool hw_device_support_voice(void);

/* hardware init. should call this api first */
void gpio_set_output(int pin);
void gpio_set_input(int pin);

/* Temperature */
uint32_t hw_temp_read(void);
uint32_t hw_bat_read(void);
void hw_temp_config(void);

#endif