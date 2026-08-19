/**
 * @file aiio_ci13162_uart.h
 * @brief CI13162 voice command UART interface.
 */

#ifndef AIIO_CI13162_UART_H
#define AIIO_CI13162_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Drive UART0 TX idle-high before the port is remuxed.
 *
 *  @return      None.
 *  @note        GPIO16 stays high so leftover Boot2 UART0 bytes do not reach
 *               the CI13162 RX pin. Protocol bytes can still be sent later.
 */
void aiio_ci13162_uart_hold_tx_idle(void);

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
