#include "hw_uart.h"
#include "m_ft62f28.h"
#include "m_fan.h"
#include "hardware.h"
#include "m_fan_control.h"

typedef struct
{
    uint8_t has_control_fan : 1;
    uint8_t flag_send_speed : 1;
    uint8_t flag_send_swing : 1;
    uint8_t flag_send_led : 1;
    uint8_t flag_send_light : 1;
    uint8_t swing_current_mode;
} m_ft62f28_data_t;

static m_ft62f28_data_t s_ft62f28_obj;

cmd_uart_t data_send;

void m_ft62f28_send_speed(void)
{
    m_fan_info_t *fan_info = m_fan_get_instance();
    if (fan_info->mode != M_FAN_MODE_PCN)
    {
        switch (fan_info->speed)
        {
        case 1:
            fan_info->pcn = 33;
            break;
        case 2:
            fan_info->pcn = 66;
            break;
        case 3:
            fan_info->pcn = 100;
            break;
        case 0:
            fan_info->pcn = 0;
            break;
        default:
            fan_info->pcn = 0;
            break;
        }
    }
    s_ft62f28_obj.has_control_fan = 1;
    s_ft62f28_obj.flag_send_speed = 1;
}

void m_ft62f28_send_swing(uint8_t swing_mode)
{
    s_ft62f28_obj.swing_current_mode = swing_mode;
    s_ft62f28_obj.has_control_fan = 1;
    s_ft62f28_obj.flag_send_swing = 1;
}

void m_ft62f28_send_light(void)
{
    s_ft62f28_obj.has_control_fan = 1;
    s_ft62f28_obj.flag_send_light = 1;
}

void m_ft62f28_send_led(void)
{
    s_ft62f28_obj.has_control_fan = 1;
    s_ft62f28_obj.flag_send_led = 1;
}

void m_ft62f28_send_reset_touch(void)
{
    os_trace("m_ft62f28_send_reset_touch");
    memset(&data_send, 0, sizeof(data_send));
    data_send.header[0] = HEADER_1;
    data_send.header[1] = HEADER_2;
    data_send.cmd = CMD_RECEIVE_RESET_TOUCH;
    data_send.len_data = 2 + sizeof(cmd_data_rcv_u);

    hw_uart_get_instance()->send_bytes((char *)&data_send, 3 + data_send.len_data);
}

void m_ft62f28_check_send_change(void)
{
    if (!s_ft62f28_obj.has_control_fan)
    {
        return;
    }
    m_fan_info_t *fan_info = m_fan_get_instance();
    s_ft62f28_obj.has_control_fan = 0;
    data_send.header[0] = HEADER_1;
    data_send.header[1] = HEADER_2;
    data_send.cmd = CMD_WIFI_SEND_STATE;

    if (s_ft62f28_obj.flag_send_speed)
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

        s_ft62f28_obj.flag_send_speed = 0;
    }
    else
    {
        data_send.data.rcv.state.change_speed = 0;
    }

    if (s_ft62f28_obj.flag_send_swing)
    {
        data_send.data.rcv.state.change_swing = 1;
        data_send.data.rcv.state.swing = s_ft62f28_obj.swing_current_mode;
        s_ft62f28_obj.flag_send_swing = 0;
    }
    else
    {
        data_send.data.rcv.state.change_swing = 0;
    }

    if (s_ft62f28_obj.flag_send_led)
    {
        // os_trace("send_led: %u\r\n", data_send.data.rcv.state.led_data);
        data_send.data.rcv.state.change_led = 1;
        data_send.data.rcv.state.led_data = m_fan_get_led_stt();
        s_ft62f28_obj.flag_send_led = 0;
    }
    else
    {
        data_send.data.rcv.state.change_led = 0;
    }

    if (s_ft62f28_obj.flag_send_light)
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
        s_ft62f28_obj.flag_send_light = 0;
    }
    else
    {
        data_send.data.rcv.state.change_light = 0;
    }

    os_trace("send_speed: pcn %u, swing: %u, light %u, led %u\r\n", data_send.data.rcv.state.speed, data_send.data.rcv.state.change_swing, data_send.data.rcv.state.ligth, data_send.data.rcv.state.led_data);

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