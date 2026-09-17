#ifndef __EASY_FLASH_COMMON_H__
#define __EASY_FLASH_COMMON_H__

#include <stdbool.h>
#include <stdint.h>

bool ef_get_u16(const char *key, uint16_t *val);
bool ef_set_u16(const char *key, uint16_t val);

#endif /* __EASY_FLASH_COMMON_H__ */
