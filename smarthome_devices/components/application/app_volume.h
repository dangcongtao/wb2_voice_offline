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
 *  @note       Accepted SET levels are 1..7. Mute (0) is not accepted via SET.
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

/** @brief Step the speaker volume one level up or down from a physical button.
 *
 *  @param[in]  up  true to raise the volume, false to lower it.
 *  @return     false when busy or OTA owns the UART (nothing sent), otherwise true
 *              (request sent, or the TX failure was already reported).
 *  @note       Asks the chip first when the level is unknown. The result is reported
 *              to the App as MQTT 225 with u=0 and act_id=SRC_ACTION_DEVICE.
 */
bool app_volume_button_step(bool up);

/** @brief Publish an unsolicited MQTT 225 report for a voice-driven volume change.
 *
 *  @param[in]  level   Absolute speaker volume in the range 0..7, where 0 is mute.
 *  @param[in]  cmd_id  ASR command identifier (50..57).
 *  @param[in]  cwsl    CWSL slot identifier (255 for default, 0..14 for learned).
 *  @return     None.
 *  @note       Levels outside 0..7 are ignored. This report does not complete a pending SET or GET.
 */
void app_volume_report_voice(uint8_t level, uint8_t cmd_id, uint8_t cwsl);

#endif /* APP_VOLUME_H */
