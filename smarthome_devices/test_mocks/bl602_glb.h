#ifndef _MOCK_BL602_GLB_H_
#define _MOCK_BL602_GLB_H_

#include <stdint.h>

#define GLB_GPIO_PIN_16 16
#define GLB_GPIO_PIN_7 7
#define GLB_UART_SIG_SWAP_NONE 0
#define UART_SIG_SWAP_NONE 0
#define GLB_UART_SIG_7 7
#define GLB_UART_SIG_FUN_UART0_RXD 7

static inline void GLB_GPIO_INPUT_Enable(int pin) { (void)pin; }
static inline void GLB_UART_Sig_Swap_Set(int swap) { (void)swap; }
static int g_mock_uart_fun_sel_count;
static int g_mock_uart_fun_sel_last_sig = -1;
static int g_mock_uart_fun_sel_last_fun = -1;
static inline void GLB_UART_Fun_Sel(int sig, int fun) { g_mock_uart_fun_sel_count++; g_mock_uart_fun_sel_last_sig = sig; g_mock_uart_fun_sel_last_fun = fun; }

/* Register/diagnostic helpers used only by the CI13162 UART trace. */
typedef int GLB_GPIO_Type;
#define GLB_BASE 0
#define GLB_UART_SIG_SEL_0 0
#define BL_RD_REG(base, reg) (0UL)
static inline uint8_t GLB_GPIO_Get_Fun(GLB_GPIO_Type pin) { (void)pin; return 7U; }
static inline uint32_t GLB_GPIO_Read(GLB_GPIO_Type pin) { (void)pin; return 1U; }

#endif
