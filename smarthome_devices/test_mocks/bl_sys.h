#ifndef _MOCK_BL_SYS_H_
#define _MOCK_BL_SYS_H_

static int g_mock_logall_disable_count;
static int g_mock_logall_enable_count;
static inline int bl_sys_logall_disable(void) { g_mock_logall_disable_count++; return 0; }
static inline int bl_sys_logall_enable(void) { g_mock_logall_enable_count++; return 0; }
void bl_sys_reset_por(void);

#endif
