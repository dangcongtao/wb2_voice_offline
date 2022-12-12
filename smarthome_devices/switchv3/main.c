#include <stdio.h>
#include <string.h>
#include <bl_gpio.h>

#include <FreeRTOS.h>
#include <task.h>

#include "hardware.h"
#include "m_button.h"
#include "m_switch.h"
#include "m_led_stt.h"

#include <blog.h>
#include "bl_sys.h"
#include <stdio.h>
#include <cli.h>
#include <hosal_uart.h>
#include <blog.h>
#include <hosal_uart.h>

#define GPIO_BUTTON_PIN 8
#define GPIO_LED_PIN 12

void main(void)
{
    bl_uart_init(0, 16, 7, 255, 255, 115200);//set uart baud 115200

    hw_pcb_sort_pins();
    m_switch_init();
    m_button_init();
    m_led_stt_init();

    gpio_set_output(LED_STT);

    for (;;) {
        printf("blink\r\n");
        bl_gpio_output_set(SWITCH1, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        bl_gpio_output_set(SWITCH1, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
