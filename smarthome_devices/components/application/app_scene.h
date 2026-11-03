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

#define APP_SCENE_COUNT_MAX    15U
#define APP_SCENE_CMD_ID_MIN   56U
#define APP_SCENE_CMD_ID_MAX   70U
#define APP_SCENE_MASK_ALL     ((uint16_t)((1U << APP_SCENE_COUNT_MAX) - 1U))

#define APP_SCENE_CODE_FULL    50U
/** @brief cmd_id sent in a 227 request to add a new scene (not edit). */
#define APP_SCENE_CMD_ID_NEW   255U

/** @brief Initialize scene management and load mask from EasyFlash. */
void app_scene_init(void);

/** @brief Handle MQTT cmd 227 (add a scene by voice learning, or edit its respond_id).
 *  Request: {"<type>":227,"u":..,"cmd_id":..,"act_id":..,"respond_id":..}
 *  cmd_id 255 adds a new scene; 56..70 edits the respond_id of that active scene.
 *  Reply: {"<type>":227,"cmd_id":..,"respond_id":..,"u":..,"act_id":..,"result":1}
 *  or the same with "result":0,"code":N on failure.
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

/** @brief Check if a 1-based scene index (1..15) is active.
 *  @param[in] scene_index 1-based index (1..15).
 *  @return true if active, false otherwise.
 */
bool app_scene_is_active(uint8_t scene_index);

/** @brief Check if an ASR command ID (56..70) is active as a scene.
 *  @param[in] cmd_id ASR command ID (56..70).
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

/** @brief Callback invoked by app_cwsl when a respond_id change (cmd:15) completes.
 *  @param[in] code 0 = chip applied it, 64 = chip rejected, 52 = timeout.
 */
void app_scene_on_resp_set_result(uint8_t code);

/** @brief Check if a multi-step scene operation (like delete-all) is active.
 *  @return true if active, false otherwise.
 */
bool app_scene_is_pending(void);

/** @brief Clear all active scenes from RAM and Flash immediately.
 *  @note  Invoked when a global voice template wipe (CWSL delete all 255,255) succeeds.
 */
void app_scene_clear_all(void);

#endif /* APP_SCENE_H */
