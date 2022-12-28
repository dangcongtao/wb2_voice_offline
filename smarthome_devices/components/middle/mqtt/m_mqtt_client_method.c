#include "m_mqtt.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>

#include <stdio.h>
#include <string.h>
#include <blog.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/sockets.h>
#include <lwip/tcpip.h>
#include <cli.h>
#include <hal_wifi.h>
#include <lwip/init.h>
#include <wifi_mgmr_ext.h>
#include "mqtt_client.h"

#include "cJSON.h"
#include "aes_cbc128.h"
#include "system_typedef.h"
#include "m_led_stt.h"
#include "m_database.h"

extern char key[16], iv[16];
extern m_mqtt_t s_mqtt_obj;
extern QueueHandle_t mqtt_tx_queue;
extern bool request_send_command_online;

m_mqtt_msg_t mqtt_msg_pub;
char msg_encrypted[MAXIMUM_SIZE_MESSAGE];
uint32_t len_msg_encrypted = MAXIMUM_SIZE_MESSAGE;

static void mqtt_on_message(char *data, uint32_t len);

void TaskXMqttRecieve(void *p)
{
    mqtt_msg_t rMsg;
    while (1)
    {
        memset(&rMsg, 0, sizeof(rMsg));
        if (mqtt_client_receive(&rMsg))
        {
            switch (rMsg.type)
            {
            // Receive new messages and send them
            case xMQTT_TYPE_RECIEVE_MSG:
                os_trace("xQueueReceive topic: %s\r\n", rMsg.topic);
                os_trace("xQueueReceive msg: %s\r\n", rMsg.payload);
                os_trace("xQueueReceive len: %d\r\n", rMsg.payloadlen);
                mqtt_on_message((char *)rMsg.payload, rMsg.payloadlen);
                break;
            // mqtt client connection succeeded
            case xMQTT_TYPE_CONNECTED:
                memcpy(rMsg.topic, s_mqtt_obj.mqtt_info.topic_sub, strlen(s_mqtt_obj.mqtt_info.topic_sub));
                rMsg.qos = 1;
                mqtt_client_subscribe(&rMsg);
                os_trace("xMQTT_TYPE_CONNECTED: %s\r\n", rMsg.topic);
                break;
            // The mqtt client was disconnected successfully
            case xMQTT_TYPE_DISCONNECTED:
                os_trace("xMQTT_TYPE_DISCONNECTED\r\n");
                break;
            // Connecting to MQTT server
            case xMQTT_TYPE_CONNECTTING:
                os_trace("xMQTT_TYPE_CONNECTTING\r\n");
                break;
            // Topic subscription succeeded
            case xMQTT_TYPE_SUB_SUCCESS:
                os_trace("xMQTT_TYPE_SUB_SUCCESS\r\n");
                request_send_command_online = true;
                m_led_stt_get_instance()._set_led_stt_device_active_cb();
                break;
            // Ping the heartbeat packet
            case xMQTT_TYPE_SEND_PING:
                os_trace("xMQTT_TYPE_SEND_PING\r\n");
                break;
            default:
                break;
            }
        }
    }
}

static void mqtt_on_message(char *data, uint32_t len)
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
    if (temp_data[0] != '{')
    {
    aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
        if (aes_cbc128_decrypt((uint8_t *)temp_data, data_len,
                               (uint8_t *)m_message_real,
                               (size_t *)&m_message_real_len) != AES_SUCCESS)
    {
        os_trace("Can not decrypt message\r\n");
        return;
        }
    }
    else
    {
        memcpy(m_message_real, temp_data, data_len);
        m_message_real_len = data_len;
    }

    os_trace("mqtt received: %s\r\n", m_message_real);
    s_mqtt_obj._mqtt_handler_msg(m_message_real, m_message_real_len);
}

void m_mqtt_publish(char *data, uint32_t length)
{
    char *topic = s_mqtt_obj.mqtt_info.topic_pub;

    m_mqtt_msg_t message;
    memset(&message, 0, sizeof(message));
    memcpy(message.topic_pub, topic, strlen(topic));
    memcpy(message.msg_pub, data, length);
    message.length = length;

    if (xQueueSend(mqtt_tx_queue, &message, pdMS_TO_TICKS(0)) != pdPASS)
    {
        os_trace("Push to queue tx fail\r\n");
    }
}

void m_mqtt_task_loop(void)
{
    if (request_send_command_online && s_mqtt_obj._mqtt_pub_startup_msg != NULL)
    {
        s_mqtt_obj._mqtt_pub_startup_msg();
        request_send_command_online = 0;
    }

    memset(&mqtt_msg_pub, 0, sizeof(mqtt_msg_pub));
    len_msg_encrypted = MAXIMUM_SIZE_MESSAGE;
    if (xQueueReceive(mqtt_tx_queue, &mqtt_msg_pub, 1) == pdPASS)
    {
        memset(msg_encrypted, 0, sizeof(msg_encrypted));
        aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
        uint32_t aes_encrypt_result = aes_cbc128_encrypt((uint8_t *)mqtt_msg_pub.msg_pub,
                                                         mqtt_msg_pub.length,
                                                         (uint8_t *)msg_encrypted,
                                                         (size_t *)&len_msg_encrypted);
        if (aes_encrypt_result != AES_SUCCESS)
        {
            os_trace("Encode fail,error-code: %ld\r\n", aes_encrypt_result);
            return;
        }

        /* call platform mqtt-publish api */
        mqtt_msg_t sMsg;
        memset(&sMsg, 0, sizeof(sMsg));
        memcpy(sMsg.topic, s_mqtt_obj.mqtt_info.topic_pub, strlen(s_mqtt_obj.mqtt_info.topic_pub));
        memcpy(sMsg.payload, msg_encrypted, len_msg_encrypted);
        sMsg.payloadlen = len_msg_encrypted;
        sMsg.qos = 0;
        sMsg.retained = 0;
        sMsg.dup = 0;
        mqtt_client_publish(&sMsg);

        /* message do not encrypted */
        memset(&sMsg, 0, sizeof(sMsg));
        memcpy(sMsg.topic, s_mqtt_obj.mqtt_info.topic_pub, strlen(s_mqtt_obj.mqtt_info.topic_pub));
        memcpy(sMsg.payload, mqtt_msg_pub.msg_pub, len_msg_encrypted);
        sMsg.payloadlen = mqtt_msg_pub.length;
        sMsg.qos = 0;
        sMsg.retained = 0;
        sMsg.dup = 0;
        mqtt_client_publish(&sMsg);
        os_trace("topic: %s\r\n", sMsg.topic);
        os_trace("publish: %s\r\n", mqtt_msg_pub.msg_pub);
    }
}

void m_mqtt_stop(void)
{
    os_trace("mqtt stop\r\n");
}

void m_mqtt_start(void)
{
    os_trace("mqtt start\r\n");

    if (s_mqtt_obj._mqtt_state == MQTT_INTIALIZED)
        return;
    s_mqtt_obj._mqtt_state = MQTT_INTIALIZED;

    char *client_id = m_db_get_instance()->_get_dev_id();
    char *server = s_mqtt_obj.mqtt_info.server;
    server += strlen("mqtt://"); // ignore mqtt:// prefix from mqtt_server

    mqtt_client_config_t xMqttConfig = {
        .MQTTVersion = 4,
        .borkerHost = server,
        .borkerPort = s_mqtt_obj.mqtt_info.port,
        .mqttCommandTimeout = 6000,
        .username = s_mqtt_obj.mqtt_info.username,
        .password = s_mqtt_obj.mqtt_info.passwd,
        .clientID = client_id,
        .keepAliveInterval = 60,
        .cleansession = true,
    };

    os_trace("\tserver: %s\r\n", xMqttConfig.borkerHost);
    os_trace("\tport: %ld\r\n", xMqttConfig.borkerPort);
    os_trace("\tusername: %s\r\n", xMqttConfig.username);
    os_trace("\tpasswd: %s\r\n", xMqttConfig.password);
    os_trace("\ttopic_pub: %s\r\n", s_mqtt_obj.mqtt_info.topic_pub);
    os_trace("\ttopic_sub: %s\r\n", s_mqtt_obj.mqtt_info.topic_sub);

    mqtt_client_init(&xMqttConfig);
    mqtt_client_register_type(TaskXMqttRecieve);

    // Start the connection of MQTT
    xMqttConnectWifiNotify(WIFI_CONNECTED);
    mqtt_client_start();
}
