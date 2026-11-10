#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <blog.h>

/* VOICEBOX hardware revision */
#define VOICEBOX_HW_V1 1 /* old: chip on UART0 TX16/RX7, 1 button GPIO1 */
#define VOICEBOX_HW_V2 2 /* new: chip on UART1 TX4/RX3, 3 buttons 21/20/0 */
#ifndef VOICEBOX_HW
#define VOICEBOX_HW VOICEBOX_HW_V2
#endif
#if (VOICEBOX_HW != VOICEBOX_HW_V1) && (VOICEBOX_HW != VOICEBOX_HW_V2)
#error "VOICEBOX_HW must be VOICEBOX_HW_V1 or VOICEBOX_HW_V2"
#endif

#if VOICEBOX_HW == VOICEBOX_HW_V1
/* V1 shares UART0 between log and the voice chip: log always off */
#define DEBUG_ENABLE 0
#else
#define DEBUG_ENABLE 1
#endif
#define AIIO_WDT_TIMEOUT_MS 60000U

void m_log(const char *fmt, ...);
#define os_trace           \
    if (DEBUG_ENABLE == 1) \
    printf

/* config for type of device, that will be used to parse json message */
#define EXTRAS 1514
#define FIRMWARE_VERSION "261103"
#define FIRMWARE_VERSION_NUMBER 240702

#ifndef BUILD_TEST
#define BUILD_TEST 0 // 1 dev, 0 release
#endif

#define HARDWARE_VERSION "230303"

#endif /*_USER_CONFIG_H_*/