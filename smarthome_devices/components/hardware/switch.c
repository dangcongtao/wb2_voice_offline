#include "hardware.h"
#include "system_typedef.h"
#include "hosal_gpio.h"
#include "m_mqtt.h"
#include "m_database.h"
#include "bl602.h"
#include "bl_irq.h"

static uint32_t switch_arr[] = {SWITCH1, SWITCH2, SWITCH3, SWITCH4};
static uint8_t switch_state[4] = {SWITCH_IDLE_LEVEL, SWITCH_IDLE_LEVEL, SWITCH_IDLE_LEVEL, SWITCH_IDLE_LEVEL};

#if PROJECT_SHOCK
uint8_t is_need_interrupt_relay = 1;
static uint8_t has_leak_current = 0;
uint8_t has_block_leak = 0;
#endif

void hw_switch_cfg(uint8_t index)
{
    gpio_set_output(switch_arr[index]);

    switch_state[index] = SWITCH_IDLE_LEVEL;
    gpio_set_level(switch_arr[index], switch_state[index]);
#if PROJECT_SHOCK
    if (is_shock_device(device_type))
    {
        antishock_init();
    }
#endif
}

void hw_write_switch_pin(uint8_t index, uint8_t val)
{
    if (index >= max_number_channel)
        return;
    os_trace("%s: %d: %lu-%d\r\n", __func__, index, switch_arr[index], val);

    switch_state[index] = val;
    bl_gpio_output_set(switch_arr[index], switch_state[index]); /* set relay */
    if (device_type == WSWITCH || device_type == LHSWITCH)
    {
        if (max_number_channel == 1 && index == 0)
            bl_gpio_output_set(switch_arr[1], val); /* bug in hardware pcb so need do it for fake
                                    led indicator */
        else if (max_number_channel == 2 && index == 1)
            bl_gpio_output_set(switch_arr[2], val); /* bug in hardware pcb so need do it for fake
                                    led indicator */
    }

#if PROJECT_SHOCK
    if (is_shock_device(device_type))
    {
        if (val)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            bl_irq_enable(GPIO_INT0_IRQn);
            hw_led_sw_not_warning();
            is_need_interrupt_relay = 1;
        }
        else
        {
            is_need_interrupt_relay = 0;
            hw_led_sw_not_warning();
            bl_irq_disable(GPIO_INT0_IRQn);
        }
    }
#endif
}

uint8_t hw_read_switch_pin(uint8_t index)
{
    if (index >= max_number_channel)
        return SWITCH_IDLE_LEVEL;

    // os_trace("hw_read_switch_pin: %d: %d\n", index, switch_state[index]);

    return switch_state[index];
}

void hw_switch_change_pin_type(void)
{
    if (device_type == SSWITCH)
    {
        switch_arr[0] = SWITCH2;
    }
    else if (is_shock_device(device_type))
    {
        switch_arr[0] = SWITCH3;
    }
    else if (device_type == SWMINI)
    {
        switch_arr[0] = SWITCH1_MINI;
        switch_arr[1] = SWITCH2_MINI;
    }
}

#if PROJECT_SHOCK
static void touch_irq_handler(void *arg)
{
    if (is_need_interrupt_relay == 1)
    {
        is_need_interrupt_relay = 2;
        if (!has_leak_current)
        {
            has_leak_current = 1;
            bl_gpio_output_set(switch_arr[0], 0);
            switch_state[0] = SWITCH_IDLE_LEVEL;
        }
    }
}

void fake_current_leak(void)
{
    if (is_need_interrupt_relay == 1)
    {
        is_need_interrupt_relay = 2;
        if (!has_leak_current)
        {
            has_leak_current = 1;
            gpio_set_level(switch_arr[0], 0);
        }
    }
}

uint8_t is_leakage_current(void)
{
    return !!has_block_leak;
}

void hw_read_leak_loop(void)
{
    static char respond_msg[256];
    static uint32_t time_blink_cnt = 0;

    if (has_block_leak)
    {
        if (is_need_interrupt_relay == 2)
        {
            os_trace("blink_warning\n");
            time_blink_cnt++;
            if (time_blink_cnt >= 6)
            {
                time_blink_cnt = 0;
                hw_led_sw_warning();
            }
        }
        else
        {
            has_block_leak = 0;
        }
    }

    if (has_leak_current == 1)
    {
        os_trace("co dong do\r\n");
        has_block_leak = 1;
        has_leak_current = 0;
        bl_irq_disable(GPIO_INT0_IRQn);
        hw_led_sw_warning();

        if (m_mqtt_get_instance()._mqtt_state >= MQTT_INTIALIZED)
        {
            memset(respond_msg, 0, sizeof(respond_msg));
            sprintf(respond_msg, "{\"%s\":%d, \"%s\":%d}", gs_dev_type[device_type], 70, CJSON_KEY_LEAKAGE, 1);
            m_mqtt_get_instance()._mqtt_publish(respond_msg, strlen(respond_msg));

            memset(respond_msg, 0, sizeof(respond_msg));
            sprintf(respond_msg, "{\"%s\":%d, \"action\":%d, \"u\":%d, \"act_id\": %d}", gs_dev_type[device_type], 0, CH1_OFF, 0, SRC_ACTION_LEADK_CURR);
            m_mqtt_get_instance()._mqtt_publish(respond_msg, strlen(respond_msg));
            m_db_get_instance()->_set_current_state_cb(0, LOW);
        }
        else
        {
            os_trace("Device is not connect mqtt\n");
        }
    }
}

void antishock_init(void)
{
    bl_gpio_enable_input(LEAKAGE_CURRENT, 1, 0);

    static hosal_gpio_dev_t touch_gpio = {
        .port = LEAKAGE_CURRENT,
    };

    hosal_gpio_irq_set(&touch_gpio, HOSAL_IRQ_TRIG_NEG_PULSE, touch_irq_handler, NULL);
}
#endif