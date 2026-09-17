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
static inline void GLB_UART_Fun_Sel(int sig, int fun) { (void)sig; (void)fun; }

#endif
