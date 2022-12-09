#ifndef _M_SMART_CONFIG_H_
#define _M_SMART_CONFIG_H_

#include <stdbool.h>

typedef enum {
    SC_START = 0,
    SC_WAIT_CONFIG,
	SC_WAIT_INFO,
    SMARTCONFIG_DONE,
} smartconfig_state_t;

bool m_smartconfig_parse_data(char* json);
void reset_smartconfig(void);
smartconfig_state_t start_smartconfig(void);

#endif /* _M_SMART_CONFIG_H_ */