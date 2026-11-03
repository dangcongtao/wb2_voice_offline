/**
 * @file app_voice.h
 * @brief Application-level assistant voice selection over MQTT and UART.
 */

#ifndef APP_VOICE_H
#define APP_VOICE_H

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"

/** @brief Initialize voice request state and register the UART callbacks.
 *
 *  @return     None.
 */
void app_voice_init(void);

/** @brief Handle one MQTT 230 request to select the assistant voice.
 *
 *  @param[in]  js_root  Parsed MQTT request object.
 *  @return     None.
 *  @note       Accepted voice_id values are 1..4. Every request key is required.
 */
void app_voice_set(cJSON *js_root);

/** @brief Handle one MQTT 231 request to read the selected assistant voice.
 *
 *  @param[in]  js_root  Parsed MQTT request object.
 *  @return     None.
 */
void app_voice_get(cJSON *js_root);

/** @brief Process voice request timeout and late-response drain state.
 *
 *  @return     None.
 *  @note       Call periodically from application task context.
 */
void app_voice_poll(void);

/** @brief Check whether a voice request is pending or draining.
 *
 *  @return     true while the voice UART transaction is active, otherwise false.
 */
bool app_voice_is_pending(void);

#endif /* APP_VOICE_H */
