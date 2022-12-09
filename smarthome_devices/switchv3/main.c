#include <stdio.h>
#include <string.h>
#include <bl_gpio.h>

#include <FreeRTOS.h>
#include <task.h>

#include "hardware.h"
#include "m_button.h"
#include "m_switch.h"

#define GPIO_BUTTON_PIN 8
#define GPIO_LED_PIN 12

void main(void)
{
    hw_pcb_sort_pins();
    m_switch_init();
    m_button_init();

    for (;;) {
        if (bl_gpio_input_get_value(GPIO_BUTTON_PIN)) {
            bl_gpio_output_set(GPIO_LED_PIN, 1);
        }
        else {
            bl_gpio_output_set(GPIO_LED_PIN, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
