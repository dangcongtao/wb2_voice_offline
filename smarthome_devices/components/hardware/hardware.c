#include "hardware.h"
#include <hosal_timer.h>
#include <blog.h>
#include <stdarg.h>

uint32_t device_type = 0;
uint32_t max_number_channel = 0;
char *gs_dev_type[] = {"wswitch2v", "lhswitch2v", "wsdatic"};
static uint32_t btn_tick_ms = 0;

void m_log(const char *fmt, ...)
{
    int n;
    char aBuffer[256];
    char *p = &aBuffer[0];
    int size = 256;

    va_list args;

    va_start(args, fmt);
    n = vsnprintf(p, size, fmt, args);

    printf("%s", aBuffer);

    va_end(args);

    printf("\r");
}

void gpio_set_output(int pin)
{
    bl_gpio_enable_output(pin, 0, 0);
}

void gpio_set_input(int pin)
{
    bl_gpio_enable_input(pin, 0, 0);
}

void hw_pcb_sort_pins(void)
{
    hw_switch_match_pcb();
    hw_button_match_pcb();
}

static void hw_timer_callback(void *args) { btn_tick_ms++; }

uint32_t hw_get_tick_ms(void) { return btn_tick_ms; }

void hw_timer_init(void)
{
    static hosal_timer_dev_t timer0 = {
        .config = {
            .arg = NULL,
            .cb = hw_timer_callback,
            .period = 100, /// 1000us
            .reload_mode = TIMER_RELOAD_PERIODIC,
        },
        .port = 0,
    };
    hosal_timer_init(&timer0);
    hosal_timer_start(&timer0);
}