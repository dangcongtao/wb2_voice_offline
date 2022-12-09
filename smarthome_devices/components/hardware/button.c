#include "hardware.h"
#include "system_typedef.h"

uint32_t button_arr[] = {BUTTON1, BUTTON2, BUTTON3, BUTTON4};

static uint32_t btn_tick_ms = 0;

static void hw_timer_callback(void *args) { btn_tick_ms++; }

uint32_t hw_get_tick_ms(void) { return btn_tick_ms; }

void hw_timer_init(void)
{
    // hw_timer_init(hw_timer_callback, NULL);
    // hw_timer_alarm_us(1000, true); /* 1ms */
}

void hw_button_config(uint32_t index)
{
    if (index >= max_number_channel)
        return;

    if (device_type == WSWITCH)
    {
        if (max_number_channel == 1)
        {
            gpio_set_output(button_arr[1]);
        }
        else if (max_number_channel == 2)
        {
            if (index == 1)
            {
                gpio_set_output(button_arr[3]);
            }
        }
        else
        {
            gpio_set_output(button_arr[3]);
        }
    }
    else
    {
        printf("hw_button_config: %ld\n", index);
        gpio_set_output(button_arr[index]);
    }
}

uint32_t hw_btn_read(uint32_t index)
{
    if (index >= max_number_channel)
        return 0; /* error */

    if (device_type == WSWITCH)
    {
        if (max_number_channel == 1)
        {
            return (uint32_t)gpio_get_level(button_arr[1]);
        }
        else if (max_number_channel == 2)
        {
            if (index == 1)
            { /* CHANNE 2 */
                return (uint32_t)gpio_get_level(button_arr[2]);
            }
        }
    }

    return (uint32_t)gpio_get_level(button_arr[index]);
}

/* Public */
void hw_button_match_pcb(void)
{
    if (device_type == WSWITCH)
    {
        switch (max_number_channel)
        {
        case 1:
        {
        }
        break;
        case 2:
        {
        }
        break;
        case 3:
        {
        }
        break;
        case 4:
        {
            button_arr[0] = BUTTON4;
            button_arr[1] = BUTTON3;
            button_arr[2] = BUTTON2;
            button_arr[3] = BUTTON1;
        }
        break;

        default:
            break;
        }
    }
    else if (device_type == LHSWITCH)
    {
        switch (max_number_channel)
        {
        case 1:
        {
            printf("lahu 01\n");
            button_arr[0] = BUTTON3;
            button_arr[1] = BUTTON4;
            button_arr[2] = BUTTON2;
            button_arr[3] = BUTTON1;
        }
        break;
        case 2:
        {
            printf("lahu 02\n");
            button_arr[0] = BUTTON3;
            button_arr[1] = BUTTON4;
            button_arr[2] = BUTTON2;
            button_arr[3] = BUTTON1;
        }
        break;
        case 3:
        {
        }
        break;
        case 4:
        {
            button_arr[0] = BUTTON3;
            button_arr[1] = BUTTON4;
            button_arr[2] = BUTTON1;
            button_arr[3] = BUTTON2;
        }
        break;

        default:
            break;
        }
    }
}