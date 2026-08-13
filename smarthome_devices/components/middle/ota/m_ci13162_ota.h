#ifndef M_CI13162_OTA_H
#define M_CI13162_OTA_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Start the CI13162 UART OTA task.
 *
 *  @param[in]  firmware_url  Firmware image HTTP URL.
 *  @param[in]  agent_url     Updater agent HTTP URL.
 *  @return     Operation status.
 *  @retval     0             OTA task started successfully.
 *  @retval     -1            Invalid input, busy state, or task creation failure.
 */
int32_t m_ci13162_ota_start(const char *firmware_url, const char *agent_url);

/** @brief Validate a CI13162 OTA HTTP URL.
 *
 *  @param[in]  url           URL to validate.
 *  @return     true when the URL is supported and within the configured limit, otherwise false.
 */
bool m_ci13162_ota_url_is_valid(const char *url);

/** @brief Check whether CI13162 OTA is running.
 *
 *  @return     true when CI13162 OTA is busy, otherwise false.
 */
bool m_ci13162_ota_is_busy(void);

/** @brief Get the maximum CI13162 OTA execution time.
 *
 *  @return     Timeout in milliseconds.
 */
uint32_t m_ci13162_ota_timeout_ms(void);

#endif /* M_CI13162_OTA_H */
