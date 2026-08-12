/**
 * @file aiio_ci13162_uart.h
 * @brief CI13162 voice command UART interface.
 */

#ifndef AIIO_CI13162_UART_H
#define AIIO_CI13162_UART_H

#include <stdint.h>

/** @brief Initialize the CI13162 UART receiver and command processing task.
 *
 *  @return      Initialization result.
 *  @retval      0  Initialization succeeded or the module is disabled.
 *  @retval      -1 Initialization failed.
 *  @note        UART0 keeps GPIO16 as TX and uses GPIO7 as RX at 115200 baud.
 *               Initialize this interface before other tasks start logging.
 */
int32_t aiio_ci13162_uart_init(void);

/** @brief Process queued CI13162 commands in application task context.
 *
 *  @return      None.
 *  @note        Call this function periodically after the fan module is initialized.
 */
void aiio_ci13162_uart_process(void);

#endif /* AIIO_CI13162_UART_H */
