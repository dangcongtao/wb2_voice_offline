#ifndef _APP_IR_H_
#define _APP_IR_H_

#include <stdint.h>

#define REQUEST_SEND_REMOTE_BRAND 1
#define REQUEST_SEND_IR_FAN 2

#define REQUEST_SEND_LEARN_IR 1
#define REQUEST_SEND_LEARN_AUTO 2

typedef struct
{
    double degrees;
    uint8_t power;
    int8_t mode;
    uint8_t fanspeed;
    int8_t swingv;
    int8_t swingh;
} data_ir_ac_t;

typedef struct
{
    uint64_t value;
} data_ir_remote_t;

typedef union
{
    data_ir_ac_t ir_ac;
    data_ir_remote_t remote;
} u_data_remote_t;

typedef struct
{
    uint8_t request_send;
    uint8_t request_send_learn;
    uint8_t model_learn;
    uint8_t decode_type;
    int8_t model;
    u_data_remote_t data;
} response_remote_data_t;

void app_ir_send_response_from_remote_brand_check_loop(void);
#endif // _APP_IR_H_