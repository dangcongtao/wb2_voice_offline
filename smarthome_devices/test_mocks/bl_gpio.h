#ifndef _MOCK_BL_GPIO_H_
#define _MOCK_BL_GPIO_H_

static inline int bl_gpio_input_get_value(int pin) { (void)pin; return 0; }
static inline void bl_gpio_output_set(int pin, int val) { (void)pin; (void)val; }
static inline void bl_gpio_enable_output(uint8_t pin, uint8_t pullup, uint8_t pulldown) { (void)pin; (void)pullup; (void)pulldown; }

#endif
