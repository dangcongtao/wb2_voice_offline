#ifndef _M_FT62F28_H_
#define _M_FT62F28_H_

#include <stdint.h>

#define HEADER_1 0x11
#define HEADER_2 0x22

#define CMD_SEND_BUTTON_CHANGE 0x01
#define CMD_WIFI_SEND_STATE 0x02
#define CMD_RECEIVE_RESET_TOUCH 0x03

// TX
typedef struct
{
    uint8_t speed : 7;
    uint8_t change_speed : 1;
    uint8_t led_data : 6;
    uint8_t change_led : 1;
    uint8_t swing : 1;
    uint8_t change_swing : 1;
    uint8_t ligth : 7;
    uint8_t change_light : 1;
    uint8_t : 7;
} cmd_data_receive_state_t;

typedef struct
{
    uint8_t button_data;
} cmd_data_receive_reset_touch_t;

typedef union
{
    cmd_data_receive_state_t state;
    cmd_data_receive_reset_touch_t touch_reset;
} cmd_data_rcv_u;

// RX
typedef struct
{
    uint8_t button_data;
} cmd_data_send_button_change_t;

typedef union
{
    cmd_data_send_button_change_t button_change;
} cmd_data_send_u;

typedef union
{
    cmd_data_rcv_u rcv;
    cmd_data_send_u send;
} cmd_uart_data_u;

typedef struct
{
    uint8_t header[2];
    uint8_t len_data;
    uint8_t cmd;
    uint8_t checksum;
    cmd_uart_data_u data;
} cmd_uart_t;

void m_ft62f28_send_speed(void);
void m_ft62f28_send_swing(void);
void m_ft62f28_send_led(void);
void m_ft62f28_send_light(void);
void m_ft62f28_check_send_change(void);
void m_ft62f28_receive_data(uint8_t *data, uint32_t len);

#endif // _M_FT62F28_H_