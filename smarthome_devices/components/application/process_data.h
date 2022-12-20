#ifndef _APP_MQTT_H_
#define _APP_MQTT_H_

#include <stdint.h>
#include "cJSON.h"


typedef void (*wswitch_message_handler_t)(cJSON *jsRoot);

typedef struct {
    wswitch_message_handler_t _handler;
    uint32_t                 _cmd_number;
} wswitch_handler_t;


void app_handler_msg_init(void);

#endif /* _APP_MQTT_H_ */
