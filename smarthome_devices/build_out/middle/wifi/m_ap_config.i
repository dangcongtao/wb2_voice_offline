# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.c"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/middle//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.c"
# 9 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.c"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.h" 1



# 1 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint.h" 1 3 4
# 11 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint.h" 3 4
# 1 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint-gcc.h" 1 3 4
# 34 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint-gcc.h" 3 4

# 34 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint-gcc.h" 3 4
typedef signed char int8_t;


typedef short int int16_t;


typedef long int int32_t;


typedef long long int int64_t;


typedef unsigned char uint8_t;


typedef short unsigned int uint16_t;


typedef long unsigned int uint32_t;


typedef long long unsigned int uint64_t;




typedef signed char int_least8_t;
typedef short int int_least16_t;
typedef long int int_least32_t;
typedef long long int int_least64_t;
typedef unsigned char uint_least8_t;
typedef short unsigned int uint_least16_t;
typedef long unsigned int uint_least32_t;
typedef long long unsigned int uint_least64_t;



typedef int int_fast8_t;
typedef int int_fast16_t;
typedef int int_fast32_t;
typedef long long int int_fast64_t;
typedef unsigned int uint_fast8_t;
typedef unsigned int uint_fast16_t;
typedef unsigned int uint_fast32_t;
typedef long long unsigned int uint_fast64_t;




typedef int intptr_t;


typedef unsigned int uintptr_t;




typedef long long int intmax_t;
typedef long long unsigned int uintmax_t;
# 12 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint.h" 2 3 4
# 5 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.h" 2
# 1 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdbool.h" 1 3 4
# 6 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.h" 2





# 10 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.h"
typedef enum {
    START_APCONFIG = 0,
    SCAN_WIFI_AP,
    WAIT_WIFI_SCAN,
    START_ACCESS_POINT,
    WAIT_USER_CONFIG,
    CONNECT_WIFI,
    AP_CONFIG_DONE
} m_apconfig_step_t;


typedef struct {
    char ssid[20][32];
    int8_t rssi[20];
    uint32_t num_ap_available;
} m_ap_around_t;

void start_webserver(void);

m_apconfig_step_t apconfig_start(void);
void reset_ap_config(void);


# 32 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.h" 3 4
_Bool 
# 32 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.h"
    apconfig_parse_wifi_info(char* json);
# 10 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_ap_config.c" 2
