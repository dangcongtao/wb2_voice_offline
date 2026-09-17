/**
 * @file app_scene.h
 * @brief Scene management and voice learning via MQTT.
 */

#ifndef APP_SCENE_H
#define APP_SCENE_H

#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"

#define APP_SCENE_CMD_ADD      227U
#define APP_SCENE_CMD_DELETE   228U
#define APP_SCENE_CMD_QUERY    229U

#define APP_SCENE_COUNT_MAX    10U
#define APP_SCENE_CMD_ID_MIN   58U
#define APP_SCENE_CMD_ID_MAX   67U

#define APP_SCENE_CODE_FULL    71U

/** @brief Initialize scene management and load mask from EasyFlash. */
void app_scene_init(void);

/** @brief Handle MQTT cmd 227 (Add Scene with voice learning).
 *  @param[in] js_root Parsed JSON request.
 */
void app_scene_add(cJSON *js_root);

/** @brief Handle MQTT cmd 228 (Delete Scene).
 *  @param[in] js_root Parsed JSON request.
 */
void app_scene_delete(cJSON *js_root);

/** @brief Handle MQTT cmd 229 (Query Scenes).
 *  @param[in] js_root Parsed JSON request.
 */
void app_scene_query(cJSON *js_root);

/** @brief Check if a 1-based scene index (1..10) is active.
 *  @param[in] scene_index 1-based index (1..10).
 *  @return true if active, false otherwise.
 */
bool app_scene_is_active(uint8_t scene_index);

/** @brief Check if an ASR command ID (58..67) is active as a scene.
 *  @param[in] cmd_id ASR command ID (58..67).
 *  @return true if active, false otherwise.
 */
bool app_scene_is_cmd_id_active(uint8_t cmd_id);

/** @brief Callback invoked by app_cwsl FSM when scene learning completes.
 *  @param[in] code Status code from chip or timeout.
 *  @param[in] cmd_id ASR command ID.
 */
void app_scene_on_learn_result(uint8_t code, uint8_t cmd_id);

/** @brief Callback invoked by app_cwsl FSM when scene deletion completes.
 *  @param[in] code Status code from chip.
 *  @param[in] cmd_id ASR command ID.
 */
void app_scene_on_delete_result(uint8_t code, uint8_t cmd_id);

/** @brief Check if a multi-step scene operation (like delete-all) is active.
 *  @return true if active, false otherwise.
 */
bool app_scene_is_pending(void);

#endif /* APP_SCENE_H */
