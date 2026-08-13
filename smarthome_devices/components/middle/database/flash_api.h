#ifndef __FLASH_API_H
#define __FLASH_API_H
#include <stdio.h>
#include <stdint.h>

#include "m_database.h"

void flash_init(void);
void flash_write_database(m_device_t data, uint8_t request_flag);
void flash_read_database(m_device_t *object_data);

/** @brief Read saved Wi-Fi station credentials.
 *
 *  @param[out]  wifi_info  Destination for the saved Wi-Fi record.
 *  @return      Operation status.
 *  @retval      0          Complete Wi-Fi record read successfully.
 *  @retval      -1         Invalid output pointer or incomplete record.
 */
int32_t flash_read_wifi_info(m_wifi_info_t *wifi_info);

void flash_read_device_id(void *object_data, uint32_t size);
void flash_write_device_id(void* device_id, uint32_t size);

#endif