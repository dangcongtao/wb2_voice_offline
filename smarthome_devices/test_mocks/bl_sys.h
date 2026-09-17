#ifndef _MOCK_BL_SYS_H_
#define _MOCK_BL_SYS_H_

static inline int bl_sys_logall_disable(void) { return 0; }
static inline int bl_sys_logall_enable(void) { return 0; }
void bl_sys_reset_por(void);

#endif
