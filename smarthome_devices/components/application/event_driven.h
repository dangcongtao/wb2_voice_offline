#ifndef _M_EVENT_DRV_H_
#define _M_EVENT_DRV_H_

#include "system_typedef.h"

void m_event_loop(void);
void signal_post(m_signal_t sig);
void signal_handler_register(m_state_transmit_t new_state);

#endif /* _M_SMART_CONFIG_H_ */