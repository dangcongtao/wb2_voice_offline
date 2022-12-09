	.file	"switch.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.hw_switch_cfg,"ax",@progbits
	.align	1
	.globl	hw_switch_cfg
	.type	hw_switch_cfg, @function
hw_switch_cfg:
.LFB4:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/switch.c"
	.loc 1 7 1
	.cfi_startproc
.LVL0:
	.loc 1 8 5
	.loc 1 7 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	.loc 1 7 1
	mv	s0,a0
	.loc 1 8 5
	call	gpio_set_output
.LVL1:
	.loc 1 9 5 is_stmt 1
	.loc 1 9 34 is_stmt 0
	lui	a0,%hi(.LANCHOR0)
	slli	s0,s0,2
	addi	a0,a0,%lo(.LANCHOR0)
	add	a0,a0,s0
	.loc 1 10 1
	lw	s0,8(sp)
	.cfi_restore 8
	lw	ra,12(sp)
	.cfi_restore 1
	.loc 1 9 5
	lbu	a0,0(a0)
	li	a1,0
	.loc 1 10 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	.loc 1 9 5
	tail	bl_gpio_output_set
.LVL2:
	.cfi_endproc
.LFE4:
	.size	hw_switch_cfg, .-hw_switch_cfg
	.section	.text.hw_write_switch_pin,"ax",@progbits
	.align	1
	.globl	hw_write_switch_pin
	.type	hw_write_switch_pin, @function
hw_write_switch_pin:
.LFB5:
	.loc 1 13 1 is_stmt 1
	.cfi_startproc
.LVL3:
	.loc 1 14 5
	.loc 1 13 1 is_stmt 0
	addi	sp,sp,-32
	.cfi_def_cfa_offset 32
	sw	s3,12(sp)
	.cfi_offset 19, -20
	.loc 1 14 15
	lui	s3,%hi(max_number_channel)
	lbu	a5,%lo(max_number_channel)(s3)
	.loc 1 13 1
	sw	ra,28(sp)
	sw	s0,24(sp)
	sw	s1,20(sp)
	sw	s2,16(sp)
	sw	s4,8(sp)
	sw	s5,4(sp)
	.cfi_offset 1, -4
	.cfi_offset 8, -8
	.cfi_offset 9, -12
	.cfi_offset 18, -16
	.cfi_offset 20, -24
	.cfi_offset 21, -28
	.loc 1 14 15
	andi	a5,a5,0xff
	.loc 1 14 8
	bleu	a5,a0,.L3
	.loc 1 17 34
	lui	s1,%hi(.LANCHOR0)
	addi	s4,s1,%lo(.LANCHOR0)
	slli	a5,a0,2
	add	a5,s4,a5
	mv	s0,a0
	.loc 1 17 5
	lbu	a0,0(a5)
.LVL4:
	mv	s2,a1
	.loc 1 17 5 is_stmt 1
	addi	s1,s1,%lo(.LANCHOR0)
	call	bl_gpio_output_set
.LVL5:
	.loc 1 19 5
	.loc 1 19 21 is_stmt 0
	lui	a5,%hi(device_type)
	lbu	a5,%lo(device_type)(a5)
	andi	a5,a5,0xff
	.loc 1 19 8
	bne	a5,zero,.L3
	.loc 1 20 32
	lbu	a5,%lo(max_number_channel)(s3)
	.loc 1 20 12
	li	a4,1
	mv	s5,s3
	.loc 1 20 9 is_stmt 1
	.loc 1 20 32 is_stmt 0
	andi	a5,a5,0xff
	.loc 1 20 12
	bne	a5,a4,.L7
	.loc 1 20 37 discriminator 1
	bne	s0,zero,.L7
	.loc 1 22 13 is_stmt 1
	lbu	a0,4(s4)
	mv	a1,s2
.L9:
	.loc 1 29 1 is_stmt 0
	lw	s0,24(sp)
	.cfi_remember_state
	.cfi_restore 8
	lw	ra,28(sp)
	.cfi_restore 1
	lw	s1,20(sp)
	.cfi_restore 9
	lw	s2,16(sp)
	.cfi_restore 18
	lw	s3,12(sp)
	.cfi_restore 19
	lw	s4,8(sp)
	.cfi_restore 20
	lw	s5,4(sp)
	.cfi_restore 21
	addi	sp,sp,32
	.cfi_def_cfa_offset 0
	.loc 1 26 13
	tail	bl_gpio_output_set
.LVL6:
.L7:
	.cfi_restore_state
	.loc 1 24 14 is_stmt 1
	.loc 1 24 37 is_stmt 0
	lbu	a5,%lo(max_number_channel)(s5)
	.loc 1 24 17
	li	a4,2
	.loc 1 24 37
	andi	a5,a5,0xff
	.loc 1 24 17
	bne	a5,a4,.L3
	.loc 1 24 42 discriminator 1
	li	a5,1
	bne	s0,a5,.L3
	.loc 1 26 13 is_stmt 1
	lbu	a0,8(s1)
	mv	a1,s2
	j	.L9
.L3:
	.loc 1 29 1 is_stmt 0
	lw	ra,28(sp)
	.cfi_restore 1
	lw	s0,24(sp)
	.cfi_restore 8
	lw	s1,20(sp)
	.cfi_restore 9
	lw	s2,16(sp)
	.cfi_restore 18
	lw	s3,12(sp)
	.cfi_restore 19
	lw	s4,8(sp)
	.cfi_restore 20
	lw	s5,4(sp)
	.cfi_restore 21
	addi	sp,sp,32
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE5:
	.size	hw_write_switch_pin, .-hw_write_switch_pin
	.section	.text.hw_read_switch_pin,"ax",@progbits
	.align	1
	.globl	hw_read_switch_pin
	.type	hw_read_switch_pin, @function
hw_read_switch_pin:
.LFB6:
	.loc 1 32 1 is_stmt 1
	.cfi_startproc
.LVL7:
	.loc 1 33 5
	.loc 1 33 15 is_stmt 0
	lui	a5,%hi(max_number_channel)
	lbu	a5,%lo(max_number_channel)(a5)
	andi	a5,a5,0xff
	.loc 1 33 8
	bleu	a5,a0,.L12
	.loc 1 36 5 is_stmt 1
	.loc 1 36 55 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	.loc 1 32 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	.loc 1 36 55
	addi	a5,a5,%lo(.LANCHOR0)
	slli	a0,a0,2
.LVL8:
	.loc 1 32 1
	sw	ra,12(sp)
	.cfi_offset 1, -4
	.loc 1 36 55
	add	a0,a5,a0
	.loc 1 36 21
	lbu	a0,0(a0)
	call	bl_gpio_input_get_value
.LVL9:
	.loc 1 37 1
	lw	ra,12(sp)
	.cfi_restore 1
	.loc 1 36 12
	andi	a0,a0,0xff
	.loc 1 37 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
.LVL10:
.L12:
	.loc 1 34 16
	li	a0,0
.LVL11:
	.loc 1 37 1
	ret
	.cfi_endproc
.LFE6:
	.size	hw_read_switch_pin, .-hw_read_switch_pin
	.section	.text.hw_switch_match_pcb,"ax",@progbits
	.align	1
	.globl	hw_switch_match_pcb
	.type	hw_switch_match_pcb, @function
hw_switch_match_pcb:
.LFB7:
	.loc 1 41 1 is_stmt 1
	.cfi_startproc
	.loc 1 42 5
	.loc 1 42 22 is_stmt 0
	lui	a4,%hi(device_type)
	lbu	a5,%lo(device_type)(a4)
	andi	a5,a5,0xff
	.loc 1 42 8
	beq	a5,zero,.L18
	.loc 1 42 48 discriminator 2
	lbu	a5,%lo(device_type)(a4)
	.loc 1 42 33 discriminator 2
	li	a4,2
	.loc 1 42 48 discriminator 2
	andi	a5,a5,0xff
	.loc 1 42 33 discriminator 2
	bne	a5,a4,.L17
.L18:
	.loc 1 42 82 discriminator 3
	lui	a5,%hi(max_number_channel)
	lbu	a5,%lo(max_number_channel)(a5)
	.loc 1 42 60 discriminator 3
	li	a4,4
	.loc 1 42 82 discriminator 3
	andi	a5,a5,0xff
	.loc 1 42 60 discriminator 3
	bne	a5,a4,.L17
	.loc 1 43 3 is_stmt 1
	.loc 1 43 17 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	addi	a5,a5,%lo(.LANCHOR0)
	li	a4,3
	sw	a4,8(a5)
	.loc 1 44 3 is_stmt 1
	.loc 1 44 17 is_stmt 0
	sw	a4,12(a5)
.L17:
	.loc 1 46 1
	ret
	.cfi_endproc
.LFE7:
	.size	hw_switch_match_pcb, .-hw_switch_match_pcb
	.globl	switch_arr
	.section	.data.switch_arr,"aw"
	.align	2
	.set	.LANCHOR0,. + 0
	.type	switch_arr, @object
	.size	switch_arr, 16
switch_arr:
	.word	14
	.word	17
	.word	3
	.word	3
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/machine/_default_types.h"
	.file 3 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/riscv64-unknown-elf/include/sys/_stdint.h"
	.file 4 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.h"
	.file 5 "/home/dungnt98/Ai-Thinker-WB2/components/platform/hosal/bl602_hal/bl_gpio.h"
	.file 6 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/app_config/system_typedef.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x21e
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
	.byte	0x22
	.byte	0x11
	.4byte	0xbf
	.byte	0x9
	.byte	0x7
	.byte	0x4
	.4byte	0x7c
	.byte	0x6
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
	.4byte	switch_arr
	.byte	0xd
	.4byte	.LASF29
	.byte	0x1
	.byte	0x28
	.byte	0x6
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.byte	0x1
	.byte	0x9c
	.byte	0xe
	.4byte	.LASF30
	.byte	0x1
	.byte	0x1f
	.byte	0x9
	.4byte	0x8a
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.byte	0x1
	.byte	0x9c
	.4byte	0x15f
	.byte	0xf
	.4byte	.LASF20
	.byte	0x1
	.byte	0x1f
	.byte	0x24
	.4byte	0x8a
	.4byte	.LLST3
	.byte	0x10
	.4byte	.LVL9
	.4byte	0x1fd
	.byte	0
	.byte	0x11
	.4byte	.LASF21
	.byte	0x1
	.byte	0xc
	.byte	0x6
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.byte	0x1
	.byte	0x9c
	.4byte	0x1b3
	.byte	0xf
	.4byte	.LASF20
	.byte	0x1
	.byte	0xc
	.byte	0x22
	.4byte	0x8a
	.4byte	.LLST1
	.byte	0x12
	.string	"val"
	.byte	0x1
	.byte	0xc
	.byte	0x31
	.4byte	0x8a
	.4byte	.LLST2
	.byte	0x13
	.4byte	.LVL5
	.4byte	0x209
	.4byte	0x1a9
	.byte	0x14
	.byte	0x1
	.byte	0x5b
	.byte	0x2
	.byte	0x82
	.byte	0
	.byte	0
	.byte	0x15
	.4byte	.LVL6
	.4byte	0x209
	.byte	0
	.byte	0x11
	.4byte	.LASF22
	.byte	0x1
	.byte	0x6
	.byte	0x6
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.byte	0x1
	.byte	0x9c
	.4byte	0x1fd
	.byte	0xf
	.4byte	.LASF20
	.byte	0x1
	.byte	0x6
	.byte	0x1c
	.4byte	0x8a
	.4byte	.LLST0
	.byte	0x13
	.4byte	.LVL1
	.4byte	0x215
	.4byte	0x1ed
	.byte	0x14
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x78
	.byte	0
	.byte	0
	.byte	0x16
	.4byte	.LVL2
	.4byte	0x209
	.byte	0x14
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0
	.byte	0x17
	.4byte	.LASF23
	.4byte	.LASF23
	.byte	0x5
	.byte	0x13
	.byte	0x5
	.byte	0x17
	.4byte	.LASF24
	.4byte	.LASF24
	.byte	0x5
	.byte	0x11
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
	.byte	0xf
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
	.byte	0x10
	.byte	0x89,0x82,0x1
	.byte	0
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x11
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
	.byte	0x12
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
	.byte	0x13
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
	.byte	0x14
	.byte	0x8a,0x82,0x1
	.byte	0
	.byte	0x2
	.byte	0x18
	.byte	0x91,0x42
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x15
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
	.byte	0x16
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
.LLST3:
	.4byte	.LVL7
	.4byte	.LVL8
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL8
	.4byte	.LVL10
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL10
	.4byte	.LVL11
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL11
	.4byte	.LFE6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
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
.LLST2:
	.4byte	.LVL3
	.4byte	.LVL5-1
	.2byte	0x1
	.byte	0x5b
	.4byte	.LVL5-1
	.4byte	.LFE5
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5b
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST0:
	.4byte	.LVL0
	.4byte	.LVL1-1
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL1-1
	.4byte	.LFE4
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
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
.LASF23:
	.string	"bl_gpio_input_get_value"
.LASF5:
	.string	"__uint8_t"
.LASF29:
	.string	"hw_switch_match_pcb"
.LASF28:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/hardware"
.LASF1:
	.string	"unsigned char"
.LASF27:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/switch.c"
.LASF7:
	.string	"long unsigned int"
.LASF3:
	.string	"short unsigned int"
.LASF25:
	.string	"gpio_set_output"
.LASF16:
	.string	"switch_arr"
.LASF6:
	.string	"__uint32_t"
.LASF24:
	.string	"bl_gpio_output_set"
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
.LASF21:
	.string	"hw_write_switch_pin"
.LASF8:
	.string	"long long int"
.LASF19:
	.string	"WSDATIC"
.LASF26:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
.LASF11:
	.string	"char"
.LASF20:
	.string	"index"
.LASF22:
	.string	"hw_switch_cfg"
.LASF2:
	.string	"short int"
.LASF30:
	.string	"hw_read_switch_pin"
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
