#ifndef _MOCK_BL602_HBN_H_
#define _MOCK_BL602_HBN_H_

#define HBN_BASE 0
#define HBN_IRQ_MODE_OFFSET 0
#define ENABLE 1
#define DISABLE 0

static inline void HBN_Aon_Pad_IeSmt_Cfg(int cfg) { (void)cfg; }
static inline void HBN_Hw_Pu_Pd_Cfg(int cfg) { (void)cfg; }

#endif
