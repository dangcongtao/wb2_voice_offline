#include "m_mqtt.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>

#include <mqtt_client.h>

#include "cJSON.h"
#include "aes_cbc128.h"
#include "system_typedef.h"

m_mqtt_t s_mqtt_obj;
// QueueHandle_t mqtt_tx_queue;
bool request_send_command_online;

extern void m_mqtt_stop(void);
extern void m_mqtt_start(void);
extern void m_mqtt_publish(char *data, uint32_t length);
extern void m_mqtt_publish_directly(char *data, uint32_t length);
extern void m_mqtt_task_loop(void);

void set_mqtt_server(char* data, uint32_t len)
{
    if (len > sizeof(s_mqtt_obj.mqtt_info.server)) {
        os_trace("fail to set mqtt server, len data too long\r\n");
        return;
    }

    memset(s_mqtt_obj.mqtt_info.server, 0, sizeof(s_mqtt_obj.mqtt_info.server));
    memcpy(s_mqtt_obj.mqtt_info.server, data, len);
}

void set_mqtt_port(uint32_t num)
{
    s_mqtt_obj.mqtt_info.port = num;
}

void set_mqtt_user(char* data, uint32_t len)
{
    if (len > sizeof(s_mqtt_obj.mqtt_info.username)) {
        os_trace("fail to set mqtt user, len data too long\r\n");
        return;
    }

    memset(s_mqtt_obj.mqtt_info.username, 0, sizeof(s_mqtt_obj.mqtt_info.username));
    memcpy(s_mqtt_obj.mqtt_info.username, data, len);
}

void set_mqtt_passwd(char* data, uint32_t len)
{
    if (len > sizeof(s_mqtt_obj.mqtt_info.passwd)) {
        os_trace("fail to set mqtt passwd, len data too long\r\n");
        return;
    }

    memset(s_mqtt_obj.mqtt_info.passwd, 0, sizeof(s_mqtt_obj.mqtt_info.passwd));
    memcpy(s_mqtt_obj.mqtt_info.passwd, data, len);
}

void set_mqtt_topic_sub(char* data, uint32_t len)
{
    if (len > sizeof(s_mqtt_obj.mqtt_info.topic_sub)) {
        os_trace("fail to set mqtt topic_sub, len data too long\r\n");
        return;
    }

    memset(s_mqtt_obj.mqtt_info.topic_sub, 0, sizeof(s_mqtt_obj.mqtt_info.topic_sub));
    memcpy(s_mqtt_obj.mqtt_info.topic_sub, data, len);
}

void set_mqtt_topic_pub(char* data, uint32_t len)
{
    if (len > sizeof(s_mqtt_obj.mqtt_info.topic_pub)) {
        os_trace("fail to set mqtt topic_sub, len data too long\r\n");
        return;
    }

    memset(s_mqtt_obj.mqtt_info.topic_pub, 0, sizeof(s_mqtt_obj.mqtt_info.topic_pub));
    memcpy(s_mqtt_obj.mqtt_info.topic_pub, data, len);
}

bool mqtt_init_queue(void)
{
    // if (mqtt_tx_queue == NULL) {
    //     mqtt_tx_queue = xQueueCreate(MAXIMUM_QUEUE_ITEM, sizeof(m_mqtt_msg_t));
    //     if(mqtt_tx_queue == NULL){
    //         os_trace("%s fail\r\n", __func__);
    //         return false;
    //     }
    // }

    return true;
}

static m_mqtt_state_t m_mqtt_get_state(void)
{
    return s_mqtt_obj._mqtt_state;
}


void m_mqtt_init(void)
{
    os_trace("m_mqtt_init\r\n");

    s_mqtt_obj._mqtt_state = MQTT_UNINTIALIZED;
    s_mqtt_obj._mqtt_get_state_cb = m_mqtt_get_state;

    s_mqtt_obj._mqtt_publish = m_mqtt_publish;
    s_mqtt_obj._mqtt_publish_directly = m_mqtt_publish_directly;
    s_mqtt_obj._mqtt_start = m_mqtt_start;
    s_mqtt_obj._mqtt_stop = m_mqtt_stop;
    s_mqtt_obj._mqtt_loop = m_mqtt_task_loop;

    s_mqtt_obj.set_mqtt_server = set_mqtt_server;
    s_mqtt_obj.set_mqtt_port = set_mqtt_port;
    s_mqtt_obj.set_mqtt_user = set_mqtt_user;
    s_mqtt_obj.set_mqtt_passwd = set_mqtt_passwd;
    s_mqtt_obj.set_mqtt_topic_sub = set_mqtt_topic_sub;
    s_mqtt_obj.set_mqtt_topic_pub = set_mqtt_topic_pub;

    /* inti queue */
    mqtt_init_queue();

    request_send_command_online = true;
}

/* have to call this api befor start mqtt */
void m_mqtt_register_msg_handler(p_mqtt_handler_msg_t handler)
{
    s_mqtt_obj._mqtt_handler_msg = handler;
}

/* have to call this api befor start mqtt */
void m_mqtt_command_startup_register_handler(p_mqtt_publish_startup_msg_t handler)
{
    s_mqtt_obj._mqtt_pub_startup_msg = handler;
}

m_mqtt_t m_mqtt_get_instance(void)
{
    return s_mqtt_obj;
}

m_mqtt_info_t* m_mqtt_get_info_reference(void)
{
    return &(s_mqtt_obj.mqtt_info);
}