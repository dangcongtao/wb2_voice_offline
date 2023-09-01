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
#include "blog.h"
#include "bl_sys.h"

#include "cJSON.h"
#include "aes_cbc128.h"
#include "system_typedef.h"
#include "m_led_stt.h"
#include "m_database.h"

extern char key[16], iv[16];
extern m_mqtt_t s_mqtt_obj;
extern QueueHandle_t mqtt_tx_queue;
extern bool request_send_command_online;

axk_mqtt_client_handle_t client = NULL;

extern void signal_post(m_signal_t sig);
extern void push_data_to_pqueue(m_data_t *message);

static void mqtt_on_message(char *data, uint32_t len);

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        blog_error("Last error %s: 0x%x\r\n", message, error_code);
    }
}

static axk_err_t mqtt_event_handler_cb(axk_mqtt_event_handle_t event)
{
    client = event->client;

    blog_debug("Event dispatched, event_id=%d\r\n", event_id);

    switch ((axk_mqtt_event_id_t)event->event_id)
    {

    case MQTT_EVENT_CONNECTED:
        os_trace("MQTT_EVENT_CONNECTED\r\n");
        axk_mqtt_client_subscribe(client, s_mqtt_obj.mqtt_info.topic_sub, 0);
        signal_post(SIG_MQTT_CONNECTED);
        break;

    case MQTT_EVENT_DISCONNECTED:
        os_trace("MQTT_EVENT_DISCONNECTED\r\n");
        m_led_stt_get_instance()._set_led_stt_mqtt_disconnect_cb();
        signal_post(SIG_MQTT_DISCONNECTED);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        os_trace("MQTT_EVENT_SUBSCRIBED, msg_id=%d\r\n", event->msg_id);
        m_led_stt_get_instance()._set_led_stt_mqtt_subscribed_cb();
        request_send_command_online = true;
        s_mqtt_obj._mqtt_state = MQTT_SUBSCRIBE;
        break;

    case MQTT_EVENT_DATA:
        os_trace("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        // os_trace("MQTT Data[%u]: [%s]\r\n", event->data_len, event->data);
        // mqtt_on_message(event->data, event->data_len);

        if (event->data_len < DATA_MAX_LEN)
        {
            m_data_t message;
            memset(&message, 0, sizeof(message));
            memcpy(message.data, event->data, event->data_len);
            message.length = event->data_len;
            message.method_trasmit = MQTT_MSG;
            push_data_to_pqueue(&message);
        }
        break;

    case MQTT_EVENT_ERROR:
        os_trace("MQTT_EVENT_ERROR\r\n");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from axk-tls", event->error_handle->axk_tls_last_axk_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->axk_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->axk_transport_sock_errno);
            os_trace("Last errno string (%s)", strerror(event->error_handle->axk_transport_sock_errno));
        }
        break;

    default:
        os_trace("Other event id:%d", event->event_id);
        break;
    }
    return AXK_OK;
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

void m_mqtt_publish_directly(char *data, uint32_t length)
{
    char *topic = s_mqtt_obj.mqtt_info.topic_pub;

    m_mqtt_msg_t message;
    memset(&message, 0, sizeof(message));
    memcpy(message.topic_pub, topic, strlen(topic));
    memcpy(message.msg_pub, data, length);
    message.length = length;

    /* encode data */
    char msg_encrypted[MAXIMUM_SIZE_MESSAGE];
    uint32_t len_msg_encrypted = MAXIMUM_SIZE_MESSAGE;
    memset(msg_encrypted, 0, sizeof(msg_encrypted));
    aes_cbc128_init((uint8_t *)key, (uint8_t *)iv);
    uint32_t aes_encrypt_result = aes_cbc128_encrypt((uint8_t *)message.msg_pub,
                                                     message.length,
                                                     (uint8_t *)msg_encrypted,
                                                     (size_t *)&len_msg_encrypted);
    if (aes_encrypt_result != AES_SUCCESS)
    {
        os_trace("Encode fail,error-code: %ld\r\n", aes_encrypt_result);
        return;
    }

    /* call platform mqtt-publish api */
    if (client != NULL)
    {
        if (axk_mqtt_client_publish(client, message.topic_pub, msg_encrypted, len_msg_encrypted, 0, 0) < 0)
        {
            os_trace("\tmqtt_client: publish message fail\r\n");
        }
    }
}

void m_mqtt_task_loop(void)
{
    /* send message after online */
    if (request_send_command_online && s_mqtt_obj._mqtt_pub_startup_msg != NULL)
    {
        s_mqtt_obj._mqtt_pub_startup_msg();
        request_send_command_online = 0;
    }

    m_mqtt_msg_t mqtt_msg_pub;
    char msg_encrypted[MAXIMUM_SIZE_MESSAGE];
    uint32_t len_msg_encrypted = MAXIMUM_SIZE_MESSAGE;

    /* send message during activity */
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
        if (client != NULL)
        {
            if (axk_mqtt_client_publish(client, s_mqtt_obj.mqtt_info.topic_pub, msg_encrypted, len_msg_encrypted, 0, 0) < 0)
            {
                os_trace("\tmqtt_client: publish message fail\r\n");
            }
        }
    }
}

void m_mqtt_start(void)
{
    os_trace("%s: free mem %d\r\n", __func__, xPortGetFreeHeapSize());

    char last_will_msg[128];
    memset(last_will_msg, 0, sizeof(last_will_msg));
    sprintf(last_will_msg, "{\"%s\":1000,\"offline\":\"device offline\", \"src\": 0}", gs_dev_type[device_type]);

    axk_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg, 0, sizeof(mqtt_cfg));
    mqtt_cfg.uri = s_mqtt_obj.mqtt_info.server;
    mqtt_cfg.username = s_mqtt_obj.mqtt_info.username;
    mqtt_cfg.password = s_mqtt_obj.mqtt_info.passwd;
    mqtt_cfg.port = s_mqtt_obj.mqtt_info.port;
    mqtt_cfg.keepalive = 10;
    mqtt_cfg.lwt_msg = last_will_msg;
    mqtt_cfg.lwt_msg_len = strlen(last_will_msg);
    mqtt_cfg.lwt_topic = s_mqtt_obj.mqtt_info.topic_pub;
    mqtt_cfg.lwt_qos = 0;
    mqtt_cfg.event_handle = mqtt_event_handler_cb;
    
    char client_id[128];
    memset(client_id, 0, sizeof(client_id));
    sprintf(client_id, "%s_%s_%s", BRAND, gs_dev_type[device_type], m_db_get_instance()->_get_dev_id());
    mqtt_cfg.client_id = client_id;

    os_trace("mqtt start\r\n");
    os_trace("\tserver: %s\r\n", mqtt_cfg.uri);
    os_trace("\tport: %d\r\n", mqtt_cfg.port);
    os_trace("\tusername: %s\r\n", mqtt_cfg.username);
    os_trace("\tpasswd: %s\r\n", mqtt_cfg.password);
    os_trace("\ttopic_pub: %s\r\n", s_mqtt_obj.mqtt_info.topic_pub);
    os_trace("\ttopic_sub: %s\r\n", s_mqtt_obj.mqtt_info.topic_sub);

    if (client != NULL)
    {
        axk_mqtt_client_destroy(client);
        vTaskDelay(1000);
        client = NULL;
    }

    client = axk_mqtt_client_init(&mqtt_cfg);
    if (axk_mqtt_client_start(client) != AXK_OK){
        bl_sys_reset_por();
    }
    s_mqtt_obj._mqtt_state = MQTT_INTIALIZED;

    os_trace("END %s: free mem %d\r\n", __func__, xPortGetFreeHeapSize());
}


void m_mqtt_stop(void)
{
    os_trace("%s: free mem %d\r\n", __func__, xPortGetFreeHeapSize());

    if (client != NULL && s_mqtt_obj._mqtt_state != MQTT_UNINTIALIZED)
    {
        axk_mqtt_client_disconnect(client);
        axk_mqtt_client_destroy(client);
        vTaskDelay(pdMS_TO_TICKS(2000));
        client = NULL;
        s_mqtt_obj._mqtt_state = MQTT_UNINTIALIZED;
    }

    os_trace("END %s: free mem %d\r\n", __func__, xPortGetFreeHeapSize());
}

