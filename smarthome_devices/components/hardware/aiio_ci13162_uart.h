/**
 * @file aiio_ci13162_uart.h
 * @brief CI13162 voice command UART interface.
 */

#ifndef AIIO_CI13162_UART_H
#define AIIO_CI13162_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Handle one final CWSL status received from the CI13162.
 *
 *  @param[in]  code    CWSL status code.
 *  @param[in]  cmd_id  ASR command identifier.
 *  @param[in]  slot    CWSL slot identifier.
 *  @return     None.
 */
typedef void (*aiio_ci13162_cwsl_status_cb_t)(uint8_t code,
                                              uint8_t cmd_id,
                                              uint8_t slot);

/** @brief Handle one CWSL query response received from the CI13162.
 *
 *  @param[in]  cmd_id      Queried ASR command identifier.
 *  @param[in]  data        Query result bytes after cmd_id.
 *  @param[in]  data_count  Number of bytes available in data.
 *  @return     None.
 *  @note       data_count is two for a bitmap and three for capacity data.
 */
typedef void (*aiio_ci13162_cwsl_info_cb_t)(uint8_t cmd_id,
                                            const uint8_t *data,
                                            uint8_t data_count);

/** @brief Handle one speaker volume response received from the CI13162.
 *
 *  @param[in]  level   Current speaker volume level from 0 to 7, where 0 is mute.
 *  @param[in]  result  Request result: 1 when accepted, otherwise 0.
 *  @return     None.
 */
typedef void (*aiio_ci13162_volume_info_cb_t)(uint8_t level,
                                              uint8_t result);

/** @brief Drive UART0 TX idle-high before the port is remuxed.
 *
 *  @return      None.
 *  @note        GPIO16 stays high so leftover Boot2 UART0 bytes do not reach
 *               the CI13162 RX pin. Protocol bytes can still be sent later.
 */
void aiio_ci13162_uart_hold_tx_idle(void);

/** @brief Initialize power control pin (GPIO 17) for CI13162 voice IC.
 *
 *  @return      Initialization result.
 *  @retval      0  Power control pin initialized and CI13162 powered ON.
 *  @retval      -1 Device does not support voice or module disabled.
 */
int32_t aiio_ci13162_power_init(void);

/** @brief Perform a power cycle reset on the CI13162 voice IC via GPIO 17.
 *
 *  @param[in]   off_duration_ms     Duration in milliseconds to hold power OFF (LOW).
 *  @param[in]   stabilize_delay_ms  Duration in milliseconds to wait after powering ON (HIGH).
 *  @return      Power cycle result.
 *  @retval      0  Power cycle completed successfully.
 *  @retval      -1 Device does not support voice or module disabled.
 */
int32_t aiio_ci13162_power_cycle(uint32_t off_duration_ms, uint32_t stabilize_delay_ms);

/** @brief Initialize the CI13162 UART receiver and command processing task.
 *
 *  @return      Initialization result.
 *  @retval      0  Initialization succeeded or the module is disabled.
 *  @retval      -1 Initialization failed.
 *  @note        UART0 keeps GPIO16 as TX and uses GPIO7 as RX at 115200 baud.
 *               TX stays idle-high until a protocol write. Debug text uses
 *               the GPIO14 log path, not UART0.
 */
int32_t aiio_ci13162_uart_init(void);

/** @brief Process queued CI13162 commands in application task context.
 *
 *  @return      None.
 *  @note        Call this function periodically after the fan module is initialized.
 */
void aiio_ci13162_uart_process(void);

/** @brief Process deferred CI13162 maintenance requests.
 *
 *  @return     None.
 *  @note       Call after application commands so CWSL can pause version TX first.
 */
void aiio_ci13162_uart_poll(void);

/** @brief Send one CWSL request to the CI13162.
 *
 *  @param[in]  action        CWSL action: learn, delete, stop, or query.
 *  @param[in]  data          Action arguments, or NULL for stop.
 *  @param[in]  data_count    Number of action arguments.
 *  @return     Send status.
 *  @retval     0             Request sent successfully.
 *  @retval     -1            Invalid arguments, UART unavailable, or send failure.
 */
int32_t aiio_ci13162_uart_send_cwsl(uint8_t action,
                                    const uint8_t *data,
                                    uint8_t data_count);

/** @brief Register application callbacks for CWSL responses.
 *
 *  @param[in]  status_cb  Status callback, or NULL to disable status delivery.
 *  @param[in]  info_cb    Query callback, or NULL to disable query delivery.
 *  @return     None.
 */
void aiio_ci13162_uart_set_cwsl_callbacks(
    aiio_ci13162_cwsl_status_cb_t status_cb,
    aiio_ci13162_cwsl_info_cb_t info_cb);

/** @brief Send one buzzer beep request to the CI13162.
 *
 *  @return     Send status.
 *  @retval     0  Request `{"cmd":200,"data":[]}` was sent.
 *  @retval     -1 UART unavailable, OTA active, or send failure.
 */
int32_t aiio_ci13162_uart_send_buzz(void);

/** @brief Send one speaker volume set request to the CI13162.
 *
 *  @param[in]  value  Absolute speaker volume level from 1 to 7.
 *  @return     Send status.
 *  @retval     0  Request `{"cmd":8,"data":[value]}` was sent.
 *  @retval     -1 Invalid value, UART unavailable, OTA active, or send failure.
 */
int32_t aiio_ci13162_uart_send_volume(uint8_t value);

/** @brief Query the current speaker volume from the CI13162.
 *
 *  @return     Send status.
 *  @retval     0  Query `{"cmd":11,"data":[]}` was sent.
 *  @retval     -1 UART unavailable, OTA active, or send failure.
 */
int32_t aiio_ci13162_uart_query_volume(void);

/** @brief Register the application callback for speaker volume responses.
 *
 *  @param[in]  callback  Response callback, or NULL to disable delivery.
 *  @return     None.
 */
void aiio_ci13162_uart_set_volume_callback(
    aiio_ci13162_volume_info_cb_t callback);

/** @brief Read the cached speaker volume level.
 *
 *  @param[out]  level  Destination for the cached level from 0 to 7, where 0 is mute.
 *  @return      Cache status.
 *  @retval      0  A valid cached level was copied.
 *  @retval      -1 The pointer is NULL or no valid response has been received.
 *  @note        This function never sends a UART request. Boot starts with
 *               no cache (valid=0), which is not mute.
 */
int32_t aiio_ci13162_uart_get_volume(uint8_t *level);

/** @brief Pause or resume automatic CI13162 version requests.
 *
 *  @param[in]  paused  Non-zero to pause requests, zero to resume them.
 *  @return     None.
 *  @note       Incoming version responses are still cached while paused.
 */
void aiio_ci13162_uart_set_version_query_paused(uint8_t paused);

/** @brief Claim UART0 for the CI13162 binary OTA protocol. */
int32_t aiio_ci13162_uart_ota_acquire(void);

/** @brief Return UART0 to the CI13162 application command protocol. */
void aiio_ci13162_uart_ota_release(void);

/** @brief Return true while the OTA protocol owns UART0. */
bool aiio_ci13162_uart_ota_held(void);

/** @brief Change UART0 baud while OTA owns the port. */
int32_t aiio_ci13162_uart_ota_set_baud(uint32_t baud);

/** @brief Send one raw OTA byte sequence. */
int32_t aiio_ci13162_uart_ota_write(const void *data, size_t length);

/** @brief Read up to length raw OTA bytes, waiting at most timeout_ms. */
int32_t aiio_ci13162_uart_ota_read(void *data, size_t length, uint32_t timeout_ms);

/** @brief Clear pending raw OTA bytes and overflow state. */
void aiio_ci13162_uart_ota_flush_rx(void);

/** @brief Return true and clear the OTA RX overflow latch. */
bool aiio_ci13162_uart_ota_take_overflow(void);

/** @brief Send the application-defined reset command to the CI13162. */
int32_t aiio_ci13162_uart_ota_send_app_reset(void);

/** @brief Ask the CI13162 for its firmware version.
 *
 *  @return      Send result.
 *  @retval      0  Query `{"cmd":3,"data":[]}` was sent.
 *  @retval      -1 UART is not ready or OTA owns the port.
 *  @note        The chip reply `{"cmd":3,"data":[<version>]}` is cached when received.
 */
int32_t aiio_ci13162_uart_request_version(void);

/** @brief Get the cached CI13162 firmware version.
 *
 *  @return      Cached firmware version, or 0 if no valid reply has been received.
 *  @note        This function only reads the cache and never sends a UART query.
 */
uint32_t aiio_ci13162_uart_get_voice_version(void);

#endif /* AIIO_CI13162_UART_H */
