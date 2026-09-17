#ifndef _MOCK_BL602_UART_H_
#define _MOCK_BL602_UART_H_

#define UART0_ID 0
#define UART_TXRX 1

static inline void UART_Enable(int id, int flag) { (void)id; (void)flag; }
static inline void UART_Disable(int id, int flag) { (void)id; (void)flag; }
static inline void UART_TxFifoClear(int id) { (void)id; }

#endif
