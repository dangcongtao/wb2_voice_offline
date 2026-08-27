/**
 * @file ci13162_ota_manifest.c
 * @brief Bounded HTTP download and parsing for CI13162 OTA manifests.
 */
#include "ci13162_ota_manifest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>

#include "cJSON.h"
#include "ci13162_ota_http.h"
#include "m_ci13162_ota.h"

#define CI13162_OTA_MANIFEST_NAME "manifest.json"

typedef struct {
    ci13162_ota_http_ctx_t http;
    uint8_t http_io[AIIO_CI13162_OTA_HTTP_CHUNK_SIZE];
    char body[AIIO_CI13162_OTA_MANIFEST_MAX + 1U];
} ci13162_ota_manifest_workspace_t;

static int ci13162_ota_manifest_is_alnum(char value)
{
    return (((value >= 'a') && (value <= 'z')) ||
            ((value >= 'A') && (value <= 'Z')) ||
            ((value >= '0') && (value <= '9')))
               ? 1
               : 0;
}

static int ci13162_ota_manifest_host_is_valid(const char *base_url)
{
    const char *port_separator;
    size_t length;
    size_t host_length;
    size_t index;
    size_t label_length = 0U;

    if (base_url == NULL) {
        return 0;
    }
    length = strlen(base_url);
    if (length == 0U) {
        return 0;
    }

    port_separator = strchr(base_url, ':');
    if (port_separator != NULL) {
        return 0;
    }
    host_length = length;

    if ((host_length == 0U) || (host_length >= CI13162_OTA_HTTP_HOST_MAX)) {
        return 0;
    }
    for (index = 0U; index < host_length; index++) {
        char value = base_url[index];

        if (value == '.') {
            if ((label_length == 0U) || (base_url[index - 1U] == '-')) {
                return 0;
            }
            label_length = 0U;
            continue;
        }
        if ((ci13162_ota_manifest_is_alnum(value) == 0) && (value != '-')) {
            return 0;
        }
        if ((label_length == 0U) && (value == '-')) {
            return 0;
        }
        label_length++;
        if (label_length > 63U) {
            return 0;
        }
    }

    return ((label_length > 0U) && (base_url[host_length - 1U] != '-')) ? 1 : 0;
}

static int ci13162_ota_manifest_filename_is_valid(const char *filename)
{
    size_t index;

    if ((filename == NULL) || (filename[0] == '\0') ||
        (strcmp(filename, ".") == 0) ||
        (strcmp(filename, "..") == 0)) {
        return 0;
    }
    for (index = 0U; filename[index] != '\0'; index++) {
        char value = filename[index];

        if ((ci13162_ota_manifest_is_alnum(value) == 0) &&
            (value != '.') && (value != '_') && (value != '-')) {
            return 0;
        }
        if (index >= CI13162_OTA_HTTP_PATH_MAX) {
            return 0;
        }
    }
    return 1;
}

static int ci13162_ota_manifest_directory_is_valid(const char *path,
                                                    size_t directory_length)
{
    size_t index;
    size_t segment_start;

    if ((path == NULL) || (directory_length == 0U) ||
        (path[0] != '/') || (path[directory_length - 1U] != '/')) {
        return 0;
    }

    segment_start = 1U;
    for (index = 1U; index < directory_length; index++) {
        unsigned char value = (unsigned char)path[index];

        if ((value <= 0x20U) || (value >= 0x7FU) || (value == '\\')) {
            return 0;
        }
        if (value == '/') {
            size_t segment_length = index - segment_start;

            if ((segment_length == 0U) ||
                ((segment_length == 1U) && (path[segment_start] == '.')) ||
                ((segment_length == 2U) &&
                 (path[segment_start] == '.') &&
                 (path[segment_start + 1U] == '.'))) {
                return 0;
            }
            segment_start = index + 1U;
        }
    }
    return 1;
}

static int32_t ci13162_ota_manifest_build_url(const char *base_url,
                                               const char *directory,
                                               size_t directory_length,
                                               const char *filename,
                                               char *output,
                                               size_t output_size)
{
    ci13162_ota_http_url_t parsed;
    int written;

    if ((base_url == NULL) || (directory == NULL) || (filename == NULL) ||
        (output == NULL) || (output_size == 0U)) {
        return -1;
    }

    written = snprintf(output,
                       output_size,
                       "http://%s%.*s%s",
                       base_url,
                       (int)directory_length,
                       directory,
                       filename);
    if ((written <= 0) || ((size_t)written >= output_size) ||
        !m_ci13162_ota_url_is_valid(output) ||
        (ci13162_ota_http_parse_url(output, &parsed) != CI13162_OTA_HTTP_OK)) {
        output[0] = '\0';
        return -1;
    }
    return 0;
}

static int32_t ci13162_ota_manifest_resolve(const char *manifest_url,
                                            const char *body,
                                            size_t body_length,
                                            ci13162_ota_manifest_urls_t *out)
{
    ci13162_ota_http_url_t parsed_manifest;
    cJSON *root = NULL;
    cJSON *base_url_item;
    cJSON *app_item;
    cJSON *agent_item;
    cJSON *erase_nv_item;
    const char *parse_end = NULL;
    const char *body_end;
    size_t path_length;
    size_t slash_index;
    size_t manifest_name_length = sizeof(CI13162_OTA_MANIFEST_NAME) - 1U;
    int32_t result = -1;

    if ((manifest_url == NULL) || (body == NULL) || (out == NULL) ||
        (body_length == 0U) ||
        (body_length > AIIO_CI13162_OTA_MANIFEST_MAX)) {
        return -1;
    }
    memset(out, 0, sizeof(*out));

    if (!m_ci13162_ota_url_is_valid(manifest_url) ||
        (ci13162_ota_http_parse_url(manifest_url, &parsed_manifest) !=
         CI13162_OTA_HTTP_OK)) {
        os_trace("[CI13162][MANIFEST] Invalid manifest URL\r\n");
        return -1;
    }

    path_length = strcspn(parsed_manifest.path, "?");
    if (path_length <= manifest_name_length) {
        os_trace("[CI13162][MANIFEST] Invalid manifest path\r\n");
        return -1;
    }
    slash_index = path_length;
    while ((slash_index > 0U) && (parsed_manifest.path[slash_index - 1U] != '/')) {
        slash_index--;
    }
    if ((slash_index == 0U) ||
        ((path_length - slash_index) != manifest_name_length) ||
        (memcmp(parsed_manifest.path + slash_index,
                CI13162_OTA_MANIFEST_NAME,
                manifest_name_length) != 0) ||
        (ci13162_ota_manifest_directory_is_valid(parsed_manifest.path,
                                                 slash_index) == 0)) {
        os_trace("[CI13162][MANIFEST] Expected */manifest.json\r\n");
        return -1;
    }

    if (body[body_length] != '\0') {
        os_trace("[CI13162][MANIFEST] Body is not terminated\r\n");
        return -1;
    }
    root = cJSON_ParseWithOpts(body, &parse_end, 0);
    if ((root == NULL) || (root->type != cJSON_Object) || (parse_end == NULL)) {
        os_trace("[CI13162][MANIFEST] JSON parse failed\r\n");
        goto CLEANUP;
    }
    body_end = body + body_length;
    while ((parse_end < body_end) &&
           ((*parse_end == ' ') || (*parse_end == '\t') ||
            (*parse_end == '\r') || (*parse_end == '\n'))) {
        parse_end++;
    }
    if (parse_end != body_end) {
        os_trace("[CI13162][MANIFEST] JSON has trailing data\r\n");
        goto CLEANUP;
    }

    base_url_item = cJSON_GetObjectItem(root, "base_url");
    app_item = cJSON_GetObjectItem(root, "app");
    agent_item = cJSON_GetObjectItem(root, "agent");
    if ((base_url_item == NULL) ||
        (app_item == NULL) ||
        (agent_item == NULL) ||
        (base_url_item->string == NULL) ||
        (app_item->string == NULL) ||
        (agent_item->string == NULL) ||
        (strcmp(base_url_item->string, "base_url") != 0) ||
        (strcmp(app_item->string, "app") != 0) ||
        (strcmp(agent_item->string, "agent") != 0) ||
        (base_url_item->type != cJSON_String) ||
        (app_item->type != cJSON_String) ||
        (agent_item->type != cJSON_String) ||
        (base_url_item->valuestring == NULL) ||
        (app_item->valuestring == NULL) ||
        (agent_item->valuestring == NULL)) {
        os_trace("[CI13162][MANIFEST] Required fields are missing\r\n");
        goto CLEANUP;
    }
    if ((ci13162_ota_manifest_host_is_valid(base_url_item->valuestring) == 0) ||
        (ci13162_ota_manifest_filename_is_valid(app_item->valuestring) == 0) ||
        (ci13162_ota_manifest_filename_is_valid(agent_item->valuestring) == 0)) {
        os_trace("[CI13162][MANIFEST] Invalid host or filename\r\n");
        goto CLEANUP;
    }

    erase_nv_item = cJSON_GetObjectItem(root, "erase_nv");
    if (erase_nv_item == NULL) {
        out->erase_nv = 0U;
    } else if ((erase_nv_item->string == NULL) ||
               (strcmp(erase_nv_item->string, "erase_nv") != 0) ||
               ((erase_nv_item->type != cJSON_True) &&
                (erase_nv_item->type != cJSON_False))) {
        os_trace("[CI13162][MANIFEST] Invalid erase_nv\r\n");
        goto CLEANUP;
    } else {
        out->erase_nv = (erase_nv_item->type == cJSON_True) ? 1U : 0U;
    }

    if ((ci13162_ota_manifest_build_url(base_url_item->valuestring,
                                        parsed_manifest.path,
                                        slash_index,
                                        app_item->valuestring,
                                        out->firmware_url,
                                        sizeof(out->firmware_url)) != 0) ||
        (ci13162_ota_manifest_build_url(base_url_item->valuestring,
                                        parsed_manifest.path,
                                        slash_index,
                                        agent_item->valuestring,
                                        out->agent_url,
                                        sizeof(out->agent_url)) != 0)) {
        os_trace("[CI13162][MANIFEST] Generated URL is invalid\r\n");
        goto CLEANUP;
    }

    result = 0;

CLEANUP:
    cJSON_Delete(root);
    if (result != 0) {
        memset(out, 0, sizeof(*out));
    }
    return result;
}

int32_t ci13162_ota_manifest_fetch(const char *manifest_url,
                                   ci13162_ota_manifest_urls_t *out)
{
    ci13162_ota_manifest_workspace_t *workspace;
    uint32_t content_length;
    int32_t http_result;
    int32_t result = -1;

    if ((manifest_url == NULL) || (out == NULL)) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    if (!m_ci13162_ota_url_is_valid(manifest_url)) {
        os_trace("[CI13162][MANIFEST] Unsupported manifest URL\r\n");
        return -1;
    }

    os_trace("[CI13162][MANIFEST] Allocating workspace, heap %u\r\n",
             (unsigned int)xPortGetFreeHeapSize());
    workspace = (ci13162_ota_manifest_workspace_t *)calloc(1U, sizeof(*workspace));
    if (workspace == NULL) {
        os_trace("[CI13162][MANIFEST] Workspace allocation failed\r\n");
        return -1;
    }
    workspace->http.sock_fd = -1;

    os_trace("[CI13162][MANIFEST] URL %s\r\n", manifest_url);
    http_result = ci13162_ota_http_open(&workspace->http,
                                        manifest_url,
                                        workspace->http_io,
                                        sizeof(workspace->http_io),
                                        AIIO_CI13162_OTA_HTTP_TIMEOUT_MS);
    if (http_result != CI13162_OTA_HTTP_OK) {
        os_trace("[CI13162][MANIFEST] HTTP open failed (%d)\r\n",
                 (int)http_result);
        goto CLEANUP;
    }

    content_length = ci13162_ota_http_content_length(&workspace->http);
    if ((content_length == 0U) ||
        (content_length > AIIO_CI13162_OTA_MANIFEST_MAX)) {
        os_trace("[CI13162][MANIFEST] Invalid body length %u\r\n",
                 (unsigned int)content_length);
        goto CLEANUP;
    }
    http_result = ci13162_ota_http_read_exact(&workspace->http,
                                               (uint8_t *)workspace->body,
                                               (size_t)content_length);
    if ((http_result != CI13162_OTA_HTTP_OK) ||
        !ci13162_ota_http_is_complete(&workspace->http)) {
        os_trace("[CI13162][MANIFEST] Body read failed (%d)\r\n",
                 (int)http_result);
        goto CLEANUP;
    }
    workspace->body[content_length] = '\0';
    ci13162_ota_http_close(&workspace->http);

    result = ci13162_ota_manifest_resolve(manifest_url,
                                          workspace->body,
                                          (size_t)content_length,
                                          out);

CLEANUP:
    ci13162_ota_http_close(&workspace->http);
    free(workspace);
    os_trace("[CI13162][MANIFEST] Workspace released, heap %u\r\n",
             (unsigned int)xPortGetFreeHeapSize());
    return result;
}

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
static int ci13162_ota_manifest_expect(const char *manifest_url,
                                       const char *body,
                                       int expect_success)
{
    ci13162_ota_manifest_urls_t urls;
    int success = (ci13162_ota_manifest_resolve(manifest_url,
                                                body,
                                                strlen(body),
                                                &urls) == 0)
                      ? 1
                      : 0;

    return (success == expect_success) ? 0 : 1;
}

int ci13162_ota_manifest_self_test(void)
{
    static const char golden_body[] =
        "{\"base_url\":\"fwm.hunonicpro.com\","
        "\"app\":\"Firmware_V26.08.13.bin\","
        "\"agent\":\"ci13lc_agent_from_tool_padded_0x5000.bin\"}";
    static const char erase_nv_true_body[] =
        "{\"base_url\":\"fwm.hunonicpro.com\","
        "\"app\":\"Firmware_V26.08.13.bin\","
        "\"agent\":\"ci13lc_agent_from_tool_padded_0x5000.bin\","
        "\"erase_nv\":true}";
    static const char erase_nv_false_body[] =
        "{\"base_url\":\"fwm.hunonicpro.com\","
        "\"app\":\"Firmware_V26.08.13.bin\","
        "\"agent\":\"ci13lc_agent_from_tool_padded_0x5000.bin\","
        "\"erase_nv\":false}";
    static const char erase_nv_wrong_type_body[] =
        "{\"base_url\":\"fwm.hunonicpro.com\","
        "\"app\":\"Firmware_V26.08.13.bin\","
        "\"agent\":\"ci13lc_agent_from_tool_padded_0x5000.bin\","
        "\"erase_nv\":\"true\"}";
    static const char golden_firmware[] =
        "http://fwm.hunonicpro.com/f/6258/Firmware_V26.08.13.bin";
    static const char golden_agent[] =
        "http://fwm.hunonicpro.com/f/6258/ci13lc_agent_from_tool_padded_0x5000.bin";
    ci13162_ota_manifest_urls_t urls;
    int failed = 0;
    int total = 0;

    total++;
    if ((ci13162_ota_manifest_resolve(
             "http://fwm.hunonicpro.com/f/6258/manifest.json",
             golden_body,
             strlen(golden_body),
             &urls) != 0) ||
        (strcmp(urls.firmware_url, golden_firmware) != 0) ||
        (strcmp(urls.agent_url, golden_agent) != 0) ||
        (urls.erase_nv != 0U)) {
        failed++;
    }

    total++;
    if ((ci13162_ota_manifest_resolve(
             "http://fwm.hunonicpro.com/f/6258/manifest.json",
             erase_nv_true_body,
             strlen(erase_nv_true_body),
             &urls) != 0) ||
        (urls.erase_nv != 1U)) {
        failed++;
    }

    total++;
    if ((ci13162_ota_manifest_resolve(
             "http://fwm.hunonicpro.com/f/6258/manifest.json",
             erase_nv_false_body,
             strlen(erase_nv_false_body),
             &urls) != 0) ||
        (urls.erase_nv != 0U)) {
        failed++;
    }

#define CI13162_OTA_MANIFEST_TEST(url, json, expected) \
    do {                                                 \
        total++;                                         \
        failed += ci13162_ota_manifest_expect((url),      \
                                               (json),     \
                                               (expected));\
    } while (0)

    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        "{\"base_url\":\"fwm.hunonicpro.com\",\"app\":\"a.bin\"}",
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        "{\"base_url\":\"fwm.hunonicpro.com\",\"app\":1,\"agent\":\"b.bin\"}",
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        "{\"base_url\":\"\",\"app\":\"a.bin\",\"agent\":\"b.bin\"}",
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        "{\"base_url\":\"fwm.hunonicpro.com\",\"app\":\"a.bin\",\"agent\":\"b.bin\"}x",
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        "{\"base_url\":\"http://fwm.hunonicpro.com\",\"app\":\"a.bin\",\"agent\":\"b.bin\"}",
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        "{\"base_url\":\"fwm.hunonicpro.com/path\",\"app\":\"a.bin\",\"agent\":\"b.bin\"}",
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        "{\"base_url\":\"fwm.hunonicpro.com\",\"app\":\"../a.bin\",\"agent\":\"b.bin\"}",
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        "{\"base_url\":\"fwm.hunonicpro.com\",\"app\":\"bad name.bin\",\"agent\":\"b.bin\"}",
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/not-manifest.json",
        golden_body,
        0);
    CI13162_OTA_MANIFEST_TEST(
        "http://fwm.hunonicpro.com/f/6258/manifest.json",
        erase_nv_wrong_type_body,
        0);

#undef CI13162_OTA_MANIFEST_TEST

    os_trace("[CI13162][TEST] manifest total=%d failed=%d\r\n", total, failed);
    return failed;
}
#endif
