#ifndef _ESP_OTA_H_
#define _ESP_OTA_H_

#include <stdbool.h>

int m_ota_start(char* url);
bool m_ota_is_busy(void);
bool m_ota_has_task_finished(void);

#endif