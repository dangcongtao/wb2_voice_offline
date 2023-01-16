#ifndef _WIFI_CFG_H_
#define _WIFI_CFG_H_

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include <stdint.h>

typedef char* (*p_handler_t)(char* buf);

typedef struct {
    char uri[256];
    char method[32];
    p_handler_t handler;
} http_service_t;

void m_web_http_server(struct netconn *conn);
char* m_http_services(char* buf, uint16_t len);


char* m_apconfig_step1(char *req);
char* m_apconfig_step2(char *req);

void m_ble_config_step1(void);
void m_ble_config_step2(void);

#endif