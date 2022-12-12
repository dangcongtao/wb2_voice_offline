#include "hardware.h"


uint32_t device_type = 0;
uint32_t max_number_channel = 0;
char* gs_dev_type[] = {"wswitch2v", "lhswitch2v", "wsdatic"};
static uint32_t btn_tick_ms = 0;

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
    // hw_timer_init(hw_timer_callback, NULL);
    // hw_timer_alarm_us(1000, true); /* 1ms */
}