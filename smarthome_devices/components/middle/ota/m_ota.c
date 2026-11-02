#include "m_ota.h"
#include "m_ci13162_ota.h"

/**
 * @file main.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-10-09
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <lwip/tcpip.h>
#include <wifi_mgmr_ext.h>
#include <hal_wifi.h>

#include <hal_boot2.h>
#include "http_parser.h"
#include "ota_parse.h"
#include "ai_ota.h"
#include "ota_config.h"
#include "ota_hal.h"
#include "bl_sys.h"
#include <stdbool.h>
#include "m_database.h"
#include "m_fan.h"
#include "flash_api.h"
#include "system_typedef.h"

// #define url "https://chencong--test.oss-cn-beijing.aliyuncs.com/test.bin"

int axk_hal_user_ota_update(char *url);

static ota_parame ota_param;
static int ota_type;
static char ota_host[256];
static char ota_resource[256];
static volatile TaskHandle_t s_ota_task_handle = NULL;
uint8_t is_ota = 0;

static void app_reboot(void);
static void app_ota_set_reboot(_Bool ota_result);

bool m_ota_has_task_finished(void)
{
    TaskHandle_t handle = (TaskHandle_t)s_ota_task_handle;
    if (handle != NULL)
    {
        eTaskState state = eTaskGetState(handle);
        if (state == eDeleted)
        {
            s_ota_task_handle = NULL;
            return true;
        }
    }
    return false;
}

static void _ota_task(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    if (ota_type == AT_OTA_MODE_HTTP)
    {
        ai_http_update_ota(&ota_param);
    }
    else
    {
        ai_https_update_ota(&ota_param);
    }

    /* SDK's ai_http_update_ota / ai_https_update_ota calls vTaskDelete(NULL)
     * upon failure or reboots via reboot_cb upon success. If it ever returns,
     * handle failure here as fallback. Unexpected task termination is detected
     * via m_ota_has_task_finished() in button_scan_task.
     */
    if (is_ota != STATE_OTA_SUCCESS)
    {
        os_trace("OTA task failed, rebooting to restore state\r\n");
        app_ota_set_reboot(false);
    }

    vTaskDelete(NULL);
}

static void app_reboot(void)
{
    bl_sys_reset_por();
}

static void app_ota_set_reboot(_Bool ota_result)
{
    printf("app_ota_set_reboot %u\r\n", ota_result);
    if (ota_result == true)
    {
        is_ota = STATE_OTA_SUCCESS;
    }
    else
    {
        is_ota = STATE_OTA_STOP;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    app_reboot();
}

int axk_hal_user_ota_update(char *url)
{
    int port;
    char schema[8] = {0};
    struct http_parser_url purl;
    HALPartition_Entry_Config otaEntry;
    uint16_t length;

    if (url == NULL || url[0] == '\0')
    {
        printf("[OTA] url is null or empty\r\n");
        return -1;
    }

    if (hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &otaEntry))
    {
        printf("[OTA] get otaEntry fail\r\n");
        return -1;
    }

    http_parser_url_init(&purl);
    length = strlen(url);

    int parser_status = http_parser_parse_url(url, length, 0, &purl);

    if (parser_status != 0)
    {
        printf("[OTA] Error parse url:%s\r\n", url);
        return -1;
    }

    memset(ota_host, 0, sizeof(ota_host));
    memset(ota_resource, 0, sizeof(ota_resource));

    if (purl.field_data[UF_SCHEMA].len > 8)
    {
        printf("[OTA] schema ovfl \r\n");
        return -1;
    }
    memcpy(schema, url + purl.field_data[UF_SCHEMA].off, purl.field_data[UF_SCHEMA].len);
    if (strcasecmp(schema, "http") == 0)
    {
        ota_type = AT_OTA_MODE_HTTP;
        port = 80;
    }
    else if (strcasecmp(schema, "https") == 0)
    {
        ota_type = AT_OTA_MODE_HTTPS;
        port = 443;
    }
    else
    {
        printf("[OTA] undef schema\r\n");
        return -1;
    }
    port = purl.port ? purl.port : port;
    memcpy(ota_host, url + purl.field_data[UF_HOST].off, purl.field_data[UF_HOST].len);
    memcpy(ota_resource, url + purl.field_data[UF_PATH].off, purl.field_data[UF_PATH].len);

    printf("[OTA] port:%d host:%s path:%s\r\n", port, ota_host, ota_resource);

    ota_param = ai_ota_parame_init(ota_host, port, ota_resource);
    ota_param.rebooot_cb = app_ota_set_reboot;

    s_ota_task_handle = NULL;
    TaskHandle_t handle = NULL;
    BaseType_t ret = xTaskCreate(_ota_task, "ota", 1024 * 4, NULL, 10, &handle);
    if (ret != pdPASS)
    {
        printf("[OTA] task create fail: %d\r\n", (int)ret);
        s_ota_task_handle = NULL;
        return -1;
    }
    s_ota_task_handle = handle;

    return 0;
}

bool m_ota_is_busy(void)
{
    return is_ota != 0U;
}

int m_ota_start(char *url)
{
    if (url == NULL || url[0] == '\0')
    {
        return -1;
    }

    if (m_ci13162_ota_is_busy() || m_ota_is_busy())
    {
        return -1;
    }

    is_ota = STATE_OTA_PROCESS;
    m_fan_prepare_for_ota();
    flash_set_rs_reason(3);
    // os_trace("Free memory: %d\r\n", xPortGetFreeHeapSize());
    m_mqtt_get_instance()._mqtt_stop();
    vTaskDelay(pdMS_TO_TICKS(2000));
    int ret = axk_hal_user_ota_update(url);
    if (ret != 0)
    {
        os_trace("OTA start failed: %d, rebooting to restore state\r\n", ret);
        app_ota_set_reboot(false);
        return ret;
    }
    return 0;
}
