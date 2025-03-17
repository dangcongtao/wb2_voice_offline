#include "hw_uart.h"
#include "m_ft62f28.h"
#include "m_fan.h"
#include "hardware.h"
#include "m_fan_control.h"

uint8_t has_control_fan = 0;
uint8_t need_buzz_off;

uint8_t flag_send_speed;
uint8_t flag_send_swing;
uint8_t flag_send_led;
uint8_t flag_send_light;
cmd_uart_t data_send;

void m_ft62f28_send_speed(void)
{
    has_control_fan = 1;
    flag_send_speed = 1;
}

void m_ft62f28_send_swing(void)
{
    has_control_fan = 1;
    flag_send_swing = 1;
}

void m_ft62f28_send_light(void)
{
    has_control_fan = 1;
    flag_send_light = 1;
}

void m_ft62f28_send_led(void)
{
    has_control_fan = 1;
    flag_send_led = 1;
}

void m_ft62f28_check_send_change(void)
{
    if (!has_control_fan)
    {
        return;
    }
    m_fan_info_t *fan_info = m_fan_get_instance();
    has_control_fan = 0;
    data_send.header[0] = HEADER_1;
    data_send.header[1] = HEADER_2;
    data_send.cmd = CMD_WIFI_SEND_STATE;

    if (flag_send_speed)
    {
        data_send.data.rcv.state.change_speed = 1;
        if (device_type == FANDC)
        {
            uint8_t speed = fan_info->speed;
            if (fan_info->mode == M_FAN_MODE_NATURAL)
            {
                speed = m_fan_get_current_speed_natural();
                data_send.data.rcv.state.speed = speed;
            }
            else if (fan_info->mode == M_FAN_MODE_PCN)
            {
                data_send.data.rcv.state.speed = fan_info->pcn;
            }
            else
            {
                switch (speed)
                {
                case M_FAN_SPEED_OFF:
                    data_send.data.rcv.state.speed = 0;
                    break;
                case M_FAN_SPEED_1:
                    data_send.data.rcv.state.speed = FAN_SPEED_LEVEL_1;
                    break;
                case M_FAN_SPEED_2:
                    data_send.data.rcv.state.speed = FAN_SPEED_LEVEL_2;
                    break;
                case M_FAN_SPEED_3:
                    data_send.data.rcv.state.speed = FAN_SPEED_LEVEL_3;
                    break;

                default:
                    break;
                }
            }
            os_trace("send_speed: pcn %u\r\n", data_send.data.rcv.state.speed);
        }
        else if (device_type == FANAC || device_type == FANACIR)
        {
            uint8_t speed = fan_info->speed;
            if (fan_info->mode == M_FAN_MODE_NATURAL)
            {
                speed = m_fan_get_current_speed_natural();
                data_send.data.rcv.state.speed = speed;
            }
            else
            {
                data_send.data.rcv.state.speed = fan_info->speed;
            }
        }

        flag_send_speed = 0;
    }
    else
    {
        data_send.data.rcv.state.change_speed = 0;
    }

    if (flag_send_swing)
    {
        data_send.data.rcv.state.change_swing = 1;
        data_send.data.rcv.state.swing = fan_info->swing;
        flag_send_swing = 0;
    }
    else
    {
        data_send.data.rcv.state.change_swing = 0;
    }

    if (flag_send_led)
    {
        // os_trace("send_led: %u\r\n", data_send.data.rcv.state.led_data);
        data_send.data.rcv.state.change_led = 1;
        data_send.data.rcv.state.led_data = m_fan_get_led_stt();
        flag_send_led = 0;
    }
    else
    {
        data_send.data.rcv.state.change_led = 0;
    }

    if (flag_send_light)
    {
        data_send.data.rcv.state.change_light = 1;
        data_send.data.rcv.state.ligth = fan_info->light;
        switch (fan_info->light)
        {
        case FAN_LIGHT_LEVEL_1:
            data_send.data.rcv.state.ligth = 30;
            break;

        case FAN_LIGHT_LEVEL_2:
            data_send.data.rcv.state.ligth = 60;
            break;

        case FAN_LIGHT_LEVEL_3:
            data_send.data.rcv.state.ligth = 100;
            break;

        default:
            break;
        }
        flag_send_light = 0;
    }
    else
    {
        data_send.data.rcv.state.change_light = 0;
    }

    data_send.len_data = 2 + sizeof(cmd_data_rcv_u);

    hw_uart_get_instance()->send_bytes((char *)&data_send, 3 + data_send.len_data);
}

void m_ft62f28_receive_data(uint8_t *data, uint32_t len)
{
    cmd_uart_t *cmd = (cmd_uart_t *)data;
    if (cmd->cmd == CMD_SEND_BUTTON_CHANGE)
    {
        // os_trace("CMD_SEND_BUTTON_CHANGE\r\n");
        hw_btn_set_val_button(cmd->data.send.button_change.button_data);
    }
}