	.file	"main.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.startup.main,"ax",@progbits
	.align	1
	.globl	main
	.type	main, @function
main:
.LFB4:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/switchv3/main.c"
	.loc 1 11 1
	.cfi_startproc
	.loc 1 12 5
	.loc 1 11 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	.loc 1 12 5
	li	a2,0
	li	a1,0
	li	a0,8
	.loc 1 11 1
	sw	ra,12(sp)
	.cfi_offset 1, -4
	.loc 1 12 5
	call	bl_gpio_enable_input
.LVL0:
	.loc 1 13 5 is_stmt 1
	li	a1,0
	li	a2,0
	li	a0,14
	call	bl_gpio_enable_output
.LVL1:
	.loc 1 14 5
	li	a1,0
	li	a0,14
	call	bl_gpio_output_set
.LVL2:
.L4:
	.loc 1 16 5
	.loc 1 17 9
	.loc 1 17 13 is_stmt 0
	li	a0,8
	call	bl_gpio_input_get_value
.LVL3:
	.loc 1 18 13
	li	a1,1
	.loc 1 17 12
	bne	a0,zero,.L6
	.loc 1 21 13 is_stmt 1
	li	a1,0
.L6:
	li	a0,14
	call	bl_gpio_output_set
.LVL4:
	.loc 1 24 9
	li	a0,5
	call	vTaskDelay
.LVL5:
	.loc 1 16 11
	.loc 1 17 12 is_stmt 0
	j	.L4
	.cfi_endproc
.LFE4:
	.size	main, .-main
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/machine/_default_types.h"
	.file 3 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/sys/_stdint.h"
	.file 4 "/home/dungnt98/Ai-Thinker-WB2/components/platform/soc/bl602/freertos_riscv_ram/portable/GCC/RISC-V/portmacro.h"
	.file 5 "/home/dungnt98/Ai-Thinker-WB2/components/platform/hosal/bl602_hal/bl_gpio.h"
	.file 6 "/home/dungnt98/Ai-Thinker-WB2/components/platform/soc/bl602/freertos_riscv_ram/config/task.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x16e
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.byte	0x1
	.4byte	.LASF18
	.byte	0xc
	.4byte	.LASF19
	.4byte	.LASF20
	.4byte	.Ldebug_ranges0+0
	.4byte	0
	.4byte	.Ldebug_line0
	.byte	0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF0
	.byte	0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF1
	.byte	0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF2
	.byte	0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF3
	.byte	0x2
	.byte	0x4
	.byte	0x5
	.4byte	.LASF4
	.byte	0x3
	.4byte	.LASF10
	.byte	0x2
	.byte	0x4f
	.byte	0x1b
	.4byte	0x54
	.byte	0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF5
	.byte	0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF6
	.byte	0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF7
	.byte	0x4
	.byte	0x4
	.byte	0x5
	.string	"int"
	.byte	0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF8
	.byte	0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF9
	.byte	0x3
	.4byte	.LASF11
	.byte	0x3
	.byte	0x30
	.byte	0x14
	.4byte	0x48
	.byte	0x3
	.4byte	.LASF12
	.byte	0x4
	.byte	0x41
	.byte	0x12
	.4byte	0x7e
	.byte	0x5
	.4byte	.LASF21
	.byte	0x1
	.byte	0xa
	.byte	0x6
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.byte	0x1
	.byte	0x9c
	.4byte	0x134
	.byte	0x6
	.4byte	.LVL0
	.4byte	0x134
	.4byte	0xc9
	.byte	0x7
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x38
	.byte	0x7
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x30
	.byte	0x7
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0x6
	.4byte	.LVL1
	.4byte	0x140
	.4byte	0xe6
	.byte	0x7
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x3e
	.byte	0x7
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x30
	.byte	0x7
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0x6
	.4byte	.LVL2
	.4byte	0x14c
	.4byte	0xfe
	.byte	0x7
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x3e
	.byte	0x7
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0x6
	.4byte	.LVL3
	.4byte	0x158
	.4byte	0x111
	.byte	0x7
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x38
	.byte	0
	.byte	0x6
	.4byte	.LVL4
	.4byte	0x14c
	.4byte	0x124
	.byte	0x7
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x3e
	.byte	0
	.byte	0x8
	.4byte	.LVL5
	.4byte	0x164
	.byte	0x7
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x35
	.byte	0
	.byte	0
	.byte	0x9
	.4byte	.LASF13
	.4byte	.LASF13
	.byte	0x5
	.byte	0x10
	.byte	0x5
	.byte	0x9
	.4byte	.LASF14
	.4byte	.LASF14
	.byte	0x5
	.byte	0xf
	.byte	0x5
	.byte	0x9
	.4byte	.LASF15
	.4byte	.LASF15
	.byte	0x5
	.byte	0x11
	.byte	0x5
	.byte	0x9
	.4byte	.LASF16
	.4byte	.LASF16
	.byte	0x5
	.byte	0x13
	.byte	0x5
	.byte	0xa
	.4byte	.LASF17
	.4byte	.LASF17
	.byte	0x6
	.2byte	0x2f6
	.byte	0x6
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x25
	.byte	0xe
	.byte	0x13
	.byte	0xb
	.byte	0x3
	.byte	0xe
	.byte	0x1b
	.byte	0xe
	.byte	0x55
	.byte	0x17
	.byte	0x11
	.byte	0x1
	.byte	0x10
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x2
	.byte	0x24
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0x3e
	.byte	0xb
	.byte	0x3
	.byte	0xe
	.byte	0
	.byte	0
	.byte	0x3
	.byte	0x16
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x4
	.byte	0x24
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0x3e
	.byte	0xb
	.byte	0x3
	.byte	0x8
	.byte	0
	.byte	0
	.byte	0x5
	.byte	0x2e
	.byte	0x1
	.byte	0x3f
	.byte	0x19
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x27
	.byte	0x19
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x6
	.byte	0x40
	.byte	0x18
	.byte	0x97,0x42
	.byte	0x19
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x6
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x7
	.byte	0x8a,0x82,0x1
	.byte	0
	.byte	0x2
	.byte	0x18
	.byte	0x91,0x42
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x8
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x9
	.byte	0x2e
	.byte	0
	.byte	0x3f
	.byte	0x19
	.byte	0x3c
	.byte	0x19
	.byte	0x6e
	.byte	0xe
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0xa
	.byte	0x2e
	.byte	0
	.byte	0x3f
	.byte	0x19
	.byte	0x3c
	.byte	0x19
	.byte	0x6e
	.byte	0xe
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0x5
	.byte	0x39
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.4byte	.LFB4
	.4byte	.LFE4
	.4byte	0
	.4byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF6:
	.string	"long long int"
.LASF8:
	.string	"unsigned int"
.LASF1:
	.string	"unsigned char"
.LASF10:
	.string	"__uint32_t"
.LASF15:
	.string	"bl_gpio_output_set"
.LASF5:
	.string	"long unsigned int"
.LASF12:
	.string	"TickType_t"
.LASF19:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/switchv3/main.c"
.LASF7:
	.string	"long long unsigned int"
.LASF17:
	.string	"vTaskDelay"
.LASF20:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/switchv3"
.LASF9:
	.string	"char"
.LASF11:
	.string	"uint32_t"
.LASF4:
	.string	"long int"
.LASF13:
	.string	"bl_gpio_enable_input"
.LASF3:
	.string	"short unsigned int"
.LASF0:
	.string	"signed char"
.LASF21:
	.string	"main"
.LASF16:
	.string	"bl_gpio_input_get_value"
.LASF14:
	.string	"bl_gpio_enable_output"
.LASF2:
	.string	"short int"
.LASF18:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
	.ident	"GCC: (SiFive GCC-Metal 10.2.0-2020.12.8) 10.2.0"
