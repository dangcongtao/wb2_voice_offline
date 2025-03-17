#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

void http_get(char *url, char* output);
uint8_t get_mqtt_info(void);
void request_get_mqtt_info(void);
void http_client_loop(void);

#endif
