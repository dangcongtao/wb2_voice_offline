	.file	"led.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.hw_led_stt_cfg,"ax",@progbits
	.align	1
	.globl	hw_led_stt_cfg
	.type	hw_led_stt_cfg, @function
hw_led_stt_cfg:
.LFB4:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/led.c"
	.loc 1 6 1
	.cfi_startproc
	.loc 1 7 5
	li	a0,12
	tail	gpio_set_output
.LVL0:
	.cfi_endproc
.LFE4:
	.size	hw_led_stt_cfg, .-hw_led_stt_cfg
	.section	.text.hw_led_stt_on,"ax",@progbits
	.align	1
	.globl	hw_led_stt_on
	.type	hw_led_stt_on, @function
hw_led_stt_on:
.LFB5:
	.loc 1 11 1
	.cfi_startproc
	.loc 1 12 5
	.loc 1 12 17 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	.loc 1 13 5
	li	a1,0
	li	a0,12
	.loc 1 12 17
	sb	zero,%lo(.LANCHOR0)(a5)
	.loc 1 13 5 is_stmt 1
	tail	bl_gpio_output_set
.LVL1:
	.cfi_endproc
.LFE5:
	.size	hw_led_stt_on, .-hw_led_stt_on
	.section	.text.hw_led_stt_off,"ax",@progbits
	.align	1
	.globl	hw_led_stt_off
	.type	hw_led_stt_off, @function
hw_led_stt_off:
.LFB6:
	.loc 1 17 1
	.cfi_startproc
	.loc 1 18 5
	.loc 1 18 17 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	li	a4,1
	.loc 1 19 5
	li	a1,1
	li	a0,12
	.loc 1 18 17
	sb	a4,%lo(.LANCHOR0)(a5)
	.loc 1 19 5 is_stmt 1
	tail	bl_gpio_output_set
.LVL2:
	.cfi_endproc
.LFE6:
	.size	hw_led_stt_off, .-hw_led_stt_off
	.section	.text.hw_toggle_led_stt,"ax",@progbits
	.align	1
	.globl	hw_toggle_led_stt
	.type	hw_toggle_led_stt, @function
hw_toggle_led_stt:
.LFB7:
	.loc 1 23 1
	.cfi_startproc
	.loc 1 24 5
	.loc 1 24 19 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	addi	a5,a5,%lo(.LANCHOR0)
	lbu	a1,0(a5)
	.loc 1 25 5
	li	a0,12
	.loc 1 24 19
	seqz	a1,a1
	.loc 1 24 17
	sb	a1,0(a5)
	.loc 1 25 5 is_stmt 1
	tail	bl_gpio_output_set
.LVL3:
	.cfi_endproc
.LFE7:
	.size	hw_toggle_led_stt, .-hw_toggle_led_stt
	.section	.sbss.led_stt_val,"aw",@nobits
	.set	.LANCHOR0,. + 0
	.type	led_stt_val, @object
	.size	led_stt_val, 1
led_stt_val:
	.zero	1
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/machine/_default_types.h"
	.file 3 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/sys/_stdint.h"
	.file 4 "/home/dungnt98/Ai-Thinker-WB2/components/platform/hosal/bl602_hal/bl_gpio.h"
	.file 5 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x153
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
	.byte	0x5
	.4byte	.LASF21
	.byte	0x1
	.byte	0x3
	.byte	0x10
	.4byte	0x7e
	.byte	0x5
	.byte	0x3
	.4byte	led_stt_val
	.byte	0x6
	.4byte	.LASF12
	.byte	0x1
	.byte	0x16
	.byte	0x6
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.byte	0x1
	.byte	0x9c
	.4byte	0xc2
	.byte	0x7
	.4byte	.LVL3
	.4byte	0x13e
	.byte	0x8
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x3c
	.byte	0
	.byte	0
	.byte	0x6
	.4byte	.LASF13
	.byte	0x1
	.byte	0x10
	.byte	0x6
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.byte	0x1
	.byte	0x9c
	.4byte	0xed
	.byte	0x7
	.4byte	.LVL2
	.4byte	0x13e
	.byte	0x8
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x3c
	.byte	0x8
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x31
	.byte	0
	.byte	0
	.byte	0x6
	.4byte	.LASF14
	.byte	0x1
	.byte	0xa
	.byte	0x6
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.byte	0x1
	.byte	0x9c
	.4byte	0x118
	.byte	0x7
	.4byte	.LVL1
	.4byte	0x13e
	.byte	0x8
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x3c
	.byte	0x8
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0
	.byte	0x6
	.4byte	.LASF15
	.byte	0x1
	.byte	0x5
	.byte	0x6
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.byte	0x1
	.byte	0x9c
	.4byte	0x13e
	.byte	0x7
	.4byte	.LVL0
	.4byte	0x14a
	.byte	0x8
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x3c
	.byte	0
	.byte	0
	.byte	0x9
	.4byte	.LASF16
	.4byte	.LASF16
	.byte	0x4
	.byte	0x11
	.byte	0x5
	.byte	0x9
	.4byte	.LASF17
	.4byte	.LASF17
	.byte	0x5
	.byte	0x3c
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
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x6
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
	.byte	0x7
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
	.byte	0x8
	.byte	0x8a,0x82,0x1
	.byte	0
	.byte	0x2
	.byte	0x18
	.byte	0x91,0x42
	.byte	0x18
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
	.byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x34
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
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
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
	.4byte	.LFB7
	.4byte	.LFE7
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
.LASF21:
	.string	"led_stt_val"
.LASF16:
	.string	"bl_gpio_output_set"
.LASF5:
	.string	"long unsigned int"
.LASF7:
	.string	"long long unsigned int"
.LASF10:
	.string	"__uint8_t"
.LASF13:
	.string	"hw_led_stt_off"
.LASF17:
	.string	"gpio_set_output"
.LASF11:
	.string	"uint8_t"
.LASF12:
	.string	"hw_toggle_led_stt"
.LASF9:
	.string	"char"
.LASF4:
	.string	"long int"
.LASF14:
	.string	"hw_led_stt_on"
.LASF3:
	.string	"short unsigned int"
.LASF0:
	.string	"signed char"
.LASF19:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/led.c"
.LASF20:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/hardware"
.LASF2:
	.string	"short int"
.LASF15:
	.string	"hw_led_stt_cfg"
.LASF18:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
	.ident	"GCC: (SiFive GCC-Metal 10.2.0-2020.12.8) 10.2.0"
