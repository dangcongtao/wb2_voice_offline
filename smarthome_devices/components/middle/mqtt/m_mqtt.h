#ifndef _M_MQTT_
#define _M_MQTT_

#include <stdint.h>
#include <stdbool.h>

#define MAXIMUM_SIZE_MQTT_TOPIC                         324
#define MAXIMUM_SIZE_MESSAGE							2048
#define MAXIMUM_QUEUE_ITEM                              4
#define MAX_NUMBER_MQTT_BROKER  5

typedef struct {
	// char topic_pub[MAXIMUM_SIZE_MQTT_TOPIC];
	char msg_pub[MAXIMUM_SIZE_MESSAGE];
	uint32_t length;
} m_mqtt_msg_t; /* message to publish - tx */

typedef struct
{
    char mqtt_message_sub[MAXIMUM_SIZE_MESSAGE];
    uint32_t length;
} m_mqtt_msg_sub_t;


typedef enum {
    MQTT_UNINTIALIZED = 0,
    MQTT_INTIALIZED,
    MQTT_SUBSCRIBE,
    MQTT_UNSUBSUBSCRIBE,
} m_mqtt_state_t;

typedef void (*p_set_data_t)(char* data, uint32_t len);
typedef void (*p_set_num_t)(uint32_t);

typedef void (*p_mqtt_publish_t)(char* data, uint32_t data_len);
typedef void (*p_mqtt_publish_directly_t)(char* data, uint32_t data_len);
typedef void (*p_mqtt_subscribe_t)(char *topic, uint32_t topic_len);
typedef void (*p_mqtt_unsubscribe_t)(char *topic, uint32_t topic_len);
typedef void (*p_mqtt_connect_t)(char *topic, uint32_t topic_len);
typedef void (*p_mqtt_disconnect_t)(char *topic, uint32_t topic_len);
typedef void (*p_mqtt_start_t)(void);
typedef void (*p_mqtt_stop_t)(void);
typedef void (*p_mqtt_task_t)(void);

typedef void (*p_mqtt_handler_msg_t)(char *data, uint32_t topic_len);
typedef void (*p_mqtt_publish_startup_msg_t)(void);

typedef m_mqtt_state_t (*p_mqtt_get_state_t)(void);

typedef struct {
    /* properties */
    char topic_sub[MAXIMUM_SIZE_MQTT_TOPIC];
    char topic_pub[MAXIMUM_SIZE_MQTT_TOPIC];
    char server[32];
    uint32_t port;
    char username[32];
    char passwd[32];
} m_mqtt_info_old_t;

typedef struct {
    /* properties */
    uint8_t type; // 1 is use uri, 0: use IP addr
    char server[32];
    uint32_t port;
    char username[32];
    char passwd[32];
} m_mqtt_server_t;

typedef struct {
    /* properties */
    uint32_t vermqtt;
    uint8_t broker_in_used;
    uint8_t num_broker;
    char topic_sub[MAXIMUM_SIZE_MQTT_TOPIC];
    char topic_pub[MAXIMUM_SIZE_MQTT_TOPIC];
    m_mqtt_server_t servers[MAX_NUMBER_MQTT_BROKER];
} m_mqtt_info_t;

typedef struct  {
/* properties */
    m_mqtt_info_t mqtt_info;
    m_mqtt_state_t _mqtt_state;

/* public methods */
    p_mqtt_publish_t    _mqtt_publish;
    p_mqtt_publish_directly_t _mqtt_publish_directly;
    p_mqtt_start_t      _mqtt_start;
    p_mqtt_stop_t       _mqtt_stop;
    p_mqtt_task_t       _mqtt_loop;
    p_mqtt_get_state_t  _mqtt_get_state_cb;

    void (*set_server_info)(int index, m_mqtt_server_t *p_server_info);
    m_mqtt_server_t* (*get_server_info)(void);
    p_set_data_t set_mqtt_topic_sub;
    p_set_data_t set_mqtt_topic_pub;
    int (*update_mqtt_broker)(void);
    void (*set_num_mqtt_broker)(int num);
    uint32_t (*get_num_mqtt_broker)(void);
    void (*set_ver_mqtt)(uint32_t version);

/* private method */
    p_mqtt_handler_msg_t _mqtt_handler_msg;
    p_mqtt_publish_startup_msg_t _mqtt_pub_startup_msg;
} m_mqtt_t;

/* App have to call this api before start mqtt */
void m_mqtt_register_msg_handler(p_mqtt_handler_msg_t handler);
void m_mqtt_command_startup_register_handler(p_mqtt_publish_startup_msg_t handler);

void m_mqtt_init(void);
void m_mqtt_stop(void);
m_mqtt_t m_mqtt_get_instance(void);
m_mqtt_info_t* m_mqtt_get_info_reference(void);

#endif /* _M_MQTT_ */