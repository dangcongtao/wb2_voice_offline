#include "m_mqtt.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>

#include "cJSON.h"
#include "aes_cbc128.h"
#include "system_typedef.h"

extern char key[16], iv[16];

static m_mqtt_t s_mqtt_obj;
static QueueHandle_t mqtt_tx_queue;
static bool request_send_command_online = false;

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

static bool mqtt_init_queue(void)
{
    if (mqtt_tx_queue == NULL) {
        mqtt_tx_queue = xQueueCreate(MAXIMUM_QUEUE_ITEM, sizeof(m_mqtt_msg_pub_t));
        if(mqtt_tx_queue == NULL){
            os_trace("%s fail\r\n", __func__);
            return false;
        }
    }

    return true;
}

static void m_mqtt_publish(char *topic, char* data, uint32_t length)
{
    if(data == NULL)
        return;

    if(topic == NULL)
        topic = s_mqtt_obj.mqtt_info.topic_pub;

    m_mqtt_msg_pub_t message;
	memset(&message, 0, sizeof(message));
	memcpy(message.mqtt_topic_pub, topic, strlen(topic));
	memcpy(message.mqtt_message_pub, data, length);
	message.length = length;

	if (xQueueSend(mqtt_tx_queue, &message, pdMS_TO_TICKS(0)) != pdPASS) {
        os_trace("Push to queue tx fail\r\n");
    }
}

static m_mqtt_state_t m_mqtt_get_state(void)
{
    return s_mqtt_obj._mqtt_state;
}

static void mqtt_on_message(char* data, uint32_t len)
{
    char temp_data[MAXIMUM_SIZE_MESSAGE];
    uint32_t data_len;
    uint32_t m_message_real_len = 0;
    char m_message_real[MAXIMUM_SIZE_MESSAGE];

    /* decode data */
    data_len = len;
    memset(temp_data, 0, sizeof(temp_data));
    memcpy(temp_data, data, (MAXIMUM_SIZE_MESSAGE > data_len ? data_len : MAXIMUM_SIZE_MESSAGE));

    memset(m_message_real, 0, sizeof(m_message_real));
    if (temp_data[0] != '{') {
        aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
        if (aes_cbc128_decrypt((uint8_t *)temp_data, data_len, 
                            (uint8_t *)m_message_real, 
                            (size_t *)&m_message_real_len) != AES_SUCCESS) {
            os_trace("Can not decrypt message\r\n");
            return;
        }
    } else {
        memcpy(m_message_real, temp_data, data_len);
        m_message_real_len = data_len;
    }

    os_trace("mqtt received: %s\r\n", m_message_real);
    s_mqtt_obj._mqtt_handler_msg(m_message_real, m_message_real_len);
}

static void m_mqtt_start(void)
{
    os_trace("mqtt start\r\n");
    os_trace("\tserver: %s\r\n", s_mqtt_obj.mqtt_info.server);
    os_trace("\tport: %ld\r\n", s_mqtt_obj.mqtt_info.port);
    os_trace("\tusername: %s\r\n", s_mqtt_obj.mqtt_info.username);
    os_trace("\tpasswd: %s\r\n", s_mqtt_obj.mqtt_info.passwd);
    os_trace("\ttopic_pub: %s\r\n", s_mqtt_obj.mqtt_info.topic_pub);
    os_trace("\ttopic_sub: %s\r\n", s_mqtt_obj.mqtt_info.topic_sub);
    s_mqtt_obj._mqtt_state = MQTT_INTIALIZED;
    request_send_command_online = true;
}

static void m_mqtt_stop(void)
{
    os_trace("mqtt stop\r\n");
}

static void m_mqtt_publish_loop(void)
{
	if (mqtt_tx_queue == NULL) {
        os_trace("Queue pub null");
        return;
    }
	
    m_mqtt_msg_pub_t mqtt_msg_pub;
    char msg_encrypted[MAXIMUM_SIZE_MESSAGE];
    uint32_t len_msg_encrypted = MAXIMUM_SIZE_MESSAGE;

    if (xQueueReceive(mqtt_tx_queue, &mqtt_msg_pub, 1) == pdPASS) {

        /* Encode AES */
        memset(msg_encrypted, 0, sizeof(msg_encrypted));
        aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
        uint32_t aes_encrypt_result = aes_cbc128_encrypt((uint8_t *)mqtt_msg_pub.mqtt_message_pub,
                                                        mqtt_msg_pub.length, 
                                                        (uint8_t*)msg_encrypted, 
                                                        (size_t *)&len_msg_encrypted);
        if (aes_encrypt_result != AES_SUCCESS) {
            os_trace("Encode fail,error-code: %ld\r\n", aes_encrypt_result);
            return;
        }

        /* call platform mqtt-publish api */
    }
}

static void m_mqtt_task_loop(void)
{
    if (request_send_command_online && s_mqtt_obj._mqtt_pub_startup_msg != NULL) {
        s_mqtt_obj._mqtt_pub_startup_msg();
        request_send_command_online = 0;
    }

    m_mqtt_publish_loop();

    vTaskDelay(pdMS_TO_TICKS(10));
}

void m_mqtt_init(void)
{
    os_trace("m_mqtt_init\r\n");

    s_mqtt_obj._mqtt_state = MQTT_UNINTIALIZED;
    s_mqtt_obj._mqtt_publish = m_mqtt_publish;
    s_mqtt_obj._mqtt_start = m_mqtt_start;
    s_mqtt_obj._mqtt_stop = m_mqtt_stop;
    s_mqtt_obj._mqtt_loop = m_mqtt_task_loop;
    s_mqtt_obj._mqtt_get_state_cb = m_mqtt_get_state;

    s_mqtt_obj.set_mqtt_server = set_mqtt_server;
    s_mqtt_obj.set_mqtt_port = set_mqtt_port;
    s_mqtt_obj.set_mqtt_user = set_mqtt_user;
    s_mqtt_obj.set_mqtt_passwd = set_mqtt_passwd;
    s_mqtt_obj.set_mqtt_topic_sub = set_mqtt_topic_sub;
    s_mqtt_obj.set_mqtt_topic_pub = set_mqtt_topic_pub;

    /* inti queue */
    mqtt_init_queue();
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