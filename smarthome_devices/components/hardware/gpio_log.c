/*
 * gpio_log.c - software (bit-bang) UART log output on a GPIO pin.
 *
 * UART0 (GPIO16/GPIO7) is occupied by the CI13162 voice module, so debug
 * log is redirected to a software UART on GPIO 14 at 9600 baud.
 *
 * GPIO 14 is shared with the status LED (LED_STT): hardware.h remaps
 * LED_STT to 0xFF when DEBUG_ENABLE == 1 so the pin is free for log,
 * same as branch lqh/atm_v3 did with its LED pin.
 *
 * Enabled by CONFIG_HUNONIC_LOG_GPIO:=1 in proj_config.mk: the SDK
 * (evb/src/debug.c, bfl_main.c) then calls sw_uart_init()/sw_uart_send_byte()
 * instead of writing log to hardware UART0.
 */
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>
#include "glb_reg.h"
#include <bl_gpio.h>
#include "bl_timer.h"
#include "hardware.h"

#define PIN 14
#define TIME_BIT_US 104
#define MAX_LEN 512

void ATTR_TCM_SECTION _enable_irq(void)
{
    taskEXIT_CRITICAL();
}

void ATTR_TCM_SECTION _disable_irq(void)
{
    taskENTER_CRITICAL();
}

uint8_t disable_log = 0;

void do_disable_log()
{
    disable_log = 1;
    _enable_irq();
}

void do_enable_log()
{
    disable_log = 0;
}

void ATTR_TCM_SECTION sw_uart_send_byte(uint8_t id, uint8_t data)
{
    if (disable_log)
        return;

    _disable_irq();

    (void)id;

    /* start bit */
    bl_gpio_output_set(PIN, 0);
    bl_timer_delay_us(TIME_BIT_US);

    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & (1 << i))
        {
            bl_gpio_output_set(PIN, 1);
            bl_timer_delay_us(TIME_BIT_US);
        }
        else
        {
            bl_gpio_output_set(PIN, 0);
            bl_timer_delay_us(TIME_BIT_US);
        }
    }

    /* stop bit */
    bl_gpio_output_set(PIN, 1);
    bl_timer_delay_us(TIME_BIT_US);

    _enable_irq();
}

void sw_uart_init(void)
{
    bl_gpio_enable_output(PIN, 0, 0);
}
