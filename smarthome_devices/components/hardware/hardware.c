#include "hardware.h"


volatile uint8_t device_type = 0;
volatile uint8_t max_number_channel = 0;
char* gs_dev_type[] = {"wswitch2v", "lhswitch2v", "wsdatic"};


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