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
#define APP_SCENE_CMD_RESP_SET  232U
/* wake words 1..6 */
#define APP_SCENE_RESP_WAKE_ID_MAX 6U
/* volume 48..55, scenes 56..70, home 71..103 */
#define APP_SCENE_RESP_CMD_ID_MIN 48U
#define APP_SCENE_RESP_CMD_ID_MAX 103U

#define APP_SCENE_COUNT_MAX    15U
#define APP_SCENE_CMD_ID_MIN   56U
#define APP_SCENE_CMD_ID_MAX   70U
#define APP_SCENE_MASK_ALL     ((uint16_t)((1U << APP_SCENE_COUNT_MAX) - 1U))

#define APP_SCENE_CODE_FULL    50U
/** @brief cmd_id sent in a 227 request to add a new scene (not edit). */
#define APP_SCENE_CMD_ID_NEW   255U

/** @brief Check a cmd_id of the VOICEBOX voice model: wake 1..6, 48..103.
 *  Used by MQTT 232 and, on VOICEBOX, by MQTT 220, 221 and 223.
 *  @param[in] cmd_id ASR command ID from the request.
 *  @return true if the chip has this command, false otherwise.
 */
static inline bool app_scene_cmd_id_exists(uint32_t cmd_id)
{
    return ((cmd_id >= 1U) && (cmd_id <= APP_SCENE_RESP_WAKE_ID_MAX)) ||
           ((cmd_id >= APP_SCENE_RESP_CMD_ID_MIN) &&
            (cmd_id <= APP_SCENE_RESP_CMD_ID_MAX));
}

/** @brief Check a response phrase ID accepted by MQTT 220, 227 and 232.
 *  Accepted set: 0..3, 11..19, 40..47, 50..68, 70, 71.
 *  @param[in] respond_id Response phrase ID from the request.
 *  @return true if accepted, false otherwise.
 */
static inline bool app_scene_respond_id_is_valid(uint32_t respond_id)
{
    return (respond_id <= 3U) ||
           ((respond_id >= 11U) && (respond_id <= 19U)) ||
           ((respond_id >= 40U) && (respond_id <= 47U)) ||
           ((respond_id >= 50U) && (respond_id <= 68U)) ||
           (respond_id == 70U) || (respond_id == 71U);
}

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

/** @brief Handle MQTT cmd 232 (set the response phrase of one chip command).
 *  Request: {"<type>":232,"u":..,"cmd_id":..,"respond_id":..,"act_id":..}
 *  Reply: {"<type>":232,"u":..,"cmd_id":..,"respond_id":..,"act_id":..,
 *  "result":1} or the same with "result":0,"code":N on failure.
 *  @param[in] js_root Parsed request JSON.
 */
void app_scene_resp_set(cJSON *js_root);

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
