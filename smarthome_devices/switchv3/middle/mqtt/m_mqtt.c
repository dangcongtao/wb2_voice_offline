#include "m_mqtt.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "cJSON.h"
#include "aes_cbc128.h"

extern char key[16], iv[16];
static const char *TAG = "MQTT_EXAMPLE";

static char m_message_real[MAXIMUM_SIZE_MESSAGE];
static uint32_t m_message_real_len = 0;
static bool request_send_command_online = false;
static QueueHandle_t xQueueMQTT_Msg_Tx;
static m_mqtt_t s_mqtt_obj;

static bool mqtt_init_queue(void)
{
    /* init queue mqtt tx */
    if (xQueueMQTT_Msg_Tx == NULL) {
        xQueueMQTT_Msg_Tx = xQueueCreate(MAXIMUM_QUEUE_ITEM, sizeof(m_mqtt_msg_pub_t));
        if(xQueueMQTT_Msg_Tx == NULL){
            os_trace("Cannot create queue send at command\r\n");
            return false;
        }
    }
    
    os_trace("Init queue success\n");

    return true;
}

static void m_mqtt_publish(char *topic, char* data, uint32_t data_len)
{
    if(mqtt_topic == NULL || mqtt_message == NULL)
        return;

    m_mqtt_msg_pub_t message;
	memset(&message, 0, sizeof(message));
	memcpy(message.mqtt_topic_pub, mqtt_topic, strlen(mqtt_topic));
	memcpy(message.mqtt_message_pub, mqtt_message, length);
	message.length = length;

	if (xQueueSend(xQueueMQTT_Msg_Tx, &message, pdMS_TO_TICKS(0)) != pdPASS) {
        os_trace("Push to queue tx fail\n");
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
            os_trace("Can not decrypt message\n");
            return;
        }
    } else {
        memcpy(m_message_real, temp_data, data_len);
        m_message_real_len = data_len;
    }

    os_trace("mqtt received: %s\n", m_message_real);
    s_mqtt_obj._mqtt_handler_msg(m_message_real, m_message_real_len);
}

static void m_mqtt_start(void)
{
    os_trace("mqtt start\n");
    os_trace("\tserver: %s\n", s_mqtt_obj.mqtt_info->server);
    os_trace("\tport: %d\n", s_mqtt_obj.mqtt_info->port);
    os_trace("\tusername: %s\n", s_mqtt_obj.mqtt_info->username);
    os_trace("\tpasswd: %s\n", s_mqtt_obj.mqtt_info->passwd);
    os_trace("\ttopic_pub: %s\n", s_mqtt_obj.mqtt_info->topic_pub);
    os_trace("\ttopic_sub: %s\n", s_mqtt_obj.mqtt_info->topic_sub);
    s_mqtt_obj._mqtt_state = MQTT_INTIALIZED;
}

static void m_mqtt_stop(void)
{
    os_trace("\tmqtt stop\n");
}

static void m_mqtt_publish_loop(void)
{
	if (xQueueMQTT_Msg_Tx == NULL) {
        os_trace("Queue pub null");
        return;
    }
	
    m_mqtt_msg_pub_t mqtt_msg_pub;

    if (xQueueReceive(xQueueMQTT_Msg_Tx, &mqtt_msg_pub, 1) == pdPASS) {
        char msg_to_pub[MAXIMUM_SIZE_MESSAGE];
        uint32_t len_to_pub = MAXIMUM_SIZE_MESSAGE;

        /* Encode AES */
        memset(msg_to_pub, 0, sizeof(msg_to_pub));
        aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
        uint32_t aes_encrypt_result = aes_cbc128_encrypt((uint8_t *)mqtt_msg_pub.mqtt_message_pub,
                                                            mqtt_msg_pub.length, 
                                                            (uint8_t*)msg_to_pub, 
                                                            (size_t *)&len_to_pub);
        if (aes_encrypt_result != AES_SUCCESS) {
            os_trace("Encode fail,error-code: %d\n", aes_encrypt_result);
            return;
        }

        /* call platform mqtt-publish api */
    }
}

static void m_mqtt_task_loop(void)
{
    if (request_send_command_online) {
        s_mqtt_obj._mqtt_pub_startup_msg();
        request_send_command_online = 0;
    }

    m_mqtt_publish_loop();

    vTaskDelay(pdMS_TO_TICKS(10));
}

void m_mqtt_init(m_mqtt_info_t mqtt)
{
    os_trace("m_mqtt_init\n");

    s_mqtt_obj.mqtt_info = mqtt;
    s_mqtt_obj._mqtt_state = MQTT_UNINTIALIZED;
    s_mqtt_obj._mqtt_publish = m_mqtt_publish;
    s_mqtt_obj._mqtt_start = m_mqtt_start;
    s_mqtt_obj._mqtt_stop = m_mqtt_stop;
    s_mqtt_obj._mqtt_loop = m_mqtt_task_loop;
    s_mqtt_obj._mqtt_get_state_cb = m_mqtt_get_state;

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

m_mqtt_t* m_mqtt_get_instance(void)
{
    return &s_mqtt_obj;
}
