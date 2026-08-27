/**
 * @file m_ci13162_ota_boot.h
 * @brief CI13162 OTA-only boot coordinator.
 */

#ifndef M_CI13162_OTA_BOOT_H
#define M_CI13162_OTA_BOOT_H

#include <stdbool.h>
#include <stdint.h>

#include "m_wifi.h"

/** @brief Save a pending CI13162 OTA request to persistent storage.
 *
 *  @param[in]  firmware_url  Firmware image HTTP URL.
 *  @param[in]  agent_url     Updater agent HTTP URL.
 *  @param[in]  erase_nv      true to erase CI13162 NV during OTA, false to keep NV.
 *  @return     Operation status.
 *  @retval     0             Request saved and verified successfully.
 *  @retval     -1            Invalid URL or persistent storage failure.
 */
int32_t m_ci13162_ota_boot_save(const char *firmware_url,
                                const char *agent_url,
                                bool erase_nv);

/** @brief Prepare the CI13162 OTA-only mode during early boot.
 *
 *  @param[out]  wifi_info  Destination for saved Wi-Fi station credentials.
 *  @return      Whether the OTA-only boot path must run.
 *  @retval      true       A valid pending request was consumed and credentials loaded.
 *  @retval      false      Continue with the normal product boot path.
 *  @note        Call after EasyFlash initialization and before product feature initialization.
 */
bool m_ci13162_ota_boot_prepare(m_wifi_info_t *wifi_info);

/** @brief Check whether the current boot is dedicated to CI13162 OTA.
 *
 *  @return     true when OTA-only boot mode is active, otherwise false.
 */
bool m_ci13162_ota_boot_is_active(void);

/** @brief Notify the OTA-only coordinator that Wi-Fi obtained an IP address.
 *
 *  @note       This function is safe to call from the Wi-Fi event task and does not start OTA directly.
 */
void m_ci13162_ota_boot_note_got_ip(void);

/** @brief Notify the OTA-only coordinator that Wi-Fi disconnected.
 *
 *  @note       A disconnect before OTA starts requires a fresh IP event before starting the update.
 */
void m_ci13162_ota_boot_note_disconnect(void);

/** @brief Process the OTA-only boot state from the boot task.
 *
 *  @return     Whether the caller must reset the device.
 *  @retval     true          Wi-Fi, OTA start, or total OTA timeout requires a retry reboot.
 *  @retval     false         Continue polling.
 */
bool m_ci13162_ota_boot_poll(void);

/** @brief Clear and verify removal of the pending CI13162 OTA request.
 *
 *  @return     Whether no valid persistent request remains.
 *  @retval     true          Request is absent after the clear operation.
 *  @retval     false         A valid request remains after all delete attempts.
 */
bool m_ci13162_ota_boot_clear(void);

#endif /* M_CI13162_OTA_BOOT_H */
