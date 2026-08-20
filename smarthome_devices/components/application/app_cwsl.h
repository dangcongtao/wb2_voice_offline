
/**
 * @file app_cwsl.h
 * @brief Application-level CWSL MQTT protocol and state machine.
 */

#ifndef APP_CWSL_H
#define APP_CWSL_H

#include <stdbool.h>

#include "cJSON.h"

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
 *  @return     true when no deferred clear is pending, otherwise false.
 */
bool app_cwsl_factory_clear_done(void);

/** @brief Notify application/cloud about a triggered voice command.
 *
 *  @param[in]  cmd_id  ASR command identifier (e.g. 7..14).
 *  @return     None.
 */
void app_cwsl_notify_voice_cmd(uint8_t cmd_id);

#endif /* APP_CWSL_H */
