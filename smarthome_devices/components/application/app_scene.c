/**
 * @file app_scene.c
 * @brief Scene management and voice learning via MQTT.
 */

#include "app_scene.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "app_cwsl.h"
#include "easyflash_common.h"
#include "process_data.h"
#include "system_typedef.h"


#define APP_SCENE_RESPONSE_SIZE 180U

static uint16_t s_app_scene_mask;
static uint8_t s_app_scene_delete_all_active;
static uint8_t s_app_scene_delete_all_cursor;
static uint32_t s_app_scene_pending_uid;
static uint8_t s_app_scene_pending_act_id;
static uint8_t s_app_scene_pending_cmd_id;
static uint8_t s_app_scene_pending_respond_id;

static void app_scene_publish(const char *response, int response_length)
{
    if ((response == NULL) ||
        (response_length <= 0) ||
        ((size_t)response_length >= APP_SCENE_RESPONSE_SIZE)) {
        os_trace("[SCENE] response formatting failed\r\n");
        return;
    }

    app_send_data((char *)response, (uint32_t)response_length);
}

static bool app_scene_save_mask(uint16_t mask)
{
    uint8_t retry;

    for (retry = 0U; retry < 3U; retry++) {
        if (ef_set_u16("scene_mask", mask)) {
            return true;
        }
    }
    os_trace("[SCENE] failed to save scene_mask to flash: 0x%04x\r\n", mask);
    return false;
}

static bool app_scene_read_number(cJSON *js_root,
                                  const char *key,
                                  uint32_t maximum,
                                  uint32_t *value)
{
    cJSON *item;
    double number;
    uint32_t converted;

    if ((js_root == NULL) || (key == NULL) || (value == NULL)) {
        return false;
    }

    item = cJSON_GetObjectItem(js_root, key);
    if ((item == NULL) || ((item->type & 0xFF) != cJSON_Number)) {
        return false;
    }

    number = item->valuedouble;
    if (!((number >= 0.0) && (number <= (double)maximum))) {
        return false;
    }

    converted = (uint32_t)number;
    if ((double)converted != number) {
        return false;
    }

    *value = converted;
    return true;
}

static void app_scene_publish_add_reject(uint8_t cmd_id, uint8_t respond_id, uint32_t u, uint8_t act_id, uint8_t code)
{
    char response[APP_SCENE_RESPONSE_SIZE];
    int len = snprintf(response,
                       sizeof(response),
                       "{\"%s\":227,\"cmd_id\":%u,\"respond_id\":%u,\"u\":%lu,\"act_id\":%u,\"result\":0,\"code\":%u}",
                       gs_dev_type[device_type],
                       (unsigned int)cmd_id,
                       (unsigned int)respond_id,
                       (unsigned long)u,
                       (unsigned int)act_id,
                       (unsigned int)code);
    app_scene_publish(response, len);
}

static void app_scene_publish_add_success(uint8_t cmd_id, uint8_t respond_id, uint32_t u, uint8_t act_id)
{
    char response[APP_SCENE_RESPONSE_SIZE];
    int len = snprintf(response,
                       sizeof(response),
                       "{\"%s\":227,\"cmd_id\":%u,\"respond_id\":%u,\"u\":%lu,\"act_id\":%u,\"result\":1}",
                       gs_dev_type[device_type],
                       (unsigned int)cmd_id,
                       (unsigned int)respond_id,
                       (unsigned long)u,
                       (unsigned int)act_id);
    app_scene_publish(response, len);
}

static void app_scene_publish_delete_reject(uint8_t cmd_id, uint32_t u, uint8_t act_id, uint8_t code)
{
    char response[APP_SCENE_RESPONSE_SIZE];
    int len = snprintf(response,
                       sizeof(response),
                       "{\"%s\":228,\"cmd_id\":%u,\"u\":%lu,\"act_id\":%u,\"result\":0,\"code\":%u}",
                       gs_dev_type[device_type],
                       (unsigned int)cmd_id,
                       (unsigned long)u,
                       (unsigned int)act_id,
                       (unsigned int)code);
    app_scene_publish(response, len);
}

static void app_scene_publish_delete_success(uint8_t cmd_id, uint32_t u, uint8_t act_id)
{
    char response[APP_SCENE_RESPONSE_SIZE];
    int len = snprintf(response,
                       sizeof(response),
                       "{\"%s\":228,\"cmd_id\":%u,\"u\":%lu,\"act_id\":%u,\"result\":1}",
                       gs_dev_type[device_type],
                       (unsigned int)cmd_id,
                       (unsigned long)u,
                       (unsigned int)act_id);
    app_scene_publish(response, len);
}

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
static void app_scene_run_validation_tests(void);
#endif

void app_scene_init(void)
{
    s_app_scene_delete_all_active = 0U;
    s_app_scene_delete_all_cursor = 0U;
    s_app_scene_pending_uid = 0U;
    s_app_scene_pending_act_id = 0U;
    s_app_scene_pending_cmd_id = 0U;
    s_app_scene_pending_respond_id = 0U;

    if (!ef_get_u16("scene_mask", &s_app_scene_mask)) {
        s_app_scene_mask = 0U;
    }
    s_app_scene_mask &= APP_SCENE_MASK_ALL;

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
    app_scene_run_validation_tests();
#endif
}

bool app_scene_is_active(uint8_t scene_index)
{
    if ((scene_index < 1U) || (scene_index > APP_SCENE_COUNT_MAX)) {
        return false;
    }
    return (s_app_scene_mask & (uint16_t)(1U << (scene_index - 1U))) != 0U;
}

bool app_scene_is_cmd_id_active(uint8_t cmd_id)
{
    if ((cmd_id < APP_SCENE_CMD_ID_MIN) || (cmd_id > APP_SCENE_CMD_ID_MAX)) {
        return false;
    }
    return (s_app_scene_mask & (uint16_t)(1U << (cmd_id - APP_SCENE_CMD_ID_MIN))) != 0U;
}

bool app_scene_is_pending(void)
{
    return (s_app_scene_delete_all_active != 0U);
}

void app_scene_clear_all(void)
{
    s_app_scene_delete_all_active = 0U;
    s_app_scene_mask = 0U;
    (void)app_scene_save_mask(0U);
}

void app_scene_add(cJSON *js_root)
{
    uint32_t u = 0U;
    uint32_t act_id = 0U;
    uint32_t cmd_id = 0U;
    uint32_t respond_id = 0U;
    uint32_t cmd_type = 0U;
    uint8_t code;
    uint8_t candidate_cmd_id;
    uint8_t bit_idx;

    if (js_root == NULL) {
        app_scene_publish_add_reject(0U, 0U, 0U, 0U, 64U);
        return;
    }

    /* Best-effort read so the reject message echoes what the caller sent */
    (void)app_scene_read_number(js_root, CJSON_KEY_USER_ID, UINT32_MAX, &u);
    (void)app_scene_read_number(js_root, CJSON_KEY_ACT_ID, UINT8_MAX, &act_id);
    (void)app_scene_read_number(js_root, APP_SCENE_KEY_CMD_ID, UINT8_MAX, &cmd_id);
    (void)app_scene_read_number(js_root, APP_SCENE_KEY_RESPOND_ID, UINT8_MAX, &respond_id);

    if (cJSON_GetObjectItem(js_root, CJSON_KEY_ACTION) != NULL) {
        app_scene_publish_add_reject((uint8_t)cmd_id, (uint8_t)respond_id, u, (uint8_t)act_id, 64U);
        return;
    }

    if (!app_scene_read_number(js_root, gs_dev_type[device_type], UINT32_MAX, &cmd_type) ||
        (cmd_type != APP_SCENE_CMD_ADD) ||
        !app_scene_read_number(js_root, CJSON_KEY_USER_ID, UINT32_MAX, &u) ||
        !app_scene_read_number(js_root, CJSON_KEY_ACT_ID, UINT8_MAX, &act_id) ||
        !app_scene_read_number(js_root, APP_SCENE_KEY_CMD_ID, UINT8_MAX, &cmd_id) ||
        !app_scene_read_number(js_root, APP_SCENE_KEY_RESPOND_ID, UINT8_MAX, &respond_id) ||
        !app_scene_respond_id_is_valid(respond_id)) {
        app_scene_publish_add_reject((uint8_t)cmd_id, (uint8_t)respond_id, u, (uint8_t)act_id, 64U);
        return;
    }

    if ((cmd_id != APP_SCENE_CMD_ID_NEW) &&
        ((cmd_id < APP_SCENE_CMD_ID_MIN) || (cmd_id > APP_SCENE_CMD_ID_MAX))) {
        app_scene_publish_add_reject((uint8_t)cmd_id, (uint8_t)respond_id, u, (uint8_t)act_id, 65U);
        return;
    }

    if (app_scene_is_pending()) {
        app_scene_publish_add_reject((uint8_t)cmd_id, (uint8_t)respond_id, u, (uint8_t)act_id, 67U);
        return;
    }

    if (cmd_id == APP_SCENE_CMD_ID_NEW) {
        /* Add: allocate the lowest free slot and start learning */
        if (s_app_scene_mask == APP_SCENE_MASK_ALL) {
            app_scene_publish_add_reject(APP_SCENE_CMD_ID_NEW, (uint8_t)respond_id, u,
                                         (uint8_t)act_id, APP_SCENE_CODE_FULL);
            return;
        }

        for (bit_idx = 0U; bit_idx < APP_SCENE_COUNT_MAX; bit_idx++) {
            if ((s_app_scene_mask & (uint16_t)(1U << bit_idx)) == 0U) {
                break;
            }
        }
        if (bit_idx >= APP_SCENE_COUNT_MAX) {
            app_scene_publish_add_reject(APP_SCENE_CMD_ID_NEW, (uint8_t)respond_id, u,
                                         (uint8_t)act_id, APP_SCENE_CODE_FULL);
            return;
        }

        candidate_cmd_id = (uint8_t)(APP_SCENE_CMD_ID_MIN + bit_idx);
        code = app_cwsl_start_scene_learn(candidate_cmd_id, u, (uint8_t)act_id, (uint8_t)respond_id);
        if (code != 0U) {
            app_scene_publish_add_reject(APP_SCENE_CMD_ID_NEW, (uint8_t)respond_id, u,
                                         (uint8_t)act_id, code);
            return;
        }
        s_app_scene_pending_cmd_id = candidate_cmd_id;
    } else {
        /* Edit: change the respond_id of an existing scene, mask is untouched */
        if (!app_scene_is_cmd_id_active((uint8_t)cmd_id)) {
            app_scene_publish_add_reject((uint8_t)cmd_id, (uint8_t)respond_id, u, (uint8_t)act_id, 65U);
            return;
        }

        code = app_cwsl_start_scene_resp_set((uint8_t)cmd_id, (uint8_t)respond_id);
        if (code != 0U) {
            app_scene_publish_add_reject((uint8_t)cmd_id, (uint8_t)respond_id, u, (uint8_t)act_id, code);
            return;
        }
        s_app_scene_pending_cmd_id = (uint8_t)cmd_id;
    }
    os_trace("[SCENE] changing respond_id of cmd_id %u to %u\r\n",
             (unsigned int)cmd_id,
             (unsigned int)respond_id);

    s_app_scene_pending_uid = u;
    s_app_scene_pending_act_id = (uint8_t)act_id;
    s_app_scene_pending_respond_id = (uint8_t)respond_id;
}

void app_scene_on_learn_result(uint8_t code, uint8_t cmd_id)
{
    if (code == 32U) {
        if ((cmd_id >= APP_SCENE_CMD_ID_MIN) && (cmd_id <= APP_SCENE_CMD_ID_MAX)) {
            uint16_t new_mask = s_app_scene_mask | (uint16_t)(1U << (cmd_id - APP_SCENE_CMD_ID_MIN));
            if (app_scene_save_mask(new_mask)) {
                s_app_scene_mask = new_mask;
                app_scene_publish_add_success(cmd_id,
                                              s_app_scene_pending_respond_id,
                                              s_app_scene_pending_uid,
                                              s_app_scene_pending_act_id);
                return;
            }
        }
        app_scene_publish_add_reject(s_app_scene_pending_cmd_id,
                                     s_app_scene_pending_respond_id,
                                     s_app_scene_pending_uid,
                                     s_app_scene_pending_act_id,
                                     70U);
    } else {
        app_scene_publish_add_reject(s_app_scene_pending_cmd_id,
                                     s_app_scene_pending_respond_id,
                                     s_app_scene_pending_uid,
                                     s_app_scene_pending_act_id,
                                     code);
    }
}

void app_scene_on_resp_set_result(uint8_t code)
{
    if (code == 0U) {
        app_scene_publish_add_success(s_app_scene_pending_cmd_id,
                                      s_app_scene_pending_respond_id,
                                      s_app_scene_pending_uid,
                                      s_app_scene_pending_act_id);
    } else {
        app_scene_publish_add_reject(s_app_scene_pending_cmd_id,
                                     s_app_scene_pending_respond_id,
                                     s_app_scene_pending_uid,
                                     s_app_scene_pending_act_id,
                                     code);
    }
}

void app_scene_delete(cJSON *js_root)
{
    uint32_t u = 0U;
    uint32_t act_id = 0U;
    uint32_t cmd_type = 0U;
    uint32_t cmd_id = 0U;

    if (js_root == NULL) {
        app_scene_publish_delete_reject(0U, 0U, 0U, 65U);
        return;
    }

    (void)app_scene_read_number(js_root, CJSON_KEY_USER_ID, UINT32_MAX, &u);
    (void)app_scene_read_number(js_root, CJSON_KEY_ACT_ID, UINT8_MAX, &act_id);
    (void)app_scene_read_number(js_root, "cmd_id", UINT8_MAX, &cmd_id);

    if (cJSON_GetObjectItem(js_root, CJSON_KEY_ACTION) != NULL) {
        app_scene_publish_delete_reject((uint8_t)cmd_id, u, (uint8_t)act_id, 65U);
        return;
    }

    if (!app_scene_read_number(js_root, gs_dev_type[device_type], UINT32_MAX, &cmd_type) ||
        (cmd_type != APP_SCENE_CMD_DELETE) ||
        !app_scene_read_number(js_root, "cmd_id", UINT8_MAX, &cmd_id) ||
        !app_scene_read_number(js_root, CJSON_KEY_USER_ID, UINT32_MAX, &u) ||
        !app_scene_read_number(js_root, CJSON_KEY_ACT_ID, UINT8_MAX, &act_id)) {
        app_scene_publish_delete_reject((uint8_t)cmd_id, u, (uint8_t)act_id, 65U);
        return;
    }

    if (!(((cmd_id >= APP_SCENE_CMD_ID_MIN) && (cmd_id <= APP_SCENE_CMD_ID_MAX)) || (cmd_id == 255U))) {
        app_scene_publish_delete_reject((uint8_t)cmd_id, u, (uint8_t)act_id, 65U);
        return;
    }

    if (app_scene_is_pending()) {
        app_scene_publish_delete_reject((uint8_t)cmd_id, u, (uint8_t)act_id, 67U);
        return;
    }

    /* Deleting a slot that is not active is rejected (255 = delete all is exempt) */
    if ((cmd_id != 255U) && !app_scene_is_cmd_id_active((uint8_t)cmd_id)) {
        app_scene_publish_delete_reject((uint8_t)cmd_id, u, (uint8_t)act_id, 65U);
        return;
    }

    if (cmd_id == 255U) {
        uint8_t code = app_cwsl_start_scene_delete(APP_SCENE_CMD_ID_MIN, u, (uint8_t)act_id);
        if (code != 0U) {
            app_scene_publish_delete_reject(255U, u, (uint8_t)act_id, code);
            return;
        }
        s_app_scene_delete_all_active = 1U;
        s_app_scene_delete_all_cursor = APP_SCENE_CMD_ID_MIN;
        s_app_scene_pending_uid = u;
        s_app_scene_pending_act_id = (uint8_t)act_id;
        return;
    }

    /* Single slot delete (56..70) */
    uint8_t code = app_cwsl_start_scene_delete((uint8_t)cmd_id, u, (uint8_t)act_id);
    if (code != 0U) {
        app_scene_publish_delete_reject((uint8_t)cmd_id, u, (uint8_t)act_id, code);
        return;
    }
    s_app_scene_pending_uid = u;
    s_app_scene_pending_act_id = (uint8_t)act_id;
}

void app_scene_on_delete_result(uint8_t code, uint8_t cmd_id)
{
    if (s_app_scene_delete_all_active != 0U) {
        if ((code == 33U) || (code == 51U)) {
            if ((cmd_id >= APP_SCENE_CMD_ID_MIN) && (cmd_id <= APP_SCENE_CMD_ID_MAX)) {
                s_app_scene_mask &= (uint16_t)~(1U << (cmd_id - APP_SCENE_CMD_ID_MIN));
                (void)app_scene_save_mask(s_app_scene_mask);
            }
            s_app_scene_delete_all_cursor++;
            if (s_app_scene_delete_all_cursor <= APP_SCENE_CMD_ID_MAX) {
                uint8_t next_code = app_cwsl_start_scene_delete(s_app_scene_delete_all_cursor,
                                                                s_app_scene_pending_uid,
                                                                s_app_scene_pending_act_id);
                if (next_code != 0U) {
                    s_app_scene_delete_all_active = 0U;
                    (void)app_scene_save_mask(s_app_scene_mask);
                    app_scene_publish_delete_reject(255U,
                                                    s_app_scene_pending_uid,
                                                    s_app_scene_pending_act_id,
                                                    next_code);
                }
            } else {
                s_app_scene_delete_all_active = 0U;
                s_app_scene_mask = 0U;
                (void)app_scene_save_mask(0U);
                app_scene_publish_delete_success(255U,
                                                 s_app_scene_pending_uid,
                                                 s_app_scene_pending_act_id);
            }
        } else {
            s_app_scene_delete_all_active = 0U;
            (void)app_scene_save_mask(s_app_scene_mask);
            app_scene_publish_delete_reject(255U,
                                            s_app_scene_pending_uid,
                                            s_app_scene_pending_act_id,
                                            code);
        }
        return;
    }

    if ((code == 33U) || (code == 51U)) {
        if ((cmd_id >= APP_SCENE_CMD_ID_MIN) && (cmd_id <= APP_SCENE_CMD_ID_MAX)) {
            s_app_scene_mask &= (uint16_t)~(1U << (cmd_id - APP_SCENE_CMD_ID_MIN));
            (void)app_scene_save_mask(s_app_scene_mask);
        }
        app_scene_publish_delete_success(cmd_id,
                                         s_app_scene_pending_uid,
                                         s_app_scene_pending_act_id);
    } else {
        app_scene_publish_delete_reject(cmd_id,
                                        s_app_scene_pending_uid,
                                        s_app_scene_pending_act_id,
                                        code);
    }
}

void app_scene_query(cJSON *js_root)
{
    uint32_t u = 0U;
    uint32_t act_id = 0U;
    uint32_t cmd_type = 0U;
    char resp[APP_SCENE_RESPONSE_SIZE];
    char scenes_buf[64];
    size_t scenes_len = 0;
    uint8_t total = 0U;
    uint8_t bit_idx;

    if (js_root == NULL) {
        int len = snprintf(resp,
                           sizeof(resp),
                           "{\"%s\":229,\"u\":0,\"act_id\":0,\"result\":0,\"code\":64}",
                           gs_dev_type[device_type]);
        app_scene_publish(resp, len);
        return;
    }

    (void)app_scene_read_number(js_root, CJSON_KEY_USER_ID, UINT32_MAX, &u);
    (void)app_scene_read_number(js_root, CJSON_KEY_ACT_ID, UINT8_MAX, &act_id);

    if (cJSON_GetObjectItem(js_root, CJSON_KEY_ACTION) != NULL) {
        int len = snprintf(resp,
                           sizeof(resp),
                           "{\"%s\":229,\"u\":%lu,\"act_id\":%u,\"result\":0,\"code\":64}",
                           gs_dev_type[device_type],
                           (unsigned long)u,
                           (unsigned int)act_id);
        app_scene_publish(resp, len);
        return;
    }

    if (!app_scene_read_number(js_root, gs_dev_type[device_type], UINT32_MAX, &cmd_type) ||
        (cmd_type != APP_SCENE_CMD_QUERY) ||
        !app_scene_read_number(js_root, CJSON_KEY_USER_ID, UINT32_MAX, &u) ||
        !app_scene_read_number(js_root, CJSON_KEY_ACT_ID, UINT8_MAX, &act_id)) {
        int len = snprintf(resp,
                           sizeof(resp),
                           "{\"%s\":229,\"u\":%lu,\"act_id\":%u,\"result\":0,\"code\":64}",
                           gs_dev_type[device_type],
                           (unsigned long)u,
                           (unsigned int)act_id);
        app_scene_publish(resp, len);
        return;
    }

    if (app_scene_is_pending()) {
        int len = snprintf(resp,
                           sizeof(resp),
                           "{\"%s\":229,\"u\":%lu,\"act_id\":%u,\"result\":0,\"code\":67}",
                           gs_dev_type[device_type],
                           (unsigned long)u,
                           (unsigned int)act_id);
        app_scene_publish(resp, len);
        return;
    }

    scenes_buf[0] = '\0';
    for (bit_idx = 0U; bit_idx < APP_SCENE_COUNT_MAX; bit_idx++) {
        if ((s_app_scene_mask & (uint16_t)(1U << bit_idx)) != 0U) {
            uint8_t cmd_id = (uint8_t)(APP_SCENE_CMD_ID_MIN + bit_idx);
            int written;
            if (total == 0U) {
                written = snprintf(scenes_buf + scenes_len,
                                   sizeof(scenes_buf) - scenes_len,
                                   "%u",
                                   (unsigned int)cmd_id);
            } else {
                written = snprintf(scenes_buf + scenes_len,
                                   sizeof(scenes_buf) - scenes_len,
                                   ",%u",
                                   (unsigned int)cmd_id);
            }
            if ((written > 0) && ((size_t)written < (sizeof(scenes_buf) - scenes_len))) {
                scenes_len += (size_t)written;
            }
            total++;
        }
    }

    int resp_len = snprintf(resp,
                            sizeof(resp),
                            "{\"%s\":229,\"scenes\":[%s],\"total\":%u,\"u\":%lu,\"act_id\":%u,\"result\":1}",
                            gs_dev_type[device_type],
                            scenes_buf,
                            (unsigned int)total,
                            (unsigned long)u,
                            (unsigned int)act_id);
    app_scene_publish(resp, resp_len);
}

#if defined(BUILD_TEST) && (BUILD_TEST == 1)

static void app_scene_run_validation_tests(void)
{
    uint32_t failed_count = 0U;

    /* Test 1: is_active bounds */
    if (app_scene_is_active(0U) != false) { failed_count++; }
    if (app_scene_is_active(16U) != false) { failed_count++; }

    /* Test 2: is_cmd_id_active bounds */
    if (app_scene_is_cmd_id_active(55U) != false) { failed_count++; }
    if (app_scene_is_cmd_id_active(71U) != false) { failed_count++; }
    if (app_scene_is_cmd_id_active(255U) != false) { failed_count++; }

    /* Test 3: mask bits indexing */
    uint16_t orig_mask = s_app_scene_mask;
    s_app_scene_mask = 0x0005U; /* bits 0 and 2 (cmd_id 56 and 58) */
    if (!app_scene_is_active(1U)) { failed_count++; }
    if (app_scene_is_active(2U)) { failed_count++; }
    if (!app_scene_is_active(3U)) { failed_count++; }
    if (!app_scene_is_cmd_id_active(56U)) { failed_count++; }
    if (app_scene_is_cmd_id_active(57U)) { failed_count++; }
    if (!app_scene_is_cmd_id_active(58U)) { failed_count++; }

    /* Test 4: JSON read number tests */
    cJSON *test_js = cJSON_Parse("{\"fanoem\":227,\"u\":100,\"act_id\":1}");
    if (test_js != NULL) {
        uint32_t val = 0;
        if (!app_scene_read_number(test_js, "u", 200, &val) || (val != 100)) { failed_count++; }
        if (app_scene_read_number(test_js, "u", 50, &val)) { failed_count++; } /* exceeded max */
        if (app_scene_read_number(test_js, "nonexist", 100, &val)) { failed_count++; }
        cJSON_Delete(test_js);
    } else {
        failed_count++;
    }

    s_app_scene_mask = orig_mask;

    os_trace("[SCENE][TEST] validation total=4 failed=%lu\r\n", (unsigned long)failed_count);
}

#endif
