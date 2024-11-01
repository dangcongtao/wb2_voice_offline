#ifndef _LED_DRV_H_
#define _LED_DRV_H_

#include <stdint.h>


/********************** New handler ***********************/
typedef enum {
   LED_STYLE_UNDEFINED = -1,
   LED_STYLE_NOT_CONFIGURED = 0,
   LED_STYLE_NORMAL = 1,
   LED_STYLE_SMARTCONFIG = 2,
   LED_STYLE_APCONFIG = 3,
   LED_STYLE_ON = 4,
   LED_STYLE_OFF = 5,
   LED_STYLE_UPDATE = 6,
   LED_STYLE_WIFI_DISCONN = 7,
   LED_STYLE_MQTT_DISCONN,
} led_stt_type_t;

typedef void (*p_set_led_stt_t)(void);
typedef void (*p_reset_led_stt_t)(void);
typedef void (*p_toggle_led_stt_t)(void);
typedef void (*p_init_led_stt_t)(void);

typedef void (*p_led_stt_handler_t)(void);

typedef struct {
/* properties */
   uint8_t                    _led_handle_count;
   uint8_t                    _max_number_handle;
   led_stt_type_t             _led_stt_type;

/* private */
   p_init_led_stt_t           _init_led_stt_cb;
   p_set_led_stt_t            _set_led_stt_on_cb;
   p_reset_led_stt_t          _set_led_stt_off_cb;
   p_toggle_led_stt_t         _toggle_led_stt_cb;

/* public */
   p_led_stt_handler_t        _set_led_stt_wifi_not_connect_cb;
   p_led_stt_handler_t        _set_led_stt_ap_config_cb;
   p_led_stt_handler_t        _set_led_stt_smart_config_cb;
   p_led_stt_handler_t        _set_led_stt_update_ota_cb;
   p_led_stt_handler_t        _set_led_stt_mqtt_disconnect_cb;
   p_led_stt_handler_t        _set_led_stt_mqtt_subscribed_cb;

} m_led_stt_t;

/* need call from hardware at start-up */
void m_led_stt_init(void);

m_led_stt_t m_led_stt_get_instance(void);

#endif /* _LED_DRV_H_ */