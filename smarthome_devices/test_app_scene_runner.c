#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CJSON_KEY_ACTION "action"
#define CJSON_KEY_USER_ID "u"
#define CJSON_KEY_ACT_ID "act_id"
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
char *gs_dev_type[] = {"fanoem"};
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
uint8_t app_cwsl_start_scene_learn(uint8_t cmd_id, uint32_t user_id, uint8_t act_id) {
    (void)user_id;
    (void)act_id;
    mock_cwsl_learn_last_cmd_id = cmd_id;
    return mock_cwsl_learn_ret;
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
        mock_flash_mask = 0x0005; /* bits 0, 2 active -> slots 46, 48 */
        app_scene_init();
        assert(app_scene_is_active(1) == true);
        assert(app_scene_is_active(2) == false);
        assert(app_scene_is_active(3) == true);
        assert(app_scene_is_active(0) == false);
        assert(app_scene_is_active(11) == false);
        assert(app_scene_is_cmd_id_active(46) == true);
        assert(app_scene_is_cmd_id_active(47) == false);
        assert(app_scene_is_cmd_id_active(48) == true);
        assert(app_scene_is_cmd_id_active(45) == false);
        assert(app_scene_is_cmd_id_active(56) == false);
        assert(app_scene_is_cmd_id_active(255) == false);
        printf("  PASS: Test 1 - Init & bounds\n");
    }

    /* TEST 2: Add Scene - Success & Lowest Free Slot Selection */
    {
        reset_mocks();
        mock_flash_mask = 0x0001; /* bit 0 set (slot 46 used) */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":227,\"u\":1001,\"act_id\":5}");
        app_scene_add(req);
        cJSON_Delete(req);

        /* Should pick bit 1 -> candidate cmd_id 47 */
        assert(mock_cwsl_learn_last_cmd_id == 47);
        assert(publish_count == 0); /* No immediate MQTT ACK */

        /* Callback arrives from FSM: LEARN_OK (32) */
        app_scene_on_learn_result(32, 47);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"cmd_id\":47") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"u\":1001") != NULL);
        assert(strstr(last_published, "\"act_id\":5") != NULL);
        assert(app_scene_is_cmd_id_active(47) == true);
        assert(mock_flash_mask == 0x0003); /* bits 0, 1 now set */
        printf("  PASS: Test 2 - Add scene lowest slot & success\n");
    }

    /* TEST 3: Add Scene - Rejection when 10 slots full (code 71) */
    {
        reset_mocks();
        mock_flash_mask = 0x03FF; /* all 10 slots active */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":227,\"u\":1002,\"act_id\":0}");
        app_scene_add(req);
        cJSON_Delete(req);

        assert(publish_count == 1);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"code\":71") != NULL);
        assert(strstr(last_published, "\"u\":1002") != NULL);
        printf("  PASS: Test 3 - Add scene reject code 71 when full\n");
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
        cJSON *req = cJSON_Parse("{\"fanoem\":227,\"u\":8888,\"act_id\":12}");
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
        mock_flash_mask = 0x0003; /* slots 46, 47 active */
        app_scene_init();

        /* Delete slot 46 */
        cJSON *req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":46,\"u\":2001,\"act_id\":1}");
        app_scene_delete(req);
        cJSON_Delete(req);

        assert(mock_cwsl_delete_last_cmd_id == 46);
        assert(publish_count == 0);

        /* Chip responds with 33 (DELETE_OK) */
        app_scene_on_delete_result(33, 46);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"cmd_id\":46") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"u\":2001") != NULL);
        assert(app_scene_is_cmd_id_active(46) == false);
        assert(app_scene_is_cmd_id_active(47) == true);
        assert(mock_flash_mask == 0x0002);

        /* Delete slot 47 and chip returns 51 (slot empty) -> treated as success */
        publish_count = 0;
        req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":47,\"u\":2002,\"act_id\":2}");
        app_scene_delete(req);
        cJSON_Delete(req);

        app_scene_on_delete_result(51, 47);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"cmd_id\":47") != NULL);
        assert(app_scene_is_cmd_id_active(47) == false); /* bit cleared! */
        assert(mock_flash_mask == 0x0000); /* flash updated! */

        /* Delete slot 47 and chip returns error 70 -> reject */
        publish_count = 0;
        req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":47,\"u\":2003,\"act_id\":3}");
        app_scene_delete(req);
        cJSON_Delete(req);

        app_scene_on_delete_result(70, 47);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"code\":70") != NULL);
        printf("  PASS: Test 6 - Single slot delete success (33 and 51) & fail (70)\n");
    }

    /* TEST 7: Delete All (cmd_id 255) - Sequence 46..55 with 33/51 producing 1 MQTT response */
    {
        reset_mocks();
        mock_flash_mask = 0x0005; /* slots 46, 48 active */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":255,\"u\":3001,\"act_id\":7}");
        app_scene_delete(req);
        cJSON_Delete(req);

        assert(app_scene_is_pending() == true);
        assert(mock_cwsl_delete_call_count == 1);
        assert(mock_cwsl_delete_last_cmd_id == 46);
        assert(publish_count == 0);

        /* Step 46: returns 33 */
        app_scene_on_delete_result(33, 46);
        assert(publish_count == 0);
        assert(mock_cwsl_delete_call_count == 2);
        assert(mock_cwsl_delete_last_cmd_id == 47);
        assert(mock_flash_mask == 0x0004); /* slot 46 cleared in flash immediately */

        /* Step 47: returns 51 (empty) */
        app_scene_on_delete_result(51, 47);
        assert(publish_count == 0);
        assert(mock_cwsl_delete_call_count == 3);
        assert(mock_cwsl_delete_last_cmd_id == 48);

        /* Step 48..54 */
        for (uint8_t c = 48; c <= 54; c++) {
            app_scene_on_delete_result((c == 48) ? 33 : 51, c);
            assert(publish_count == 0);
            assert(mock_cwsl_delete_last_cmd_id == c + 1);
        }

        /* Step 55 (final) */
        app_scene_on_delete_result(51, 55);
        assert(publish_count == 1);
        assert(app_scene_is_pending() == false);
        assert(strstr(last_published, "\"cmd_id\":255") != NULL);
        assert(strstr(last_published, "\"result\":1") != NULL);
        assert(strstr(last_published, "\"u\":3001") != NULL);
        assert(strstr(last_published, "\"act_id\":7") != NULL);
        assert(mock_flash_mask == 0);
        printf("  PASS: Test 7 - Delete all 255 sequence 46..55 only 1 MQTT response & flash synced\n");
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
        cJSON *bad_req = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":46,\"u\":9999,\"act_id\":9}");
        app_scene_delete(bad_req);
        cJSON_Delete(bad_req);

        /* Must reject with 67 and NOT kill delete-all */
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":67") != NULL);
        assert(strstr(last_published, "\"u\":9999") != NULL);
        assert(app_scene_is_pending() == true); /* delete-all still active! */

        /* Another add arrives concurrently */
        publish_count = 0;
        cJSON *bad_add = cJSON_Parse("{\"fanoem\":227,\"u\":7777,\"act_id\":3}");
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
        for (uint8_t c = 46; c <= 55; c++) {
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
        mock_flash_mask = 0x0005; /* slots 46, 48 */
        app_scene_init();

        cJSON *req = cJSON_Parse("{\"fanoem\":229,\"u\":4001,\"act_id\":9}");
        app_scene_query(req);
        cJSON_Delete(req);

        assert(publish_count == 1);
        assert(strstr(last_published, "\"scenes\":[46,48]") != NULL);
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

        /* Slot 46 deleted */
        app_scene_on_delete_result(33, 46);
        /* Slot 47 deleted */
        app_scene_on_delete_result(33, 47);

        /* Slot 48 times out (52) */
        publish_count = 0;
        app_scene_on_delete_result(52, 48);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":52") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);
        assert(strstr(last_published, "\"cmd_id\":255") != NULL);
        assert(app_scene_is_pending() == false);
        /* Bits 0 and 1 were cleared in flash mask */
        assert((mock_flash_mask & 0x0003) == 0);
        assert((mock_flash_mask & 0x0004) != 0); /* bit 2 (slot 48) preserved */
        printf("  PASS: Test 10 - Delete all aborts on system error and syncs flash\n");
    }

    /* TEST 11: Delete Scene validation - Missing u or act_id must reject 65 */
    {
        reset_mocks();
        /* Missing u */
        cJSON *req1 = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":46,\"act_id\":0}");
        app_scene_delete(req1);
        cJSON_Delete(req1);
        assert(publish_count == 1);
        assert(strstr(last_published, "\"code\":65") != NULL);
        assert(strstr(last_published, "\"result\":0") != NULL);

        /* Missing act_id */
        publish_count = 0;
        cJSON *req2 = cJSON_Parse("{\"fanoem\":228,\"cmd_id\":46,\"u\":100}");
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

    printf("\nALL 11 UNIT TEST SCENARIOS PASSED 100%%!\n");
    return 0;
}
