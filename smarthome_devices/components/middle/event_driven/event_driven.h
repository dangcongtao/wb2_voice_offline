#ifndef _M_EVENT_DRV_H_
#define _M_EVENT_DRV_H_

#include "system_typedef.h"

void m_event_init(void);
void m_event_loop(uint32_t time_schedual_ms);
void signal_post(m_signal_t sig);
void signal_handler_register(m_state_transmit_t new_state);
void sync_led_stt_when_reset_ts04(void);
uint8_t dev_is_doing_config(void);
void set_timeout_reset(void);
void clear_timeout_reset(void);
uint8_t dev_is_online(void);

#endif /* _M_SMART_CONFIG_H_ */