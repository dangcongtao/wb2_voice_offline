/**
 * @file app_button.h
 * @brief VOICEBOX physical button actions and the button-to-application queue.
 */

#ifndef APP_BUTTON_H
#define APP_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"

#if !defined(VOICEBOX_HW) || !defined(VOICEBOX_HW_V1)
#error "VOICEBOX_HW is not defined: include app_config.h first"
#endif

/** Action produced by the VOICEBOX button state machine. */
typedef enum
{
    APP_BUTTON_ACTION_NONE = 0,
    APP_BUTTON_ACTION_VOLUME_UP,
    APP_BUTTON_ACTION_VOLUME_DOWN,
    APP_BUTTON_ACTION_VOICE_NEXT,
    APP_BUTTON_ACTION_LED_SMART_CONFIG,
    APP_BUTTON_ACTION_LED_AP_CONFIG,
    APP_BUTTON_ACTION_SMART_CONFIG,
    APP_BUTTON_ACTION_AP_CONFIG,
} app_button_action_t;

#if VOICEBOX_HW == VOICEBOX_HW_V1
#define APP_BUTTON_INDEX_FUNCTION 0U       /**< GPIO1, the only button */
#define APP_BUTTON_INDEX_VOLUME_UP 0xFFU   /**< not fitted on V1 */
#define APP_BUTTON_INDEX_VOLUME_DOWN 0xFFU /**< not fitted on V1 */
#else
#define APP_BUTTON_INDEX_VOLUME_UP 0U   /**< IO21 */
#define APP_BUTTON_INDEX_FUNCTION 1U    /**< IO20 */
#define APP_BUTTON_INDEX_VOLUME_DOWN 2U /**< IO0  */
#endif

/** @brief Translate one button-library event into an action.
 *
 *  @param[in]  index  Button index (APP_BUTTON_INDEX_*). V2: 0 volume up,
 *                     1 function, 2 volume down. V1: 0 function.
 *  @param[in]  event  One APP_BTN_EVT_* value.
 *  @return     The action to run, or APP_BUTTON_ACTION_NONE.
 *  @note       Pure state machine, keeps per-button hold state. Call from the button task.
 */
app_button_action_t app_button_voicebox_event(uint32_t index, int event);

/** @brief Create the static queue of 4 queued button actions.
 *
 *  @return     None.
 */
void app_button_init(void);

/** @brief Queue one volume or voice action for the application task.
 *
 *  @param[in]  action  APP_BUTTON_ACTION_VOLUME_UP, VOLUME_DOWN or VOICE_NEXT.
 *  @return     true when queued, false if not initialized, queue full or other action.
 *  @note       Call from the button task.
 */
bool app_button_post(app_button_action_t action);

/** @brief Run all queued button actions.
 *
 *  @return     None.
 *  @note       Call from application task context. A busy rejection beeps once
 *              unless a 220/227 learn session is active (then it stays silent).
 */
void app_button_poll(void);

#endif /* APP_BUTTON_H */
