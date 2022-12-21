#ifndef _WIFI_CFG_H_
#define _WIFI_CFG_H_

typedef char* (*p_handler_t)(char* buf);

typedef struct {
    char uri[256];
    char method[32];
    p_handler_t handler;
} http_service_t;

char* m_http_services(char* buf);

#endif