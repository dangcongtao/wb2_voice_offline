/**
 * @file ci13162_ota_manifest.h
 * @brief CI13162 OTA manifest downloader and URL resolver.
 */
#ifndef CI13162_OTA_MANIFEST_H
#define CI13162_OTA_MANIFEST_H

#include <stddef.h>
#include <stdint.h>

#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Resolved URLs consumed by the existing CI13162 OTA boot flow. */
typedef struct {
    char firmware_url[AIIO_CI13162_OTA_URL_MAX];
    char agent_url[AIIO_CI13162_OTA_URL_MAX];
} ci13162_ota_manifest_urls_t;

/** @brief Download and resolve a CI13162 OTA manifest.
 *
 *  @param[in]  manifest_url  Absolute manifest HTTP URL.
 *  @param[out] out           Resolved application and agent URLs.
 *  @return     0 on success, -1 on allocation, HTTP, JSON, or URL error.
 */
int32_t ci13162_ota_manifest_fetch(const char *manifest_url,
                                   ci13162_ota_manifest_urls_t *out);

#if defined(BUILD_TEST) && (BUILD_TEST == 1)
/** @brief Run offline tests for manifest parsing and URL construction. */
int ci13162_ota_manifest_self_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CI13162_OTA_MANIFEST_H */
