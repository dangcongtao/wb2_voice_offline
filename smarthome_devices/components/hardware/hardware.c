#include "hardware.h"
#include <aos/kernel.h>
#include <blog.h>
#include <stdarg.h>

uint32_t device_type = 0;
uint32_t max_number_channel = 0;
char *gs_dev_type[] = {"wswitch2v", "lhswitch2v", "wsdatic"};

#define MAX_WIDTH   100
#define MAX_LEN     1024

void m_log(const char *fmt, ...)
{
    int size = MAX_WIDTH;
    char msg[MAX_WIDTH];
    char *p = &msg[0];

    va_list args;

    va_start(args, fmt);
    vsnprintf(p, size, fmt, args);

    printf("%*s", MAX_WIDTH, msg);

    va_end(args);
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