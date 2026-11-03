#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CJSON_KEY_ACTION "action"
#define CJSON_KEY_USER_ID "u"
#define CJSON_KEY_ACT_ID "act_id"
#define APP_SCENE_KEY_RESPOND_ID "respond_id"
#define APP_SCENE_KEY_CMD_ID "cmd_id"
#define os_trace printf

/* Guard unwanted SDK headers */
#define _USER_CONFIG_H_
#define __EASY_FLASH_COMMON_H__
#define _APP_MQTT_H__
#define _APP_MQTT_H_
#define CJSON_KEYWORD_H__
#define APP_CWSL_H

/* Mock cJSON by including repo implementation */
#include "components/third_party/cJson/cJSON.c"

/* Mock dependencies needed by app_scene.c */
char *gs_dev_type[] = {"fanoem", "lhswitch3v", "swminiv2", "sswitch2v", "swshockv2", "swshohuv2", "fanoem", "fandc", "fanac", "fanproacv2", "voiceboxv1"};
int device_type = 0;

static uint16_t mock_flash_mask = 0;
static bool mock_flash_fail_write = false;
static char last_published[256];
static int publish_count = 0;

bool ef_get_u16(const char *key, uint16_t *val) {
    if (strcmp(key, "scene_mask") == 0) {
        *val = mock_flash_mask;
        return true;
    }
    return false;
}

bool ef_set_u16(const char *key, uint16_t val) {
    if (strcmp(key, "scene_mask") == 0) {
        if (mock_flash_fail_write) {
            return false;
        }
        mock_flash_mask = val;
        return true;
    }
    return false;
}

void app_send_data(char *data, uint32_t len) {
    (void)len;
    strncpy(last_published, data, sizeof(last_published) - 1);
    last_published[sizeof(last_published) - 1] = '\0';
    publish_count++;
}

static uint8_t mock_cwsl_learn_ret = 0;
static uint8_t mock_cwsl_learn_last_cmd_id = 0;
static uint8_t mock_cwsl_learn_call_count = 0;
static uint8_t mock_cwsl_learn_respond_id = 0;
uint8_t app_cwsl_start_scene_learn(uint8_t cmd_id, uint32_t user_id, uint8_t act_id, uint8_t respond_id) {
    (void)user_id;
    (void)act_id;
    mock_cwsl_learn_call_count++;
    mock_cwsl_learn_respond_id = respond_id;
    mock_cwsl_learn_last_cmd_id = cmd_id;
    return mock_cwsl_learn_ret;
}

static uint8_t mock_cwsl_resp_set_ret = 0;
static uint8_t mock_cwsl_resp_set_call_count = 0;
static uint8_t mock_cwsl_resp_set_last_cmd_id = 0;
static uint8_t mock_cwsl_resp_set_last_respond_id = 0;
uint8_t app_cwsl_start_scene_resp_set(uint8_t cmd_id, uint8_t respond_id) {
    mock_cwsl_resp_set_call_count++;
    mock_cwsl_resp_set_last_cmd_id = cmd_id;
    mock_cwsl_resp_set_last_respond_id = respond_id;
    return mock_cwsl_resp_set_ret;
}

static uint8_t mock_cwsl_delete_ret = 0;
static uint8_t mock_cwsl_delete_call_count = 0;
static uint8_t mock_cwsl_delete_last_cmd_id = 0;
uint8_t app_cwsl_start_scene_delete(uint8_t cmd_id, uint32_t user_id, uint8_t act_id) {
    (void)user_id;
    (void)act_id;
    mock_cwsl_delete_call_count++;
    mock_cwsl_delete_last_cmd_id = cmd_id;
    return mock_cwsl_delete_ret;
}

/* Include the actual implementation */
#include "components/application/app_scene.c"

static void reset_mocks(void) {
    mock_flash_mask = 0;
    mock_flash_fail_write = false;
    last_published[0] = '\0';
    publish_count = 0;
    mock_cwsl_learn_ret = 0;
    mock_cwsl_learn_last_cmd_id = 0;
    mock_cwsl_learn_call_count = 0;
    mock_cwsl_learn_respond_id = 0;
    mock_cwsl_resp_set_ret = 0;
    mock_cwsl_resp_set_call_count = 0;
    mock_cwsl_resp_set_last_cmd_id = 0;
    mock_cwsl_resp_set_last_respond_id = 0;
    mock_cwsl_delete_ret = 0;
    mock_cwsl_delete_call_count = 0;
    mock_cwsl_delete_last_cmd_id = 0;
    app_scene_init();
}

int main(void) {
    printf("[TEST] Running native app_scene test suite...\n");

    /* TEST 1: Init & Bounds */
    {
        reset_mocks();
        mock_flash_mask = 0x0005; /* bits 0, 2 active -> slots 56, 58 */
        app_scene_init();
        assert(app_scene_is_active(1) == true);
        assert(app_scene_is_active(2) == false);
        assert(app_scene_is_active(3) == true);
        assert(app_scene_is_active(0) == false);
        assert(app_scene_is_active(11) == false);
        assert(app_scene_is_active(16) == false);
        assert(app_scene_is_cmd_id_active(56) == true);
        assert(app_scene_is_cmd_id_active(57) == false);
        assert(app_scene_is_cmd_id_active(58) == true);
        assert(app_scene_is_cmd_id_active(55) == false);
        assert(app_scene_is_cmd_id_active(66) == false);
        assert(app_scene_is_cmd_id_active(70) == false);
        assert(app_scene_is_cmd_id_active(71) == false);
        assert(app_scene_is_cmd_id_active(255) == false);
        printf("  PASS: Test 1 - Init & bounds\n");
    }

    /* TEST 2: Add Scene - Success & Lowest Free Slot Selection */
    {
        reset_mocks();
        mock_flash_mask = 0x0001; /* bit 0 set (slot 56 used) */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":227,\"cmd_id\":255,\"u\":1001,\"act_id\":5,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);

        /* Should pick bit 1 -> candidate cmd_id 57 */
        assert(mock_cwsl_learn_last_cmd_id == 57);
        assert(publish_count == 0); /* No immediate MQTT ACK */

        /* Callback arrives from FSM: LEARN_OK (32) */
        app_scene_on_learn_result(32, 57);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"cmd_id\":57") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"u\":1001") != NULL);
        assert(strstr(last_published, "\"act_id\":5") != NULL);
        assert(app_scene_is_cmd_id_active(57) == true);
        assert(mock_flash_mask == 0x0003); /* bits 0, 1 now set */
        printf("  PASS: Test 2 - Add scene lowest slot & success\n");
    }

    /* TEST 3: Add Scene - Rejection when 15 slots full (code 50) */
    {
        reset_mocks();
        mock_flash_mask = 0x7FFF; /* all 15 slots active */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":227,\"cmd_id\":255,\"u\":1002,\"act_id\":0,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);

        assert(publish_count == 1);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"code\":50") != NULL);
        assert(strstr(last_published, "\"u\":1002") != NULL);
        printf("  PASS: Test 3 - Add scene reject code 50 when full\n");
    }

    /* TEST 4: Add Scene - Rejection when CWSL busy / OTA (code 67, 69) without state corruption */
    {
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();

        /* Set a pending session state */
        s_app_scene_pending_uid = 999;
        s_app_scene_pending_act_id = 99;

        mock_cwsl_learn_ret = 67; /* BUSY */
        cJSON *req = cJSON_Parse("{\"fanoem\":227,\"cmd_id\":255,\"u\":8888,\"act_id\":12,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);

        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":67") != NULL);
        assert(strstr(last_published, "\"u\":8888") != NULL);
        /* Verify previous pending state was NOT corrupted */
        assert(s_app_scene_pending_uid == 999);
        assert(s_app_scene_pending_act_id == 99);
        printf("  PASS: Test 4 - Add scene reject busy protects pending state\n");
    }

    /* TEST 5: Add Scene - Invalid JSON / action field (code 64) */
    {
        reset_mocks();
        cJSON *req = cJSON_Parse("{\"fanoem\":227,\"action\":1,\"u\":100,\"act_id\":0}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":64") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);
        printf("  PASS: Test 5 - Add scene reject code 64 on deprecated action\n");
    }

    /* TEST 6: Delete Single Slot - Success (33) & Failure (51) */
    {
        reset_mocks();
        mock_flash_mask = 0x0003; /* slots 56, 57 active */
        app_scene_init();

        /* Delete slot 56 */
        cJSON *req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":56,\"u\":2001,\"act_id\":1}");
        app_scene_delete(req);
        cJSON_Delete(req);

        assert(mock_cwsl_delete_last_cmd_id == 56);
        assert(publish_count == 0);

        /* Chip responds with 33 (DELETE_OK) */
        app_scene_on_delete_result(33, 56);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"cmd_id\":56") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"u\":2001") != NULL);
        assert(app_scene_is_cmd_id_active(56) == false);
        assert(app_scene_is_cmd_id_active(57) == true);
        assert(mock_flash_mask == 0x0002);

        /* Delete slot 57 and chip returns 51 (slot empty) -> treated as success */
        publish_count = 0;
        req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":57,\"u\":2002,\"act_id\":2}");
        app_scene_delete(req);
        cJSON_Delete(req);

        app_scene_on_delete_result(51, 57);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"cmd_id\":57") != NULL);
        assert(app_scene_is_cmd_id_active(57) == false); /* bit cleared! */
        assert(mock_flash_mask == 0x0000); /* flash updated! */

        /* Delete slot 57 and chip returns error 70 -> reject */
        publish_count = 0;
        mock_flash_mask = 0x0002; /* slot 57 active again */
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":57,\"u\":2003,\"act_id\":3}");
        app_scene_delete(req);
        cJSON_Delete(req);

        app_scene_on_delete_result(70, 57);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"code\":70") != NULL);
        printf("  PASS: Test 6 - Single slot delete success (33 and 51) & fail (70)\n");
    }

    /* TEST 7: Delete All (cmd_id 255) - Sequence 56..65 with 33/51 producing 1 MQTT response */
    {
        reset_mocks();
        mock_flash_mask = 0x0005; /* slots 56, 58 active */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":255,\"u\":3001,\"act_id\":7}");
        app_scene_delete(req);
        cJSON_Delete(req);

        assert(app_scene_is_pending() == true);
        assert(mock_cwsl_delete_call_count == 1);
        assert(mock_cwsl_delete_last_cmd_id == 56);
        assert(publish_count == 0);

        /* Step 56: returns 33 */
        app_scene_on_delete_result(33, 56);
        assert(publish_count == 0);
        assert(mock_cwsl_delete_call_count == 2);
        assert(mock_cwsl_delete_last_cmd_id == 57);
        assert(mock_flash_mask == 0x0004); /* slot 56 cleared in flash immediately */

        /* Step 57: returns 51 (empty) */
        app_scene_on_delete_result(51, 57);
        assert(publish_count == 0);
        assert(mock_cwsl_delete_call_count == 3);
        assert(mock_cwsl_delete_last_cmd_id == 58);

        /* Step 58..69 */
        for (uint8_t c = 58; c <= 69; c++) {
            app_scene_on_delete_result((c == 58) ? 33 : 51, c);
            assert(publish_count == 0);
            assert(mock_cwsl_delete_last_cmd_id == c + 1);
        }

        /* Step 70 (final) */
        app_scene_on_delete_result(51, 70);
        assert(publish_count == 1);
        assert(app_scene_is_pending() == false);
        assert(strstr(last_published, "\"cmd_id\":255") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"u\":3001") != NULL);
        assert(strstr(last_published, "\"act_id\":7") != NULL);
        assert(mock_flash_mask == 0);
        printf("  PASS: Test 7 - Delete all 255 sequence 56..70 only 1 MQTT response & flash synced\n");
    }

    /* TEST 7b: Delete All walks to 70 and clears bits 10..14 (Review Focus 4) */
    {
        reset_mocks();
        mock_flash_mask = 0x4005; /* slots 56, 58 and 70 active */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":255,\"u\":3101,\"act_id\":2}");
        app_scene_delete(req);
        cJSON_Delete(req);
        assert(app_scene_is_pending() == true);
        assert(mock_cwsl_delete_last_cmd_id == 56);

        for (uint8_t c = 56; c <= 70; c++) {
            assert(publish_count == 0);
            assert(mock_cwsl_delete_last_cmd_id == c);
            app_scene_on_delete_result(((c == 56) || (c == 58) || (c == 70)) ? 33 : 51, c);
        }
        assert(mock_cwsl_delete_last_cmd_id == 70);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"cmd_id\":255") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(app_scene_is_pending() == false);
        assert(mock_flash_mask == 0);
        printf("  PASS: Test 7b - Delete all clears bit 14 (cmd_id 70)\n");
    }

    /* TEST 8: Concurrency Protection - New request during Delete All (255) must reject 67 and NOT abort Delete All */
    {
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();

        /* Start Delete All */
        cJSON *req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":255,\"u\":5000,\"act_id\":1}");
        app_scene_delete(req);
        cJSON_Delete(req);
        assert(app_scene_is_pending() == true);

        /* Another delete arrives concurrently */
        cJSON *bad_req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":56,\"u\":9999,\"act_id\":9}");
        app_scene_delete(bad_req);
        cJSON_Delete(bad_req);

        /* Must reject with 67 and NOT kill delete-all */
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":67") != NULL);
        assert(strstr(last_published, "\"u\":9999") != NULL);
        assert(app_scene_is_pending() == true); /* delete-all still active! */

        /* Another add arrives concurrently */
        publish_count = 0;
        cJSON *bad_add = cJSON_Parse("{\"fanoem\":227,\"cmd_id\":255,\"u\":7777,\"act_id\":3,\"respond_id\":47}");
        app_scene_add(bad_add);
        cJSON_Delete(bad_add);

        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":67") != NULL);
        assert(strstr(last_published, "\"u\":7777") != NULL);
        assert(app_scene_is_pending() == true); /* delete-all still active! */

        /* Query arrives concurrently -> must also reject 67 */
        publish_count = 0;
        cJSON *bad_query = cJSON_Parse("{\"fanoem\":229,\"u\":6666,\"act_id\":2}");
        app_scene_query(bad_query);
        cJSON_Delete(bad_query);

        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":67") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"u\":6666") != NULL);
        assert(app_scene_is_pending() == true); /* delete-all still active! */

        /* Now complete the delete all */
        publish_count = 0;
        for (uint8_t c = 56; c <= 70; c++) {
            app_scene_on_delete_result(51, c);
        }
        assert(publish_count == 1);
        assert(strstr(last_published, "\"cmd_id\":255") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"u\":5000") != NULL); /* correct original uid preserved! */
        assert(app_scene_is_pending() == false);
        printf("  PASS: Test 8 - Concurrency rejection 67 preserves delete-all session\n");
    }

    /* TEST 9: Query Scenes (cmd 229) */
    {
        reset_mocks();
        mock_flash_mask = 0x0005; /* slots 56, 58 */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":229,\"u\":4001,\"act_id\":9}");
        app_scene_query(req);
        cJSON_Delete(req);

        assert(publish_count == 1);
        assert(strstr(last_published, "\"scenes\":[56,58]") != NULL);
        assert(strstr(last_published, "\"total\":2") != NULL);
        assert(strstr(last_published, "\"u\":4001") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);

        /* Query with action field -> reject 64 */
        publish_count = 0;
        req = cJSON_Parse("{\"fanoem\":229,\"action\":6,\"u\":4002,\"act_id\":0}");
        app_scene_query(req);
        cJSON_Delete(req);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":64") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"u\":4002") != NULL);
        printf("  PASS: Test 9 - Query scenes valid and reject\n");
    }

    /* TEST 10: Delete All aborts on system error (52) and syncs flash */
    {
        reset_mocks();
        mock_flash_mask = 0x03FF; /* all 10 slots */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":255,\"u\":6001,\"act_id\":0}");
        app_scene_delete(req);
        cJSON_Delete(req);

        /* Slot 56 deleted */
        app_scene_on_delete_result(33, 56);
        /* Slot 57 deleted */
        app_scene_on_delete_result(33, 57);

        /* Slot 58 times out (52) */
        publish_count = 0;
        app_scene_on_delete_result(52, 58);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":52") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"cmd_id\":255") != NULL);
        assert(app_scene_is_pending() == false);
        /* Bits 0 and 1 were cleared in flash mask */
        assert((mock_flash_mask & 0x0003) == 0);
        assert((mock_flash_mask & 0x0004) != 0); /* bit 2 (slot 58) preserved */
        printf("  PASS: Test 10 - Delete all aborts on system error and syncs flash\n");
    }

    /* TEST 11: Delete Scene validation - Missing u or act_id must reject 65 */
    {
        reset_mocks();
        /* Missing u */
        cJSON *req1 = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":56,\"act_id\":0}");
        app_scene_delete(req1);
        cJSON_Delete(req1);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":65") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);

        /* Missing act_id */
        publish_count = 0;
        cJSON *req2 = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":56,\"u\":100}");
        app_scene_delete(req2);
        cJSON_Delete(req2);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":65") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);

        /* Missing cmd_id */
        publish_count = 0;
        cJSON *req3 = cJSON_Parse("{\"fanoem\":228,\"u\":100,\"act_id\":0}");
        app_scene_delete(req3);
        cJSON_Delete(req3);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":65") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);
        printf("  PASS: Test 11 - Delete scene rejects code 65 when missing u or act_id\n");
    }

    /* Test 12: app_scene_clear_all wipes RAM mask and Flash storage */
    {
        mock_flash_mask = 0x0005U; /* scenes 1 and 3 active */
        app_scene_init();
        assert(app_scene_is_active(1U) == true);
        assert(app_scene_is_active(3U) == true);
        assert(app_scene_is_cmd_id_active(56U) == true);
        assert(app_scene_is_cmd_id_active(58U) == true);

        app_scene_clear_all();
        assert(app_scene_is_active(1U) == false);
        assert(app_scene_is_active(3U) == false);
        assert(app_scene_is_cmd_id_active(56U) == false);
        assert(app_scene_is_cmd_id_active(58U) == false);
        assert(mock_flash_mask == 0U);
        assert(app_scene_is_pending() == false);
        printf("  PASS: Test 12 - app_scene_clear_all resets RAM mask and saves 0 to flash\n");
    }

    /* Test 13: VOICEBOX (voiceboxv1) scene add, query, and delete */
    {
        reset_mocks();
        device_type = 10; /* voiceboxv1 */
        app_scene_init();

        /* Add scene for voiceboxv1 */
        cJSON *add_req = cJSON_Parse("{\"voiceboxv1\":227,\"cmd_id\":255,\"u\":101,\"act_id\":1,\"respond_id\":47}");
        assert(add_req != NULL);
        app_scene_add(add_req);
        cJSON_Delete(add_req);

        assert(mock_cwsl_learn_last_cmd_id == 56);
        assert(publish_count == 0);

        /* Complete learning */
        app_scene_on_learn_result(32, 56);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"voiceboxv1\":227") != NULL);
        assert(strstr(last_published, "\"cmd_id\":56") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(app_scene_is_active(1) == true);

        /* Query scene for voiceboxv1 */
        publish_count = 0;
        cJSON *query_req = cJSON_Parse("{\"voiceboxv1\":229,\"u\":102,\"act_id\":2}");
        assert(query_req != NULL);
        app_scene_query(query_req);
        cJSON_Delete(query_req);

        assert(publish_count == 1);
        assert(strstr(last_published, "\"voiceboxv1\":229") != NULL);
        assert(strstr(last_published, "\"scenes\":[56]") != NULL);
        assert(strstr(last_published, "\"total\":1") != NULL);

        /* Delete scene for voiceboxv1 */
        publish_count = 0;
        cJSON *del_req = cJSON_Parse("{\"voiceboxv1\":228,\"cmd_id\":56,\"u\":103,\"act_id\":3}");
        assert(del_req != NULL);
        app_scene_delete(del_req);
        cJSON_Delete(del_req);

        assert(mock_cwsl_delete_last_cmd_id == 56);
        app_scene_on_delete_result(33, 56);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"voiceboxv1\":228") != NULL);
        assert(strstr(last_published, "\"cmd_id\":56") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(app_scene_is_active(1) == false);

        device_type = 0; /* restore */
        printf("  PASS: Test 13 - voiceboxv1 scene operations with voiceboxv1 MQTT key\n");
    }

    /* TEST 14: 15 scenes (cmd_id 56..70) */
    {
        /* (a) bit 14 -> scene 15 / cmd_id 70 */
        reset_mocks();
        mock_flash_mask = 0x4000;
        app_scene_init();
        assert(app_scene_is_active(15) == true);
        assert(app_scene_is_cmd_id_active(70) == true);

        /* (b) legacy 10-bit mask keeps scenes 1..10, next add gets 66 */
        reset_mocks();
        mock_flash_mask = 0x03FF;
        app_scene_init();
        assert(app_scene_is_active(10) == true);
        assert(app_scene_is_active(11) == false);
        cJSON *add_req = cJSON_Parse("{\"fanoem\":227,\"cmd_id\":255,\"u\":1,\"act_id\":0,\"respond_id\":47}");
        app_scene_add(add_req);
        cJSON_Delete(add_req);
        assert(mock_cwsl_learn_last_cmd_id == 66);

        /* (c) query with all 15 scenes lists 56..70, total 15 */
        reset_mocks();
        mock_flash_mask = 0x7FFF;
        app_scene_init();
        cJSON *query_req = cJSON_Parse("{\"fanoem\":229,\"u\":1,\"act_id\":0}");
        app_scene_query(query_req);
        cJSON_Delete(query_req);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"scenes\":[56,57,58,59,60,61,62,63,64,65,66,67,68,69,70]") != NULL);
        assert(strstr(last_published, "\"total\":15") != NULL);

        /* (d) add when all 15 are used is rejected */
        reset_mocks();
        mock_flash_mask = 0x7FFF;
        app_scene_init();
        cJSON *full_req = cJSON_Parse("{\"fanoem\":227,\"cmd_id\":255,\"u\":1,\"act_id\":0,\"respond_id\":47}");
        app_scene_add(full_req);
        cJSON_Delete(full_req);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"code\":50") != NULL);
        printf("  PASS: Test 14 - 15 scenes\n");
    }

    /* TEST 15: respond_id validation */
    {
        reset_mocks();
        cJSON *req = cJSON_Parse("{\"fanoem\":227,\"u\":1,\"cmd_id\":255,\"act_id\":0,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(mock_cwsl_learn_call_count == 1);
        assert(mock_cwsl_learn_respond_id == 47);
        assert(publish_count == 0);

        const int good[] = {0, 1, 2, 3, 11, 19, 40, 47, 50, 68, 70, 71};
        for (size_t i = 0; i < sizeof(good) / sizeof(good[0]); i++) {
            char buf[112];
            reset_mocks();
            snprintf(buf, sizeof(buf), "{\"fanoem\":227,\"u\":1,\"cmd_id\":255,\"act_id\":0,\"respond_id\":%d}", good[i]);
            req = cJSON_Parse(buf);
            app_scene_add(req);
            cJSON_Delete(req);
            assert(mock_cwsl_learn_call_count == 1);
            assert(mock_cwsl_learn_respond_id == good[i]);
            assert(publish_count == 0);
        }

        const char *bad[] = {"4", "10", "20", "39", "48", "49", "69", "72", "99", "255", "256", "-1",
                             "47.5", "\"47\"", "true", "null"};
        for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
            char buf[112];
            reset_mocks();
            snprintf(buf, sizeof(buf), "{\"fanoem\":227,\"u\":1,\"cmd_id\":255,\"act_id\":0,\"respond_id\":%s}", bad[i]);
            req = cJSON_Parse(buf);
            assert(req != NULL);
            app_scene_add(req);
            cJSON_Delete(req);
            assert(publish_count == 1);
            assert(strstr(last_published, "\"result\":0") != NULL);
            assert(strstr(last_published, "\"code\":64") != NULL);
            assert(mock_cwsl_learn_call_count == 0);
        }
        printf("  PASS: Test 15 - respond_id validation\n");
    }

    /* TEST 16: 228 on inactive slot -> 65 */
    {
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        cJSON *req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":57,\"u\":4,\"act_id\":1}");
        app_scene_delete(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":228,\"cmd_id\":57,\"u\":4,\"act_id\":1,\"result\":0,\"code\":65}") == 0);
        assert(mock_cwsl_delete_call_count == 0);

        reset_mocks();
        mock_flash_mask = 0x0400;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":66,\"u\":4,\"act_id\":1}");
        app_scene_delete(req);
        cJSON_Delete(req);
        assert(mock_cwsl_delete_last_cmd_id == 66);
        printf("  PASS: Test 16 - 228 inactive\n");
    }

    /* TEST 17: 227 add/edit with respond_id */
    {
        cJSON *req;
        char buf[160];
        char expect[200];
        const char *rid_bad[] = {"9", "\"47\"", "47.5", "-1", "256", "null"};
        const char *rid_bad_echo[] = {"9", "0", "0", "0", "0", "0"};
        const unsigned cmds[] = {255U, 56U};

        device_type = 0;

        /* 1. add success */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":1001,\"cmd_id\":255,\"act_id\":5,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(mock_cwsl_learn_call_count == 1);
        assert(mock_cwsl_learn_last_cmd_id == 57);
        assert(mock_cwsl_learn_respond_id == 47);
        assert(publish_count == 0);
        app_scene_on_learn_result(32U, 57U);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":57,\"respond_id\":47,\"u\":1001,\"act_id\":5,\"result\":1}") == 0);

        /* 2. learn failure */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":1001,\"cmd_id\":255,\"act_id\":5,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);
        app_scene_on_learn_result(48U, 57U);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":57,\"respond_id\":47,\"u\":1001,\"act_id\":5,\"result\":0,\"code\":48}") == 0);

        /* 3. full */
        reset_mocks();
        mock_flash_mask = 0x7FFF;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":1,\"cmd_id\":255,\"act_id\":0,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":255,\"respond_id\":47,\"u\":1,\"act_id\":0,\"result\":0,\"code\":50}") == 0);

        /* 4. missing cmd_id */
        reset_mocks();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":1,\"act_id\":0,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":0,\"respond_id\":47,\"u\":1,\"act_id\":0,\"result\":0,\"code\":64}") == 0);
        assert(mock_cwsl_learn_call_count == 0);
        assert(mock_cwsl_resp_set_call_count == 0);

        /* 5. missing respond_id, value only */
        reset_mocks();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":1,\"cmd_id\":255,\"act_id\":0}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":255,\"respond_id\":0,\"u\":1,\"act_id\":0,\"result\":0,\"code\":64}") == 0);
        assert(mock_cwsl_learn_call_count == 0);
        reset_mocks();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":1,\"cmd_id\":255,\"act_id\":0,\"value\":47}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":255,\"respond_id\":0,\"u\":1,\"act_id\":0,\"result\":0,\"code\":64}") == 0);
        assert(mock_cwsl_learn_call_count == 0);

        /* 6. bad respond_id for add and edit */
        for (size_t c = 0; c < 2; c++) {
            for (size_t i = 0; i < sizeof(rid_bad) / sizeof(rid_bad[0]); i++) {
                reset_mocks();
                mock_flash_mask = 0x0001;
                app_scene_init();
                snprintf(buf, sizeof(buf), "{\"fanoem\":227,\"u\":1,\"cmd_id\":%u,\"act_id\":0,\"respond_id\":%s}", cmds[c], rid_bad[i]);
                snprintf(expect, sizeof(expect), "{\"fanoem\":227,\"cmd_id\":%u,\"respond_id\":%s,\"u\":1,\"act_id\":0,\"result\":0,\"code\":64}", cmds[c], rid_bad_echo[i]);
                req = cJSON_Parse(buf);
                assert(req != NULL);
                app_scene_add(req);
                cJSON_Delete(req);
                assert(strcmp(last_published, expect) == 0);
                assert(mock_cwsl_learn_call_count == 0);
                assert(mock_cwsl_resp_set_call_count == 0);
            }
        }

        /* 7. cmd_id out of range */
        {
            const unsigned bad_cmd[] = {55U, 71U, 254U};
            for (size_t i = 0; i < 3; i++) {
                reset_mocks();
                mock_flash_mask = 0x0001;
                app_scene_init();
                snprintf(buf, sizeof(buf), "{\"fanoem\":227,\"u\":1,\"cmd_id\":%u,\"act_id\":0,\"respond_id\":47}", bad_cmd[i]);
                snprintf(expect, sizeof(expect), "{\"fanoem\":227,\"cmd_id\":%u,\"respond_id\":47,\"u\":1,\"act_id\":0,\"result\":0,\"code\":65}", bad_cmd[i]);
                req = cJSON_Parse(buf);
                app_scene_add(req);
                cJSON_Delete(req);
                assert(strcmp(last_published, expect) == 0);
                assert(mock_cwsl_learn_call_count == 0);
                assert(mock_cwsl_resp_set_call_count == 0);
            }
        }

        /* 8. edit success */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":56,\"act_id\":3,\"respond_id\":50}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(mock_cwsl_resp_set_call_count == 1);
        assert(mock_cwsl_resp_set_last_cmd_id == 56);
        assert(mock_cwsl_resp_set_last_respond_id == 50);
        assert(mock_cwsl_learn_call_count == 0);
        assert(publish_count == 0);
        app_scene_on_resp_set_result(0U);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":56,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":1}") == 0);
        assert(mock_flash_mask == 0x0001);

        /* 9. edit failure: chip rejected (64), timeout (52) */
        {
            const unsigned codes[] = {64U, 52U};
            for (size_t i = 0; i < 2; i++) {
                reset_mocks();
                mock_flash_mask = 0x0001;
                app_scene_init();
                req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":56,\"act_id\":3,\"respond_id\":50}");
                app_scene_add(req);
                cJSON_Delete(req);
                app_scene_on_resp_set_result((uint8_t)codes[i]);
                snprintf(expect, sizeof(expect), "{\"fanoem\":227,\"cmd_id\":56,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":0,\"code\":%u}", codes[i]);
                assert(strcmp(last_published, expect) == 0);
                assert(mock_flash_mask == 0x0001);
            }
        }

        /* 10. edit inactive context */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":57,\"act_id\":3,\"respond_id\":50}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":57,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":0,\"code\":65}") == 0);
        assert(mock_cwsl_resp_set_call_count == 0);

        /* 11. edit cannot start (busy) */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        mock_cwsl_resp_set_ret = 67;
        req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":56,\"act_id\":3,\"respond_id\":50}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":56,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":0,\"code\":67}") == 0);

        /* 12. edit during delete-all */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":255,\"u\":9,\"act_id\":0}");
        app_scene_delete(req);
        cJSON_Delete(req);
        assert(app_scene_is_pending() == true);
        req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":56,\"act_id\":3,\"respond_id\":50}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":56,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":0,\"code\":67}") == 0);
        assert(mock_cwsl_resp_set_call_count == 0);
        assert(app_scene_is_pending() == true);

        /* 12a. check order: invalid respond_id (64) before out-of-range cmd_id (65) */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":55,\"act_id\":3,\"respond_id\":9}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strstr(last_published, "\"code\":64") != NULL);
        assert(strstr(last_published, "\"cmd_id\":55,\"respond_id\":9") != NULL);
        assert(mock_cwsl_resp_set_call_count == 0);

        /* 12b. check order during delete-all: out-of-range cmd_id (65) before busy (67) */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":255,\"u\":9,\"act_id\":0}");
        app_scene_delete(req);
        cJSON_Delete(req);
        req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":55,\"act_id\":3,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strstr(last_published, "\"code\":65") != NULL);
        assert(strstr(last_published, "\"cmd_id\":55,\"respond_id\":47") != NULL);
        assert(mock_cwsl_resp_set_call_count == 0);

        /* 12c. check order during delete-all: busy (67) before inactive context (65) */
        reset_mocks();
        mock_flash_mask = 0x0001;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":255,\"u\":9,\"act_id\":0}");
        app_scene_delete(req);
        cJSON_Delete(req);
        req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":57,\"act_id\":3,\"respond_id\":50}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":57,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":0,\"code\":67}") == 0);
        assert(mock_cwsl_resp_set_call_count == 0);

        /* 12d. a failed second edit start keeps the first edit pending */
        reset_mocks();
        mock_flash_mask = 0x0003;
        app_scene_init();
        req = cJSON_Parse("{\"fanoem\":227,\"u\":2,\"cmd_id\":56,\"act_id\":3,\"respond_id\":50}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(mock_cwsl_resp_set_call_count == 1);
        assert(publish_count == 0);
        mock_cwsl_resp_set_ret = 67;
        req = cJSON_Parse("{\"fanoem\":227,\"u\":9,\"cmd_id\":57,\"act_id\":1,\"respond_id\":47}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(strstr(last_published, "\"cmd_id\":57") != NULL && strstr(last_published, "\"code\":67") != NULL);
        app_scene_on_resp_set_result(0U);
        assert(strcmp(last_published, "{\"fanoem\":227,\"cmd_id\":56,\"respond_id\":50,\"u\":2,\"act_id\":3,\"result\":1}") == 0);

        /* 13. voiceboxv1 */
        reset_mocks();
        device_type = 10;
        app_scene_init();
        req = cJSON_Parse("{\"voiceboxv1\":227,\"u\":55519,\"cmd_id\":255,\"act_id\":0,\"respond_id\":1}");
        app_scene_add(req);
        cJSON_Delete(req);
        assert(mock_cwsl_learn_call_count == 1);
        assert(mock_cwsl_learn_respond_id == 1);
        app_scene_on_learn_result(32U, 56U);
        assert(strcmp(last_published, "{\"voiceboxv1\":227,\"cmd_id\":56,\"respond_id\":1,\"u\":55519,\"act_id\":0,\"result\":1}") == 0);
        device_type = 0;
        printf("  PASS: Test 17 - 227 add/edit with respond_id\n");
    }

    printf("\nALL 17 UNIT TEST SCENARIOS PASSED 100%%!\n");
    return 0;
}
