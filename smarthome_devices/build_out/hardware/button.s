	.file	"button.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.hw_get_tick_ms,"ax",@progbits
	.align	1
	.globl	hw_get_tick_ms
	.type	hw_get_tick_ms, @function
hw_get_tick_ms:
.LFB5:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/button.c"
	.loc 1 10 31
	.cfi_startproc
	.loc 1 10 33
	.loc 1 10 53 is_stmt 0
	li	a0,0
	ret
	.cfi_endproc
.LFE5:
	.size	hw_get_tick_ms, .-hw_get_tick_ms
	.section	.text.hw_timer_init,"ax",@progbits
	.align	1
	.globl	hw_timer_init
	.type	hw_timer_init, @function
hw_timer_init:
.LFB6:
	.loc 1 13 1 is_stmt 1
	.cfi_startproc
	.loc 1 16 1
	ret
	.cfi_endproc
.LFE6:
	.size	hw_timer_init, .-hw_timer_init
	.section	.rodata.hw_button_config.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.string	"hw_button_config: %ld\n"
	.section	.text.hw_button_config,"ax",@progbits
	.align	1
	.globl	hw_button_config
	.type	hw_button_config, @function
hw_button_config:
.LFB7:
	.loc 1 19 1
	.cfi_startproc
.LVL0:
	.loc 1 20 5
	.loc 1 20 15 is_stmt 0
	lui	a5,%hi(max_number_channel)
	lbu	a4,%lo(max_number_channel)(a5)
	andi	a4,a4,0xff
	.loc 1 20 8
	bleu	a4,a0,.L9
	.loc 1 23 21
	lui	a4,%hi(device_type)
	lbu	a4,%lo(device_type)(a4)
	.loc 1 19 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	.loc 1 23 21
	andi	a4,a4,0xff
	mv	s0,a0
	.loc 1 23 5 is_stmt 1
	.loc 1 23 8 is_stmt 0
	bne	a4,zero,.L5
	.loc 1 25 9 is_stmt 1
	.loc 1 25 32 is_stmt 0
	lbu	a4,%lo(max_number_channel)(a5)
	.loc 1 25 12
	li	a3,1
	.loc 1 25 32
	andi	a4,a4,0xff
	.loc 1 25 12
	bne	a4,a3,.L6
	.loc 1 27 13 is_stmt 1
	lui	a5,%hi(.LANCHOR0+4)
	lw	a0,%lo(.LANCHOR0+4)(a5)
.LVL1:
.L12:
	.loc 1 46 1 is_stmt 0
	lw	s0,8(sp)
	.cfi_remember_state
	.cfi_restore 8
	lw	ra,12(sp)
	.cfi_restore 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	.loc 1 44 9
	tail	gpio_set_output
.LVL2:
.L6:
	.cfi_restore_state
	.loc 1 29 14 is_stmt 1
	.loc 1 29 37 is_stmt 0
	lbu	a5,%lo(max_number_channel)(a5)
	.loc 1 29 17
	li	a4,2
	.loc 1 29 37
	andi	a5,a5,0xff
	.loc 1 29 17
	bne	a5,a4,.L7
	.loc 1 31 13 is_stmt 1
	.loc 1 31 16 is_stmt 0
	bne	a0,a3,.L3
.L7:
	.loc 1 33 17 is_stmt 1
	lui	a5,%hi(.LANCHOR0+12)
	lw	a0,%lo(.LANCHOR0+12)(a5)
	j	.L12
.L5:
	.loc 1 43 9
	mv	a1,a0
	lui	a0,%hi(.LC0)
	addi	a0,a0,%lo(.LC0)
	call	printf
.LVL3:
	.loc 1 44 9
	.loc 1 44 35 is_stmt 0
	lui	a1,%hi(.LANCHOR0)
	slli	s0,s0,2
.LVL4:
	addi	a1,a1,%lo(.LANCHOR0)
	add	s0,a1,s0
	.loc 1 44 9
	lw	a0,0(s0)
	j	.L12
.LVL5:
.L3:
	.loc 1 46 1
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s0,8(sp)
	.cfi_restore 8
.LVL6:
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
.L9:
	ret
	.cfi_endproc
.LFE7:
	.size	hw_button_config, .-hw_button_config
	.section	.text.hw_btn_read,"ax",@progbits
	.align	1
	.globl	hw_btn_read
	.type	hw_btn_read, @function
hw_btn_read:
.LFB8:
	.loc 1 49 1 is_stmt 1
	.cfi_startproc
.LVL7:
	.loc 1 50 5
	.loc 1 50 15 is_stmt 0
	lui	a4,%hi(max_number_channel)
	lbu	a5,%lo(max_number_channel)(a4)
	andi	a5,a5,0xff
	.loc 1 50 8
	bleu	a5,a0,.L14
	.loc 1 53 5 is_stmt 1
	.loc 1 53 21 is_stmt 0
	lui	a5,%hi(device_type)
	lbu	a3,%lo(device_type)(a5)
	lui	a5,%hi(.LANCHOR0)
	addi	a5,a5,%lo(.LANCHOR0)
	andi	a3,a3,0xff
	.loc 1 53 8
	bne	a3,zero,.L15
	.loc 1 55 9 is_stmt 1
	.loc 1 55 32 is_stmt 0
	lbu	a3,%lo(max_number_channel)(a4)
	.loc 1 55 12
	li	a2,1
	.loc 1 55 32
	andi	a3,a3,0xff
	.loc 1 55 12
	bne	a3,a2,.L16
	.loc 1 57 13 is_stmt 1
	.loc 1 57 30 is_stmt 0
	lbu	a0,4(a5)
.LVL8:
.L17:
	.loc 1 68 22
	tail	bl_gpio_input_get_value
.LVL9:
.L16:
	.loc 1 59 14 is_stmt 1
	.loc 1 59 37 is_stmt 0
	lbu	a4,%lo(max_number_channel)(a4)
	.loc 1 59 17
	li	a3,2
	.loc 1 59 37
	andi	a4,a4,0xff
	.loc 1 59 17
	bne	a4,a3,.L15
	.loc 1 61 13 is_stmt 1
	.loc 1 61 16 is_stmt 0
	bne	a0,a2,.L15
	.loc 1 63 17 is_stmt 1
	.loc 1 63 34 is_stmt 0
	lbu	a0,8(a5)
.LVL10:
	j	.L17
.LVL11:
.L15:
	.loc 1 68 5 is_stmt 1
	.loc 1 68 56 is_stmt 0
	slli	a0,a0,2
.LVL12:
	add	a5,a5,a0
	.loc 1 68 22
	lbu	a0,0(a5)
	j	.L17
.LVL13:
.L14:
	.loc 1 69 1
	li	a0,0
.LVL14:
	ret
	.cfi_endproc
.LFE8:
	.size	hw_btn_read, .-hw_btn_read
	.section	.rodata.hw_button_match_pcb.str1.4,"aMS",@progbits,1
	.align	2
.LC1:
	.string	"lahu 01\n"
	.align	2
.LC2:
	.string	"lahu 02\n"
	.section	.text.hw_button_match_pcb,"ax",@progbits
	.align	1
	.globl	hw_button_match_pcb
	.type	hw_button_match_pcb, @function
hw_button_match_pcb:
.LFB9:
	.loc 1 73 1 is_stmt 1
	.cfi_startproc
	.loc 1 74 5
	.loc 1 74 21 is_stmt 0
	lui	a4,%hi(device_type)
	lbu	a5,%lo(device_type)(a4)
	andi	a5,a5,0xff
	.loc 1 74 8
	bne	a5,zero,.L19
	.loc 1 76 9 is_stmt 1
	lui	a5,%hi(max_number_channel)
	lbu	a5,%lo(max_number_channel)(a5)
	li	a4,4
	andi	a5,a5,0xff
	bne	a5,a4,.L24
	.loc 1 92 13
	.loc 1 92 27 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	addi	a5,a5,%lo(.LANCHOR0)
	li	a4,8
	sw	a4,0(a5)
	.loc 1 93 13 is_stmt 1
	.loc 1 93 27 is_stmt 0
	sw	a4,4(a5)
	.loc 1 94 13 is_stmt 1
	.loc 1 94 27 is_stmt 0
	li	a4,5
	sw	a4,8(a5)
	.loc 1 95 13 is_stmt 1
	.loc 1 95 27 is_stmt 0
	li	a4,1
	sw	a4,12(a5)
	.loc 1 97 9 is_stmt 1
	ret
.L19:
	.loc 1 103 10
	.loc 1 73 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	.cfi_offset 8, -8
	.loc 1 103 26
	lbu	s0,%lo(device_type)(a4)
	.loc 1 73 1
	sw	ra,12(sp)
	sw	s1,4(sp)
	.cfi_offset 1, -4
	.cfi_offset 9, -12
	.loc 1 103 26
	andi	s0,s0,0xff
	.loc 1 103 13
	li	a5,1
	bne	s0,a5,.L18
	.loc 1 105 9 is_stmt 1
	lui	a5,%hi(max_number_channel)
	lbu	s1,%lo(max_number_channel)(a5)
	li	a5,2
	andi	s1,s1,0xff
	beq	s1,a5,.L21
	li	a5,4
	beq	s1,a5,.L22
	bne	s1,s0,.L18
	.loc 1 109 13
	lui	a0,%hi(.LC1)
	addi	a0,a0,%lo(.LC1)
	call	printf
.LVL15:
	.loc 1 110 13
	.loc 1 110 27 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	addi	a5,a5,%lo(.LANCHOR0)
	li	a4,8
	sw	a4,0(a5)
	.loc 1 111 13 is_stmt 1
	.loc 1 111 27 is_stmt 0
	sw	a4,4(a5)
	.loc 1 112 13 is_stmt 1
	.loc 1 112 27 is_stmt 0
	li	a4,5
	sw	a4,8(a5)
	.loc 1 113 13 is_stmt 1
	.loc 1 113 27 is_stmt 0
	sw	s1,12(a5)
	.loc 1 115 9 is_stmt 1
.L18:
	.loc 1 142 1 is_stmt 0
	lw	ra,12(sp)
	.cfi_remember_state
	.cfi_restore 1
	lw	s0,8(sp)
	.cfi_restore 8
	lw	s1,4(sp)
	.cfi_restore 9
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
.L21:
	.cfi_restore_state
	.loc 1 118 13 is_stmt 1
	lui	a0,%hi(.LC2)
	addi	a0,a0,%lo(.LC2)
	call	printf
.LVL16:
	.loc 1 119 13
	.loc 1 119 27 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	addi	a5,a5,%lo(.LANCHOR0)
	li	a4,8
	sw	a4,0(a5)
	.loc 1 120 13 is_stmt 1
	.loc 1 120 27 is_stmt 0
	sw	a4,4(a5)
	.loc 1 121 13 is_stmt 1
	.loc 1 121 27 is_stmt 0
	li	a4,5
	sw	a4,8(a5)
	.loc 1 122 13 is_stmt 1
	.loc 1 122 27 is_stmt 0
	sw	s0,12(a5)
	.loc 1 124 9 is_stmt 1
	j	.L18
.L22:
	.loc 1 131 13
	.loc 1 131 27 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	addi	a5,a5,%lo(.LANCHOR0)
	li	a4,8
	sw	a4,0(a5)
	.loc 1 132 13 is_stmt 1
	.loc 1 132 27 is_stmt 0
	sw	a4,4(a5)
	.loc 1 133 13 is_stmt 1
	.loc 1 134 27 is_stmt 0
	li	a4,5
	.loc 1 133 27
	sw	s0,8(a5)
	.loc 1 134 13 is_stmt 1
	.loc 1 134 27 is_stmt 0
	sw	a4,12(a5)
	.loc 1 136 9 is_stmt 1
	.loc 1 142 1 is_stmt 0
	j	.L18
.L24:
	.cfi_def_cfa_offset 0
	.cfi_restore 1
	.cfi_restore 8
	.cfi_restore 9
	ret
	.cfi_endproc
.LFE9:
	.size	hw_button_match_pcb, .-hw_button_match_pcb
	.globl	button_arr
	.section	.data.button_arr,"aw"
	.align	2
	.set	.LANCHOR0,. + 0
	.type	button_arr, @object
	.size	button_arr, 16
button_arr:
	.word	1
	.word	5
	.word	8
	.word	8
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/machine/_default_types.h"
	.file 3 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/sys/_stdint.h"
	.file 4 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.h"
	.file 5 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/stdio.h"
	.file 6 "/home/dungnt98/Ai-Thinker-WB2/components/platform/hosal/bl602_hal/bl_gpio.h"
	.file 7 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/app_config/system_typedef.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x22c
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.byte	0x1
	.4byte	.LASF26
	.byte	0xc
	.4byte	.LASF27
	.4byte	.LASF28
	.4byte	.Ldebug_ranges0+0
	.4byte	0
	.4byte	.Ldebug_line0
	.byte	0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF0
	.byte	0x3
	.4byte	.LASF5
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
	.byte	0x3
	.4byte	.LASF6
	.byte	0x2
	.byte	0x4f
	.byte	0x1b
	.4byte	0x60
	.byte	0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF7
	.byte	0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF8
	.byte	0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF9
	.byte	0x4
	.byte	0x4
	.byte	0x5
	.string	"int"
	.byte	0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF10
	.byte	0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF11
	.byte	0x3
	.4byte	.LASF12
	.byte	0x3
	.byte	0x18
	.byte	0x13
	.4byte	0x2c
	.byte	0x5
	.4byte	0x8a
	.byte	0x3
	.4byte	.LASF13
	.byte	0x3
	.byte	0x30
	.byte	0x14
	.4byte	0x54
	.byte	0x6
	.4byte	.LASF14
	.byte	0x4
	.byte	0x20
	.byte	0x19
	.4byte	0x96
	.byte	0x6
	.4byte	.LASF15
	.byte	0x4
	.byte	0x21
	.byte	0x19
	.4byte	0x96
	.byte	0x7
	.4byte	0x9b
	.4byte	0xca
	.byte	0x8
	.byte	0
	.byte	0x6
	.4byte	.LASF16
	.byte	0x4
	.byte	0x23
	.byte	0x11
	.4byte	0xbf
	.byte	0x9
	.byte	0x7
	.byte	0x4
	.4byte	0x7c
	.byte	0x7
	.byte	0x28
	.byte	0xe
	.4byte	0xf7
	.byte	0xa
	.4byte	.LASF17
	.byte	0
	.byte	0xa
	.4byte	.LASF18
	.byte	0x1
	.byte	0xa
	.4byte	.LASF19
	.byte	0x2
	.byte	0
	.byte	0x7
	.4byte	0x9b
	.4byte	0x107
	.byte	0xb
	.4byte	0x7c
	.byte	0x3
	.byte	0
	.byte	0xc
	.4byte	0xca
	.byte	0x1
	.byte	0x4
	.byte	0xa
	.4byte	0xf7
	.byte	0x5
	.byte	0x3
	.4byte	button_arr
	.byte	0xd
	.4byte	.LASF29
	.byte	0x1
	.byte	0x6
	.byte	0x11
	.4byte	0x9b
	.byte	0xe
	.4byte	.LASF20
	.byte	0x1
	.byte	0x48
	.byte	0x6
	.4byte	.LFB9
	.4byte	.LFE9-.LFB9
	.byte	0x1
	.byte	0x9c
	.4byte	0x166
	.byte	0xf
	.4byte	.LVL15
	.4byte	0x20b
	.4byte	0x152
	.byte	0x10
	.byte	0x1
	.byte	0x5a
	.byte	0x5
	.byte	0x3
	.4byte	.LC1
	.byte	0
	.byte	0x11
	.4byte	.LVL16
	.4byte	0x20b
	.byte	0x10
	.byte	0x1
	.byte	0x5a
	.byte	0x5
	.byte	0x3
	.4byte	.LC2
	.byte	0
	.byte	0
	.byte	0x12
	.4byte	.LASF30
	.byte	0x1
	.byte	0x30
	.byte	0xa
	.4byte	0x9b
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.byte	0x1
	.byte	0x9c
	.4byte	0x19a
	.byte	0x13
	.4byte	.LASF22
	.byte	0x1
	.byte	0x30
	.byte	0x1f
	.4byte	0x9b
	.4byte	.LLST1
	.byte	0x14
	.4byte	.LVL9
	.4byte	0x217
	.byte	0
	.byte	0xe
	.4byte	.LASF21
	.byte	0x1
	.byte	0x12
	.byte	0x6
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.byte	0x1
	.byte	0x9c
	.4byte	0x1e3
	.byte	0x13
	.4byte	.LASF22
	.byte	0x1
	.byte	0x12
	.byte	0x20
	.4byte	0x9b
	.4byte	.LLST0
	.byte	0x14
	.4byte	.LVL2
	.4byte	0x223
	.byte	0x11
	.4byte	.LVL3
	.4byte	0x20b
	.byte	0x10
	.byte	0x1
	.byte	0x5a
	.byte	0x5
	.byte	0x3
	.4byte	.LC0
	.byte	0x10
	.byte	0x1
	.byte	0x5b
	.byte	0x2
	.byte	0x78
	.byte	0
	.byte	0
	.byte	0
	.byte	0x15
	.4byte	.LASF31
	.byte	0x1
	.byte	0xc
	.byte	0x6
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.byte	0x1
	.byte	0x9c
	.byte	0x16
	.4byte	.LASF32
	.byte	0x1
	.byte	0xa
	.byte	0xa
	.4byte	0x9b
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.byte	0x1
	.byte	0x9c
	.byte	0x17
	.4byte	.LASF23
	.4byte	.LASF23
	.byte	0x5
	.byte	0xc8
	.byte	0x5
	.byte	0x17
	.4byte	.LASF24
	.4byte	.LASF24
	.byte	0x6
	.byte	0x13
	.byte	0x5
	.byte	0x17
	.4byte	.LASF25
	.4byte	.LASF25
	.byte	0x4
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
	.byte	0x35
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x6
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
	.byte	0x7
	.byte	0x1
	.byte	0x1
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x8
	.byte	0x21
	.byte	0
	.byte	0
	.byte	0
	.byte	0x9
	.byte	0x4
	.byte	0x1
	.byte	0x3e
	.byte	0xb
	.byte	0xb
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xa
	.byte	0x28
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x1c
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0xb
	.byte	0x21
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0x2f
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0xc
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
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0xd
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
	.byte	0
	.byte	0
	.byte	0xe
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
	.byte	0xf
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
	.byte	0x10
	.byte	0x8a,0x82,0x1
	.byte	0
	.byte	0x2
	.byte	0x18
	.byte	0x91,0x42
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x11
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x12
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
	.byte	0x49
	.byte	0x13
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
	.byte	0x13
	.byte	0x5
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
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x14
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
	.byte	0x15
	.byte	0x2e
	.byte	0
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
	.byte	0
	.byte	0
	.byte	0x16
	.byte	0x2e
	.byte	0
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
	.byte	0x49
	.byte	0x13
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x6
	.byte	0x40
	.byte	0x18
	.byte	0x97,0x42
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0x17
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
	.4byte	.LVL7
	.4byte	.LVL8
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL8
	.4byte	.LVL9
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL9
	.4byte	.LVL10
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL10
	.4byte	.LVL11
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL11
	.4byte	.LVL12
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL12
	.4byte	.LVL13
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL13
	.4byte	.LVL14
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL14
	.4byte	.LFE8
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
	.4byte	.LVL2
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL2
	.4byte	.LVL4
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL4
	.4byte	.LVL5
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL5
	.4byte	.LVL6
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL6
	.4byte	.LFE7
	.2byte	0x1
	.byte	0x5a
	.4byte	0
	.4byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x3c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.4byte	.LFB9
	.4byte	.LFE9-.LFB9
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.4byte	.LFB5
	.4byte	.LFE5
	.4byte	.LFB6
	.4byte	.LFE6
	.4byte	.LFB7
	.4byte	.LFE7
	.4byte	.LFB8
	.4byte	.LFE8
	.4byte	.LFB9
	.4byte	.LFE9
	.4byte	0
	.4byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF24:
	.string	"bl_gpio_input_get_value"
.LASF5:
	.string	"__uint8_t"
.LASF16:
	.string	"button_arr"
.LASF21:
	.string	"hw_button_config"
.LASF29:
	.string	"btn_tick_ms"
.LASF28:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/hardware"
.LASF30:
	.string	"hw_btn_read"
.LASF27:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/button.c"
.LASF1:
	.string	"unsigned char"
.LASF7:
	.string	"long unsigned int"
.LASF31:
	.string	"hw_timer_init"
.LASF3:
	.string	"short unsigned int"
.LASF32:
	.string	"hw_get_tick_ms"
.LASF25:
	.string	"gpio_set_output"
.LASF6:
	.string	"__uint32_t"
.LASF15:
	.string	"device_type"
.LASF18:
	.string	"LHSWITCH"
.LASF10:
	.string	"unsigned int"
.LASF9:
	.string	"long long unsigned int"
.LASF12:
	.string	"uint8_t"
.LASF20:
	.string	"hw_button_match_pcb"
.LASF8:
	.string	"long long int"
.LASF19:
	.string	"WSDATIC"
.LASF26:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
.LASF11:
	.string	"char"
.LASF22:
	.string	"index"
.LASF23:
	.string	"printf"
.LASF2:
	.string	"short int"
.LASF13:
	.string	"uint32_t"
.LASF4:
	.string	"long int"
.LASF0:
	.string	"signed char"
.LASF17:
	.string	"WSWITCH"
.LASF14:
	.string	"max_number_channel"
	.ident	"GCC: (SiFive GCC-Metal 10.2.0-2020.12.8) 10.2.0"
