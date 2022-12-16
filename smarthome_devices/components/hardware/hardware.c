#include "hardware.h"
#include <aos/kernel.h>
#include <blog.h>
#include <stdarg.h>

uint32_t device_type = 0;
uint32_t max_number_channel = 0;
char *gs_dev_type[] = {"wswitch2v", "lhswitch2v", "wsdatic"};

void m_log(const char *fmt, ...)
{
    char aBuffer[256];
    char *p = &aBuffer[0];
    int size = 256;

    va_list args;

    va_start(args, fmt);
    vsnprintf(p, size, fmt, args);

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

uint32_t sys_get_tick_ms(void) { return aos_now_ms(); }