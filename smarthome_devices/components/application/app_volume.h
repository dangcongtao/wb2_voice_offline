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

/** @brief Increase the speaker volume by one level.
 *
 *  @return     Start status.
 *  @retval     0  The asynchronous operation was started.
 *  @retval     -1 Another operation is pending, UART is unavailable, or OTA is active.
 */
int32_t app_volume_increase(void);

/** @brief Decrease the speaker volume by one level.
 *
 *  @return     Start status.
 *  @retval     0  The asynchronous operation was started.
 *  @retval     -1 Another operation is pending, UART is unavailable, or OTA is active.
 */
int32_t app_volume_decrease(void);

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
