/**
 * @file aiio_uart0_rx_log.h
 * @brief UART0 receive data logging interface.
 */

#ifndef AIIO_UART0_RX_LOG_H
#define AIIO_UART0_RX_LOG_H

#include <stdint.h>

/** @brief Initialize UART0 RX and its data logging task.
 *
 *  @return      Initialization result.
 *  @retval      0  Initialization succeeded or the module is disabled.
 *  @retval      -1 Initialization failed.
 *  @note        UART0 keeps GPIO16 as TX and uses the configured RX pin at 115200 baud.
 */
int32_t aiio_uart0_rx_log_init(void);

#endif /* AIIO_UART0_RX_LOG_H */
