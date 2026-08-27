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

/** @brief Publish an unsolicited MQTT 225 report for a voice-driven volume change.
 *
 *  @param[in]  level  Absolute speaker volume in the range 1..7.
 *  @return     None.
 *  @note       Levels outside 1..7 are ignored. This report does not complete a pending SET or GET.
 */
void app_volume_report_voice(uint8_t level);

#endif /* APP_VOLUME_H */
