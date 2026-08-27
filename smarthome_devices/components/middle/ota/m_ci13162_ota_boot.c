#include "m_ci13162_ota_boot.h"

#include <stddef.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <bl_wdt.h>

#include "aiio_ci13162_config.h"
#include "app_config.h"
#include "easyflash_common.h"
#include "flash_api.h"
#include "m_ci13162_ota.h"

#define AI_CI13162_OTA_PENDING_KEY       "ci_ota_pend"
#define AI_CI13162_OTA_ATTEMPT_KEY       "ci_ota_att"
#define AI_CI13162_OTA_PENDING_MAGIC     0x31333143U
#define AI_CI13162_OTA_PENDING_VERSION   2U
#define AI_CI13162_OTA_BOOT_RESET_DELAY_MS 100U

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t attempt;
    uint16_t firmware_url_length;
    uint16_t agent_url_length;
    char firmware_url[AIIO_CI13162_OTA_URL_MAX];
    char agent_url[AIIO_CI13162_OTA_URL_MAX];
    uint8_t erase_nv;
    uint32_t crc32;
} ai_ci13162_ota_pending_t;

static volatile uint8_t s_ai_update_only_active;
static volatile uint8_t s_ai_got_ip;
static volatile uint8_t s_ai_ota_started;
static TickType_t s_ai_boot_start_tick;
static TickType_t s_ai_ota_start_tick;
static ai_ci13162_ota_pending_t s_ai_pending;
static ai_ci13162_ota_pending_t s_ai_pending_verify;
static char s_ai_pending_key[sizeof(AI_CI13162_OTA_PENDING_KEY)];
static char s_ai_attempt_key[sizeof(AI_CI13162_OTA_ATTEMPT_KEY)];

static uint32_t ai_ci13162_ota_pending_crc(const ai_ci13162_ota_pending_t *pending)
{
    if (pending == NULL) {
        return 0U;
    }

    return ef_calc_crc32(0U, pending, offsetof(ai_ci13162_ota_pending_t, crc32));
}

static bool ai_ci13162_ota_pending_validate(const ai_ci13162_ota_pending_t *pending)
{
    if (pending == NULL) {
        return false;
    }
    if ((pending->magic != AI_CI13162_OTA_PENDING_MAGIC) ||
        (pending->version != AI_CI13162_OTA_PENDING_VERSION) ||
        (pending->firmware_url_length == 0U) ||
        (pending->firmware_url_length >= AIIO_CI13162_OTA_URL_MAX) ||
        (pending->agent_url_length == 0U) ||
        (pending->agent_url_length >= AIIO_CI13162_OTA_URL_MAX) ||
        (pending->erase_nv > 1U)) {
        return false;
    }
    if ((pending->firmware_url[pending->firmware_url_length] != '\0') ||
        (pending->agent_url[pending->agent_url_length] != '\0') ||
        (memchr(pending->firmware_url, '\0', pending->firmware_url_length) != NULL) ||
        (memchr(pending->agent_url, '\0', pending->agent_url_length) != NULL)) {
        return false;
    }
    if ((pending->crc32 != ai_ci13162_ota_pending_crc(pending)) ||
        !m_ci13162_ota_url_is_valid(pending->firmware_url) ||
        !m_ci13162_ota_url_is_valid(pending->agent_url)) {
        return false;
    }

    return true;
}

static const char *ai_ci13162_ota_pending_key(void)
{
    memcpy(s_ai_pending_key,
           AI_CI13162_OTA_PENDING_KEY,
           sizeof(s_ai_pending_key));
    return s_ai_pending_key;
}

static const char *ai_ci13162_ota_attempt_key(void)
{
    memcpy(s_ai_attempt_key,
           AI_CI13162_OTA_ATTEMPT_KEY,
           sizeof(s_ai_attempt_key));
    return s_ai_attempt_key;
}

static uint16_t ai_ci13162_ota_attempt_read(void)
{
    uint16_t attempt = 0U;

    if (!ef_get_u16(ai_ci13162_ota_attempt_key(), &attempt)) {
        return 0U;
    }
    return attempt;
}

static bool ai_ci13162_ota_attempt_write(uint16_t attempt)
{
    return ef_set_u16(ai_ci13162_ota_attempt_key(), attempt);
}

static bool ai_ci13162_ota_pending_read(ai_ci13162_ota_pending_t *pending)
{
    int read_length;

    if (pending == NULL) {
        return false;
    }

    memset(pending, 0, sizeof(*pending));
    read_length = ef_get_bytes(ai_ci13162_ota_pending_key(),
                               (uint8_t *)pending,
                               sizeof(*pending));
    return (read_length == (int)sizeof(*pending)) &&
           ai_ci13162_ota_pending_validate(pending);
}

static bool ai_ci13162_ota_pending_write(const ai_ci13162_ota_pending_t *pending)
{
    if (pending == NULL) {
        return false;
    }

    os_trace("[CI13162][BOOT] Saving pending request, size %u\r\n",
             (unsigned int)sizeof(*pending));
    if (!ef_set_bytes(ai_ci13162_ota_pending_key(),
                      (uint8_t *)pending,
                      sizeof(*pending))) {
        os_trace("[CI13162][BOOT] EasyFlash write failed\r\n");
        return false;
    }

    os_trace("[CI13162][BOOT] Verifying pending request\r\n");
    if (!ai_ci13162_ota_pending_read(&s_ai_pending_verify) ||
        (memcmp(pending, &s_ai_pending_verify, sizeof(*pending)) != 0)) {
        os_trace("[CI13162][BOOT] EasyFlash verify failed\r\n");
        return false;
    }

    os_trace("[CI13162][BOOT] Pending request saved\r\n");
    return true;
}

int32_t m_ci13162_ota_boot_save(const char *firmware_url,
                                const char *agent_url,
                                bool erase_nv)
{
#if AIIO_CI13162_OTA_ENABLE == 1
    size_t firmware_url_length;
    size_t agent_url_length;

    if (!m_ci13162_ota_url_is_valid(firmware_url) ||
        !m_ci13162_ota_url_is_valid(agent_url)) {
        return -1;
    }

    firmware_url_length = strlen(firmware_url);
    agent_url_length = strlen(agent_url);
    memset(&s_ai_pending, 0, sizeof(s_ai_pending));
    s_ai_pending.magic = AI_CI13162_OTA_PENDING_MAGIC;
    s_ai_pending.version = AI_CI13162_OTA_PENDING_VERSION;
    s_ai_pending.attempt = 0U;
    s_ai_pending.firmware_url_length = (uint16_t)firmware_url_length;
    s_ai_pending.agent_url_length = (uint16_t)agent_url_length;
    memcpy(s_ai_pending.firmware_url, firmware_url, firmware_url_length + 1U);
    memcpy(s_ai_pending.agent_url, agent_url, agent_url_length + 1U);
    s_ai_pending.erase_nv = erase_nv ? 1U : 0U;
    s_ai_pending.crc32 = ai_ci13162_ota_pending_crc(&s_ai_pending);

    if (!ai_ci13162_ota_pending_write(&s_ai_pending)) {
        return -1;
    }
    if (!ai_ci13162_ota_attempt_write(0U)) {
        os_trace("[CI13162][BOOT] Attempt counter reset failed\r\n");
    }
    return 0;
#else
    (void)firmware_url;
    (void)agent_url;
    (void)erase_nv;
    return -1;
#endif
}

bool m_ci13162_ota_boot_prepare(m_wifi_info_t *wifi_info)
{
#if AIIO_CI13162_OTA_ENABLE == 1
    s_ai_update_only_active = 0U;
    s_ai_got_ip = 0U;
    s_ai_ota_started = 0U;

    if (!ai_ci13162_ota_pending_read(&s_ai_pending)) {
        return false;
    }
    s_ai_pending.attempt = ai_ci13162_ota_attempt_read();
    if ((wifi_info == NULL) ||
        (s_ai_pending.attempt >= AIIO_CI13162_OTA_BOOT_MAX_ATTEMPTS) ||
        (flash_read_wifi_info(wifi_info) != 0)) {
        if (!m_ci13162_ota_boot_clear()) {
            os_trace("[CI13162][BOOT] Pending request clear failed\r\n");
        }
        return false;
    }

    s_ai_pending.attempt++;
    if (!ai_ci13162_ota_attempt_write(s_ai_pending.attempt)) {
        os_trace("[CI13162][BOOT] Attempt counter write failed\r\n");
    }

    s_ai_boot_start_tick = xTaskGetTickCount();
    s_ai_ota_start_tick = 0U;
    s_ai_update_only_active = 1U;
    os_trace("[CI13162][BOOT] OTA-only attempt %u/%u\r\n",
             (unsigned int)s_ai_pending.attempt,
             (unsigned int)AIIO_CI13162_OTA_BOOT_MAX_ATTEMPTS);
    return true;
#else
    (void)wifi_info;
    return false;
#endif
}

bool m_ci13162_ota_boot_is_active(void)
{
    return s_ai_update_only_active != 0U;
}

void m_ci13162_ota_boot_note_got_ip(void)
{
    if (s_ai_update_only_active != 0U) {
        s_ai_got_ip = 1U;
    }
}

void m_ci13162_ota_boot_note_disconnect(void)
{
    if ((s_ai_update_only_active != 0U) &&
        (s_ai_ota_started == 0U)) {
        s_ai_got_ip = 0U;
    }
}

bool m_ci13162_ota_boot_poll(void)
{
    TickType_t now;

    if (s_ai_update_only_active == 0U) {
        return false;
    }

    now = xTaskGetTickCount();
    if ((s_ai_got_ip != 0U) && (s_ai_ota_started == 0U)) {
        s_ai_ota_started = 1U;
        if (!ai_ci13162_ota_pending_read(&s_ai_pending)) {
            os_trace("[CI13162][BOOT] Pending request read failed\r\n");
            vTaskDelay(pdMS_TO_TICKS(AI_CI13162_OTA_BOOT_RESET_DELAY_MS));
            return true;
        }
        if (m_ci13162_ota_start(s_ai_pending.firmware_url,
                                s_ai_pending.agent_url,
                                (s_ai_pending.erase_nv != 0U)) != 0) {
            os_trace("[CI13162][BOOT] OTA start failed\r\n");
            vTaskDelay(pdMS_TO_TICKS(AI_CI13162_OTA_BOOT_RESET_DELAY_MS));
            return true;
        }
        s_ai_ota_start_tick = now;
        os_trace("[CI13162][BOOT] OTA started, free heap: %u bytes\r\n",
                 (unsigned int)xPortGetFreeHeapSize());
    }

    if (s_ai_ota_started == 0U) {
        return (now - s_ai_boot_start_tick) >=
               pdMS_TO_TICKS(AIIO_CI13162_OTA_WIFI_TIMEOUT_MS);
    }

    return (now - s_ai_ota_start_tick) >=
           pdMS_TO_TICKS(AIIO_CI13162_OTA_TOTAL_TIMEOUT_MS);
}

bool m_ci13162_ota_boot_clear(void)
{
    uint32_t attempt;

    for (attempt = 0U;
         attempt < AIIO_CI13162_OTA_CLEAR_RETRY_COUNT;
         attempt++) {
        (void)ef_del_key(ai_ci13162_ota_pending_key());
        (void)ef_del_key(ai_ci13162_ota_attempt_key());
        if (!ai_ci13162_ota_pending_read(&s_ai_pending_verify)) {
            s_ai_update_only_active = 0U;
            s_ai_got_ip = 0U;
            s_ai_ota_started = 0U;
            return true;
        }
        bl_wdt_feed();
        vTaskDelay(pdMS_TO_TICKS(
            AIIO_CI13162_OTA_CLEAR_RETRY_DELAY_MS));
    }

    return false;
}
