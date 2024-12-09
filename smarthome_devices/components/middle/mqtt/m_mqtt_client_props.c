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
static uint8_t num_broker_change = 0;

extern void m_mqtt_stop(void);
extern void m_mqtt_start(void);
extern void m_mqtt_publish(char *data, uint32_t length);
extern void m_mqtt_publish_directly(char *data, uint32_t length);
extern void m_mqtt_task_loop(void);

static void set_mqtt_server_info(int index, m_mqtt_server_t *p_server_info)
{
    if (index >= MAX_NUMBER_MQTT_BROKER) {
        os_trace("fail to set mqtt server, len data too long\r\n");
        return;
    }
    num_broker_change = 0;
    memset(&s_mqtt_obj.mqtt_info.servers[index], 0, sizeof(m_mqtt_server_t));
    memcpy(&s_mqtt_obj.mqtt_info.servers[index], p_server_info, sizeof(m_mqtt_server_t));
}

static m_mqtt_server_t* get_mqtt_server_info(void)
{
    if (num_broker_change >= MAX_NUMBER_MQTT_BROKER) {
        os_trace("fail to set mqtt server, out of range %u\r\n", s_mqtt_obj.mqtt_info.broker_in_used);
        return NULL;
    }
    return &s_mqtt_obj.mqtt_info.servers[s_mqtt_obj.mqtt_info.broker_in_used];
}

static void set_mqtt_topic_sub(char* data, uint32_t len)
{
    if (len > sizeof(s_mqtt_obj.mqtt_info.topic_sub)) {
        os_trace("fail to set mqtt topic_sub, len data too long\r\n");
        return;
    }

    memset(s_mqtt_obj.mqtt_info.topic_sub, 0, sizeof(s_mqtt_obj.mqtt_info.topic_sub));
    memcpy(s_mqtt_obj.mqtt_info.topic_sub, data, len);
}

static void set_mqtt_topic_pub(char* data, uint32_t len)
{
    if (len > sizeof(s_mqtt_obj.mqtt_info.topic_pub)) {
        os_trace("fail to set mqtt topic_sub, len data too long\r\n");
        return;
    }

    memset(s_mqtt_obj.mqtt_info.topic_pub, 0, sizeof(s_mqtt_obj.mqtt_info.topic_pub));
    memcpy(s_mqtt_obj.mqtt_info.topic_pub, data, len);
}


static void set_num_mqtt_broker(int num)
{
    s_mqtt_obj.mqtt_info.num_broker = num;
}

static uint32_t get_num_mqtt_broker(void)
{
    return s_mqtt_obj.mqtt_info.num_broker;
}

static void set_ver_mqtt(uint32_t version)
{
    s_mqtt_obj.mqtt_info.vermqtt = version;
}

static int update_mqtt_broker(void)
{
    uint32_t new_index = (s_mqtt_obj.mqtt_info.broker_in_used + 1) % s_mqtt_obj.mqtt_info.num_broker;
    s_mqtt_obj.mqtt_info.broker_in_used = new_index;

    num_broker_change++;
    if (num_broker_change >= s_mqtt_obj.mqtt_info.num_broker)
    {
        // memset(s_mqtt_obj.mqtt_info.servers, 0, sizeof(s_mqtt_obj.mqtt_info.servers));

        // s_mqtt_obj.mqtt_info.num_broker = 0;
        // s_mqtt_obj.mqtt_info.broker_in_used = 0;
        // s_mqtt_obj.mqtt_info.vermqtt = 0;

        // num_broker_change = 0;
        return 0;
    }

    return 1;
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

    
    s_mqtt_obj.set_server_info = set_mqtt_server_info;
    s_mqtt_obj.get_server_info = get_mqtt_server_info;
    s_mqtt_obj.set_mqtt_topic_sub = set_mqtt_topic_sub;
    s_mqtt_obj.set_mqtt_topic_pub = set_mqtt_topic_pub;

    s_mqtt_obj.update_mqtt_broker = update_mqtt_broker;
    s_mqtt_obj.set_num_mqtt_broker = set_num_mqtt_broker;
    s_mqtt_obj.get_num_mqtt_broker = get_num_mqtt_broker;
    s_mqtt_obj.set_ver_mqtt = set_ver_mqtt;

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