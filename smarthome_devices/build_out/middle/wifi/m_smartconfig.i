# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_smartconfig.c"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/middle//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_smartconfig.c"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_smartconfig.h" 1



# 1 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdbool.h" 1 3 4
# 5 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_smartconfig.h" 2

typedef enum {
    SC_START = 0,
    SC_WAIT_CONFIG,
 SC_WAIT_INFO,
    SMARTCONFIG_DONE,
} smartconfig_state_t;


# 13 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_smartconfig.h" 3 4
_Bool 
# 13 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_smartconfig.h"
    m_smartconfig_parse_data(char* json);
void reset_smartconfig(void);
smartconfig_state_t start_smartconfig(void);
# 2 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_smartconfig.c" 2
