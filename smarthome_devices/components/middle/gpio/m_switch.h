#ifndef CONTROL_H__
#define CONTROL_H__


#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "system_typedef.h"

#define APP_MAX_CHANNEL_SUPPORT 4

typedef enum {
	APP_SWITCH_ONE = 0,
	APP_SWITCH_TWO = 1,
	APP_SWITCH_THREE = 2,
	APP_SWITCH_FOUR = 3
} app_switch_pos_t;

typedef switch_action_t m_switch_state_t;

typedef enum {
	UNLOCK = 0,
	LOCK
} m_switch_lock_t;

typedef enum {
	SWITCH_ONE_KEEP_OLD_STATE = 0,
	SWITCH_ONE_DEFAULT_ON = 1,
	SWITCH_ONE_DEFAULT_OFF = 2,

	SWITCH_TWO_KEEP_OLD_STATE = 3,
	SWITCH_TWO_DEFAULT_ON = 4,
	SWITCH_TWO_DEFAULT_OFF = 5,

	SWITCH_THREE_KEEP_OLD_STATE = 6,
	SWITCH_THREE_DEFAULT_ON = 7,
	SWITCH_THREE_DEFAULT_OFF = 8,

	SWITCH_FOUR_KEEP_OLD_STATE = 9,
	SWITCH_FOUR_DEFAULT_ON = 10,
	SWITCH_FOUR_DEFAULT_OFF = 11

} m_state_default_switch_t;

typedef enum {
	OS_NOT_RESET_DEFAULT = 0,
	OS_RESET_DEFAULT_BY_BTN = 1,
	OS_RESET_DEFAULT_BY_SERVER = 2
} app_reset_src_t;

typedef struct {
	bool send_ble;
	bool need_create_new_id;
	uint8_t mac_source[6];
} m_ble_send_t;

typedef void(*p_switch_cfg_t)(uint8_t _index);
typedef uint8_t(*p_read_switch_t)(uint8_t _index);
typedef void(*p_write_switch_t)(uint8_t _index, uint8_t _value);
typedef uint8_t(*p_toggle_switch_t)(uint32_t _index, uint32_t act_id, bool respond, bool set_event_release);
typedef uint8_t(*p_control_switch_t)(uint32_t action, uint32_t act_id, uint32_t uid, bool respond, bool set_event_release, m_ble_send_t send_ble);

typedef struct {
	m_state_default_switch_t default_state[APP_MAX_CHANNEL_SUPPORT];
    m_switch_state_t         current_state[APP_MAX_CHANNEL_SUPPORT];
    m_switch_lock_t          channel_lock_state[APP_MAX_CHANNEL_SUPPORT];  
	uint32_t				 time_release[APP_MAX_CHANNEL_SUPPORT];
    uint32_t				 release_mode[APP_MAX_CHANNEL_SUPPORT];
} m_switch_info_t;

typedef struct {
	m_switch_info_t _switch_info;
	uint8_t max_number_channel;
/* need port by hardware */
	p_switch_cfg_t _switch_cfg_cb;
	p_read_switch_t _read_switch_cb;
	p_write_switch_t _write_switch_cb;

/* protected api */
	p_toggle_switch_t	_toggle_switch_cb;
	p_control_switch_t _control_switch_cb;
} m_switch_t;

/* need call from hardware at start-up */
void m_switch_init(void);
m_switch_t m_switch_get_instance(void);
m_switch_info_t* m_switch_get_info_reference(void);
m_switch_state_t m_control_get_status_switch(uint8_t index);

/******************* SWITCH EVENT HANDLER ******************/
typedef enum {
    NOT_USE = 0,
    MODE_ON = 1,
    MODE_OFF = 2,
    MODE_BOTH = 3
} press_release_mode_t;

typedef struct {
    uint8_t enable;
    uint32_t time_on;
    uint32_t time_off;
} cyclic_event_switch_t;

void start_cylcic_timer(switch_action_t action, uint32_t time_act1, uint32_t time_act2);
void stop_cylcic_timer(uint32_t channel_index);
void set_user_trigger_press_event(uint8_t channel_index, bool set_event, uint32_t action);
void m_switch_cyclic_timer_loop(void);
void m_switch_press_release_loop(void);
void blink_switch_config_loop(void);

#endif /* CONTROL_H__ */


