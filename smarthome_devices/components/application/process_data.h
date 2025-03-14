#ifndef _APP_MQTT_H_
#define _APP_MQTT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "system_typedef.h"
#include "cJSON.h"


typedef void (*wswitch_message_handler_t)(cJSON *jsRoot);
typedef uint8_t (*ir_data_handler_t)(uint8_t data[], uint16_t nbytes);

typedef struct {
    wswitch_message_handler_t _handler;
    uint32_t                 _cmd_number;
} wswitch_handler_t;
typedef struct {
    ir_data_handler_t _handler;
    uint8_t                 _cmd_number;
} irwifi_data_brand_handler_t;


void process_data_init(void);
void app_handler_msg_loop(void);
void app_send_data(char *data, uint32_t length);

#endif /* _APP_MQTT_H_ */
