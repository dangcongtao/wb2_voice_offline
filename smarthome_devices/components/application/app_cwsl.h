
/**
 * @file app_cwsl.h
 * @brief Application-level CWSL MQTT protocol and state machine.
 */

#ifndef APP_CWSL_H
#define APP_CWSL_H

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"

/** Maximum ASR command identifier accepted by CWSL LEARN/DELETE/QUERY. */
#define APP_CWSL_CMD_ID_MAX 65U

/** @brief Initialize the CWSL state machine and UART response callbacks.
 *
 *  @return     None.
 */
void app_cwsl_init(void);

/** @brief Handle one CWSL learn request.
 *
 *  @param[in]  js_root  Parsed MQTT request object.
 *  @return     None.
 */
void app_cwsl_learn(cJSON *js_root);

/** @brief Handle one CWSL delete request.
 *
 *  @param[in]  js_root  Parsed MQTT request object.
 *  @return     None.
 */
void app_cwsl_delete(cJSON *js_root);

/** @brief Handle one CWSL stop request.
 *
 *  @param[in]  js_root  Parsed MQTT request object.
 *  @return     None.
 */
void app_cwsl_stop(cJSON *js_root);

/** @brief Handle one CWSL query request.
 *
 *  @param[in]  js_root  Parsed MQTT request object.
 *  @return     None.
 */
void app_cwsl_query(cJSON *js_root);

/** @brief Process CWSL timeouts from application task context.
 *
 *  @return     None.
 */
void app_cwsl_poll(void);

/** @brief Check whether a mutating or query request is pending.
 *
 *  @return     true when at least one CWSL request is pending, otherwise false.
 */
bool app_cwsl_is_pending(void);

/** @brief Start a best-effort clear of all learned templates.
 *
 *  @return     true when the factory reset may proceed immediately, otherwise false.
 *  @note       A false result means STOP was sent and app_cwsl_poll must finish the clear.
 */
bool app_cwsl_factory_clear(void);

/** @brief Check whether the deferred factory clear has completed.
 *
 *  @return     true when no deferred clear is pending and mutating is inactive, otherwise false.
 */
bool app_cwsl_factory_clear_done(void);

/** @brief Notify application/cloud about a triggered voice command.
 *
 *  @param[in]  cmd_id  ASR command identifier (e.g. 7..67).
 *  @param[in]  cwsl    CWSL slot identifier (255 for default, 0..14 for learned).
 *  @return     None.
 */
void app_cwsl_notify_voice_cmd(uint8_t cmd_id, uint8_t cwsl);

/** @brief Start a scene learning session for an ASR command ID (58..67).
 *
 *  @param[in]  cmd_id   ASR command identifier (58..67).
 *  @param[in]  user_id  Requesting user ID.
 *  @param[in]  act_id   Requesting action ID.
 *  @return     0 on success, or error code (67, 69, 70).
 */
uint8_t app_cwsl_start_scene_learn(uint8_t cmd_id, uint32_t user_id, uint8_t act_id);

/** @brief Start a scene deletion session for an ASR command ID (58..67).
 *
 *  @param[in]  cmd_id   ASR command identifier (58..67).
 *  @param[in]  user_id  Requesting user ID.
 *  @param[in]  act_id   Requesting action ID.
 *  @return     0 on success, or error code (67, 69, 70).
 */
uint8_t app_cwsl_start_scene_delete(uint8_t cmd_id, uint32_t user_id, uint8_t act_id);

#endif /* APP_CWSL_H */

