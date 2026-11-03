#ifndef _MOCK_BL602_UART_H_
#define _MOCK_BL602_UART_H_

#define UART0_ID 0
#define UART_TXRX 1

static inline void UART_Enable(int id, int flag) { (void)id; (void)flag; }
static int g_mock_uart_disable_count;
static inline void UART_Disable(int id, int flag) { (void)id; (void)flag; g_mock_uart_disable_count++; }
static inline void UART_TxFifoClear(int id) { (void)id; }
typedef int UART_ID_Type;
static inline unsigned char UART_GetRxFifoCount(UART_ID_Type id) { (void)id; return 0U; }

#endif
