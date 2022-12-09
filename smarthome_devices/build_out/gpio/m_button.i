# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.c"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/gpio//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.c"
# 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.h" 1


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
# 4 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.h" 2


# 5 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.h"
typedef void (*p_btn_config_t)(uint32_t index);
typedef uint32_t (*p_btn_read_t)(uint32_t index);
typedef uint32_t (*p_get_tick_ms_t)(void);
typedef void (*p_btn_timer_init_t)(void);

typedef struct {

    uint32_t _btn_max_number_channel;
    uint32_t _btn_idle_level;


    p_btn_config_t _button_conf_cb;
    p_btn_read_t _button_read_cb;
    p_btn_timer_init_t _button_timer_init_cb;
    p_get_tick_ms_t _button_get_tick_ms_cb;
} m_button_t;


void m_button_config(m_button_t m_button_obj);
m_button_t m_button_get_instace(void);
# 2 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.c" 2
