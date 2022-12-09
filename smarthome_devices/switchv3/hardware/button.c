#include "hardware.h"

gpio_num_t button_arr[4] = {BUTTON1, BUTTON2, BUTTON3, BUTTON4};

static uint32_t btn_tick_ms = 0;

uint32_t hw_get_tick_ms(void) { return btn_tick_ms; }

static void hw_timer_callback(void *args) { btn_tick_ms++; }

static void hw_button_timer_init()
{
    // hw_timer_init(hw_timer_callback, NULL);
    // hw_timer_alarm_us(1000, true); /* 1ms */
}

static void hw_button_config(uint32_t index)
{
    if (index >= max_number_channel)
        return;

    if (device_type == WSWITCH)
    {
        if (max_number_channel == 1)
        {
            gpio_set_ouput_pin(button_arr[1]);
        }
        else if (max_number_channel == 2)
        {
            if (index == 1)
            {
                gpio_set_ouput_pin(button_arr[3]);
            }
        }
        else
        {
            gpio_set_ouput_pin(button_arr[3]);
        }
    }
    else
    {
        printf("hw_button_config: %d\n", index);
        gpio_set_ouput_pin(button_arr[index]);
    }
}

static uint32_t hw_btn_read(uint32_t index)
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
void hw_button_register_driver(void)
{
    m_button_t buttons_conf;

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
            button_arr[0] = GPIO_PIN_BTN_4;
            button_arr[1] = GPIO_PIN_BTN_3;
            button_arr[2] = GPIO_PIN_BTN_2;
            button_arr[3] = GPIO_PIN_BTN_1;
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
            button_arr[0] = GPIO_PIN_BTN_3;
            button_arr[1] = GPIO_PIN_BTN_4;
            button_arr[2] = GPIO_PIN_BTN_2;
            button_arr[3] = GPIO_PIN_BTN_1;
        }
        break;
        case 2:
        {
            printf("lahu 02\n");
            button_arr[0] = GPIO_PIN_BTN_3;
            button_arr[1] = GPIO_PIN_BTN_4;
            button_arr[2] = GPIO_PIN_BTN_2;
            button_arr[3] = GPIO_PIN_BTN_1;
        }
        break;
        case 3:
        {
        }
        break;
        case 4:
        {
            button_arr[0] = GPIO_PIN_BTN_3;
            button_arr[1] = GPIO_PIN_BTN_4;
            button_arr[2] = GPIO_PIN_BTN_1;
            button_arr[3] = GPIO_PIN_BTN_2;
        }
        break;

        default:
            break;
        }
    }

    buttons_conf._btn_max_number_channel = max_number_channel;
    buttons_conf._btn_idle_level = BTN_IDLE_LEVEL;
    buttons_conf._button_conf_cb = hw_button_config;
    buttons_conf._button_read_cb = hw_btn_read;
    buttons_conf._button_timer_init_cb = hw_button_timer_init;
    buttons_conf._button_get_tick_ms_cb = hw_get_tick_ms;

    m_button_config(buttons_conf);
}