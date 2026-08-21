/**
 * @file app_volume.h
 * @brief Application-level speaker volume control over MQTT and UART.
 */

#ifndef APP_VOLUME_H
#define APP_VOLUME_H

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"

/** @brief Initialize speaker volume request state and UART callbacks.
 *
 *  @return     None.
 */
void app_volume_init(void);

/** @brief Handle one MQTT request to set the absolute speaker volume.
 *
 *  @param[in]  js_root  Parsed MQTT request object.
 *  @return     None.
 */
void app_volume_set(cJSON *js_root);

/** @brief Handle one MQTT request to get the current speaker volume.
 *
 *  @param[in]  js_root  Parsed MQTT request object.
 *  @return     None.
 */
void app_volume_get(cJSON *js_root);

/** @brief Process speaker volume timeout and late-response drain state.
 *
 *  @return     None.
 *  @note       Call periodically from application task context.
 */
void app_volume_poll(void);

/** @brief Check whether a speaker volume request is pending or draining.
 *
 *  @return     true while the volume UART transaction is active, otherwise false.
 */
bool app_volume_is_pending(void);

#endif /* APP_VOLUME_H */
