	.file	"hardware.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.gpio_set_output,"ax",@progbits
	.align	1
	.globl	gpio_set_output
	.type	gpio_set_output, @function
gpio_set_output:
.LFB4:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.c"
	.loc 1 10 1
	.cfi_startproc
.LVL0:
	.loc 1 11 5
	li	a2,0
	li	a1,0
	andi	a0,a0,0xff
.LVL1:
	tail	bl_gpio_enable_output
.LVL2:
	.cfi_endproc
.LFE4:
	.size	gpio_set_output, .-gpio_set_output
	.section	.text.gpio_set_input,"ax",@progbits
	.align	1
	.globl	gpio_set_input
	.type	gpio_set_input, @function
gpio_set_input:
.LFB5:
	.loc 1 15 1
	.cfi_startproc
.LVL3:
	.loc 1 16 5
	li	a2,0
	li	a1,0
	andi	a0,a0,0xff
.LVL4:
	tail	bl_gpio_enable_input
.LVL5:
	.cfi_endproc
.LFE5:
	.size	gpio_set_input, .-gpio_set_input
	.section	.text.hw_pcb_sort_pins,"ax",@progbits
	.align	1
	.globl	hw_pcb_sort_pins
	.type	hw_pcb_sort_pins, @function
hw_pcb_sort_pins:
.LFB6:
	.loc 1 21 1
	.cfi_startproc
	.loc 1 22 5
	.loc 1 21 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	ra,12(sp)
	.cfi_offset 1, -4
	.loc 1 22 5
	call	hw_switch_match_pcb
.LVL6:
	.loc 1 23 5 is_stmt 1
	.loc 1 24 1 is_stmt 0
	lw	ra,12(sp)
	.cfi_restore 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	.loc 1 23 5
	tail	hw_button_match_pcb
.LVL7:
	.cfi_endproc
.LFE6:
	.size	hw_pcb_sort_pins, .-hw_pcb_sort_pins
	.globl	gs_dev_type
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.string	"wswitch2v"
	.align	2
.LC1:
	.string	"lhswitch2v"
	.align	2
.LC2:
	.string	"wsdatic"
	.globl	max_number_channel
	.globl	device_type
	.section	.data.gs_dev_type,"aw"
	.align	2
	.type	gs_dev_type, @object
	.size	gs_dev_type, 12
gs_dev_type:
	.word	.LC0
	.word	.LC1
	.word	.LC2
	.section	.sbss.device_type,"aw",@nobits
	.type	device_type, @object
	.size	device_type, 1
device_type:
	.zero	1
	.section	.sbss.max_number_channel,"aw",@nobits
	.type	max_number_channel, @object
	.size	max_number_channel, 1
max_number_channel:
	.zero	1
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/machine/_default_types.h"
	.file 3 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/sys/_stdint.h"
	.file 4 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.h"
	.file 5 "/home/dungnt98/Ai-Thinker-WB2/components/platform/hosal/bl602_hal/bl_gpio.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x1c5
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.byte	0x1
	.4byte	.LASF22
	.byte	0xc
	.4byte	.LASF23
	.4byte	.LASF24
	.4byte	.Ldebug_ranges0+0
	.4byte	0
	.4byte	.Ldebug_line0
	.byte	0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF0
	.byte	0x3
	.4byte	.LASF10
	.byte	0x2
	.byte	0x2b
	.byte	0x17
	.4byte	0x38
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
	.byte	0x5
	.byte	0x4
	.4byte	0x7d
	.byte	0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF9
	.byte	0x3
	.4byte	.LASF11
	.byte	0x3
	.byte	0x18
	.byte	0x13
	.4byte	0x2c
	.byte	0x6
	.4byte	0x84
	.byte	0x7
	.4byte	.LASF12
	.byte	0x4
	.byte	0x20
	.byte	0x19
	.4byte	0x90
	.byte	0x7
	.4byte	.LASF13
	.byte	0x4
	.byte	0x21
	.byte	0x19
	.4byte	0x90
	.byte	0x8
	.4byte	0xa1
	.byte	0x1
	.byte	0x4
	.byte	0x12
	.byte	0x5
	.byte	0x3
	.4byte	device_type
	.byte	0x8
	.4byte	0x95
	.byte	0x1
	.byte	0x5
	.byte	0x12
	.byte	0x5
	.byte	0x3
	.4byte	max_number_channel
	.byte	0x9
	.4byte	0x77
	.4byte	0xd9
	.byte	0xa
	.4byte	0x70
	.byte	0x2
	.byte	0
	.byte	0xb
	.4byte	.LASF14
	.byte	0x1
	.byte	0x6
	.byte	0x7
	.4byte	0xc9
	.byte	0x5
	.byte	0x3
	.4byte	gs_dev_type
	.byte	0xc
	.4byte	.LASF15
	.byte	0x1
	.byte	0x14
	.byte	0x6
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.byte	0x1
	.byte	0x9c
	.4byte	0x114
	.byte	0xd
	.4byte	.LVL6
	.4byte	0x198
	.byte	0xe
	.4byte	.LVL7
	.4byte	0x1a4
	.byte	0
	.byte	0xc
	.4byte	.LASF16
	.byte	0x1
	.byte	0xe
	.byte	0x6
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.byte	0x1
	.byte	0x9c
	.4byte	0x156
	.byte	0xf
	.string	"pin"
	.byte	0x1
	.byte	0xe
	.byte	0x19
	.4byte	0x69
	.4byte	.LLST1
	.byte	0x10
	.4byte	.LVL5
	.4byte	0x1b0
	.byte	0x11
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x11
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x30
	.byte	0x11
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0
	.byte	0xc
	.4byte	.LASF17
	.byte	0x1
	.byte	0x9
	.byte	0x6
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.byte	0x1
	.byte	0x9c
	.4byte	0x198
	.byte	0xf
	.string	"pin"
	.byte	0x1
	.byte	0x9
	.byte	0x1a
	.4byte	0x69
	.4byte	.LLST0
	.byte	0x10
	.4byte	.LVL2
	.4byte	0x1bc
	.byte	0x11
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x11
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x30
	.byte	0x11
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0
	.byte	0x12
	.4byte	.LASF18
	.4byte	.LASF18
	.byte	0x4
	.byte	0x35
	.byte	0x6
	.byte	0x12
	.4byte	.LASF19
	.4byte	.LASF19
	.byte	0x4
	.byte	0x2a
	.byte	0x6
	.byte	0x12
	.4byte	.LASF20
	.4byte	.LASF20
	.byte	0x5
	.byte	0x10
	.byte	0x5
	.byte	0x12
	.4byte	.LASF21
	.4byte	.LASF21
	.byte	0x5
	.byte	0xf
	.byte	0x5
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
	.byte	0xf
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x6
	.byte	0x35
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x7
	.byte	0x34
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
	.byte	0x3f
	.byte	0x19
	.byte	0x3c
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0x8
	.byte	0x34
	.byte	0
	.byte	0x47
	.byte	0x13
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x9
	.byte	0x1
	.byte	0x1
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xa
	.byte	0x21
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0x2f
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0xb
	.byte	0x34
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
	.byte	0x3f
	.byte	0x19
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0xc
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
	.byte	0xd
	.byte	0x89,0x82,0x1
	.byte	0
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xe
	.byte	0x89,0x82,0x1
	.byte	0
	.byte	0x11
	.byte	0x1
	.byte	0x95,0x42
	.byte	0x19
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xf
	.byte	0x5
	.byte	0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x10
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x95,0x42
	.byte	0x19
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x11
	.byte	0x8a,0x82,0x1
	.byte	0
	.byte	0x2
	.byte	0x18
	.byte	0x91,0x42
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x12
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
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST1:
	.4byte	.LVL3
	.4byte	.LVL4
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL4
	.4byte	.LFE5
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST0:
	.4byte	.LVL0
	.4byte	.LVL1
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL1
	.4byte	.LFE4
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.4byte	.LFB4
	.4byte	.LFE4
	.4byte	.LFB5
	.4byte	.LFE5
	.4byte	.LFB6
	.4byte	.LFE6
	.4byte	0
	.4byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF6:
	.string	"long long int"
.LASF16:
	.string	"gpio_set_input"
.LASF8:
	.string	"unsigned int"
.LASF24:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/hardware"
.LASF13:
	.string	"device_type"
.LASF5:
	.string	"long unsigned int"
.LASF7:
	.string	"long long unsigned int"
.LASF10:
	.string	"__uint8_t"
.LASF14:
	.string	"gs_dev_type"
.LASF17:
	.string	"gpio_set_output"
.LASF11:
	.string	"uint8_t"
.LASF1:
	.string	"unsigned char"
.LASF15:
	.string	"hw_pcb_sort_pins"
.LASF4:
	.string	"long int"
.LASF20:
	.string	"bl_gpio_enable_input"
.LASF23:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.c"
.LASF3:
	.string	"short unsigned int"
.LASF0:
	.string	"signed char"
.LASF12:
	.string	"max_number_channel"
.LASF19:
	.string	"hw_button_match_pcb"
.LASF21:
	.string	"bl_gpio_enable_output"
.LASF2:
	.string	"short int"
.LASF18:
	.string	"hw_switch_match_pcb"
.LASF22:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
.LASF9:
	.string	"char"
	.ident	"GCC: (SiFive GCC-Metal 10.2.0-2020.12.8) 10.2.0"
