	.file	"m_switch.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.m_control_switch,"ax",@progbits
	.align	1
	.type	m_control_switch, @function
m_control_switch:
.LFB5:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_switch.c"
	.loc 1 57 1
	.cfi_startproc
.LVL0:
	.loc 1 99 5
	.loc 1 100 1 is_stmt 0
	li	a0,1
.LVL1:
	ret
	.cfi_endproc
.LFE5:
	.size	m_control_switch, .-m_control_switch
	.section	.text.m_toggle_switch,"ax",@progbits
	.align	1
	.type	m_toggle_switch, @function
m_toggle_switch:
.LFB6:
	.loc 1 104 1 is_stmt 1
	.cfi_startproc
.LVL2:
	.loc 1 105 5
	.loc 1 105 37 is_stmt 0
	lui	a5,%hi(.LANCHOR0+8)
	.loc 1 105 25
	lw	a5,%lo(.LANCHOR0+8)(a5)
	.loc 1 104 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	.loc 1 104 1
	mv	s0,a0
	.loc 1 105 25
	andi	a0,a0,0xff
.LVL3:
	jalr	a5
.LVL4:
	.loc 1 110 5 is_stmt 1
	.loc 1 162 1 is_stmt 0
	lw	ra,12(sp)
	.cfi_restore 1
	sltiu	a0,s0,4
	lw	s0,8(sp)
	.cfi_restore 8
.LVL5:
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE6:
	.size	m_toggle_switch, .-m_toggle_switch
	.section	.text.m_control_get_status_switch,"ax",@progbits
	.align	1
	.globl	m_control_get_status_switch
	.type	m_control_get_status_switch, @function
m_control_get_status_switch:
.LFB4:
	.loc 1 14 1 is_stmt 1
	.cfi_startproc
.LVL6:
	.loc 1 15 5
	.loc 1 15 38 is_stmt 0
	lui	a5,%hi(.LANCHOR0+8)
	.loc 1 15 26
	lw	a5,%lo(.LANCHOR0+8)(a5)
	.loc 1 14 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	.loc 1 14 1
	mv	s0,a0
	.loc 1 15 26
	jalr	a5
.LVL7:
	mv	a5,a0
.LVL8:
	.loc 1 17 5 is_stmt 1
	.loc 1 17 8 is_stmt 0
	bne	s0,zero,.L5
	.loc 1 23 16 discriminator 1
	seqz	a0,a0
.LVL9:
	addi	a0,a0,1
.L4:
	.loc 1 51 1
	lw	ra,12(sp)
	.cfi_remember_state
	.cfi_restore 1
	lw	s0,8(sp)
	.cfi_restore 8
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
.LVL10:
.L5:
	.cfi_restore_state
	.loc 1 21 10 is_stmt 1
	.loc 1 25 10
	.loc 1 25 13 is_stmt 0
	li	a4,1
	bne	s0,a4,.L7
	.loc 1 31 16 discriminator 1
	seqz	a0,a0
.LVL11:
	addi	a0,a0,3
	j	.L4
.LVL12:
.L7:
	.loc 1 29 10 is_stmt 1
	.loc 1 33 10
	.loc 1 33 13 is_stmt 0
	li	a4,2
	bne	s0,a4,.L8
	.loc 1 39 16 discriminator 1
	seqz	a0,a0
.LVL13:
	addi	a0,a0,5
	j	.L4
.LVL14:
.L8:
	.loc 1 37 10 is_stmt 1
	.loc 1 41 10
	.loc 1 41 13 is_stmt 0
	li	a4,3
	.loc 1 50 12
	li	a0,0
.LVL15:
	.loc 1 41 13
	bne	s0,a4,.L4
	.loc 1 47 16 discriminator 1
	seqz	a0,a5
	addi	a0,a0,7
	j	.L4
	.cfi_endproc
.LFE4:
	.size	m_control_get_status_switch, .-m_control_get_status_switch
	.section	.text.m_switch_init,"ax",@progbits
	.align	1
	.globl	m_switch_init
	.type	m_switch_init, @function
m_switch_init:
.LFB8:
	.loc 1 222 1 is_stmt 1
	.cfi_startproc
	.loc 1 223 5
	.loc 1 223 37 is_stmt 0
	lui	a5,%hi(max_number_channel)
	lbu	a3,%lo(max_number_channel)(a5)
	.loc 1 222 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	.loc 1 223 37
	lui	a4,%hi(.LANCHOR0)
	.loc 1 222 1
	sw	s0,8(sp)
	sw	s1,4(sp)
	sw	s2,0(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 9, -12
	.cfi_offset 18, -16
	.cfi_offset 1, -4
	.loc 1 223 37
	addi	a5,a4,%lo(.LANCHOR0)
	sb	a3,0(a5)
	.loc 1 224 5 is_stmt 1
	.loc 1 224 33 is_stmt 0
	lui	a3,%hi(hw_switch_cfg)
	addi	a3,a3,%lo(hw_switch_cfg)
	sw	a3,4(a5)
	.loc 1 225 5 is_stmt 1
	.loc 1 225 34 is_stmt 0
	lui	a3,%hi(hw_read_switch_pin)
	addi	a3,a3,%lo(hw_read_switch_pin)
	sw	a3,8(a5)
	.loc 1 226 5 is_stmt 1
	.loc 1 226 35 is_stmt 0
	lui	a3,%hi(hw_write_switch_pin)
	addi	a3,a3,%lo(hw_write_switch_pin)
	sw	a3,12(a5)
	.loc 1 227 5 is_stmt 1
	.loc 1 227 36 is_stmt 0
	lui	a3,%hi(m_toggle_switch)
	addi	a3,a3,%lo(m_toggle_switch)
	sw	a3,16(a5)
	.loc 1 228 5 is_stmt 1
	.loc 1 228 37 is_stmt 0
	lui	a3,%hi(m_control_switch)
	addi	a3,a3,%lo(m_control_switch)
	sw	a3,20(a5)
	.loc 1 230 5 is_stmt 1
.LVL16:
	.loc 1 231 5
	.loc 1 231 17
	.loc 1 231 12 is_stmt 0
	li	s0,0
	addi	s1,a4,%lo(.LANCHOR0)
	.loc 1 231 5
	li	s2,4
.LVL17:
.L16:
	.loc 1 232 9 is_stmt 1 discriminator 3
	lw	a5,4(s1)
	andi	a0,s0,0xff
	.loc 1 231 25 is_stmt 0 discriminator 3
	addi	s0,s0,1
.LVL18:
	.loc 1 232 9 discriminator 3
	jalr	a5
.LVL19:
	.loc 1 231 24 is_stmt 1 discriminator 3
	.loc 1 231 17 discriminator 3
	.loc 1 231 5 is_stmt 0 discriminator 3
	bne	s0,s2,.L16
	.loc 1 235 1
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s0,8(sp)
	.cfi_restore 8
.LVL20:
	lw	s1,4(sp)
	.cfi_restore 9
	lw	s2,0(sp)
	.cfi_restore 18
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE8:
	.size	m_switch_init, .-m_switch_init
	.section	.text.m_switch_get_instance,"ax",@progbits
	.align	1
	.globl	m_switch_get_instance
	.type	m_switch_get_instance, @function
m_switch_get_instance:
.LFB9:
	.loc 1 238 1 is_stmt 1
	.cfi_startproc
.LVL21:
	.loc 1 239 5
	.loc 1 239 12 is_stmt 0
	lui	a1,%hi(.LANCHOR0)
	.loc 1 238 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	.loc 1 239 12
	li	a2,24
	addi	a1,a1,%lo(.LANCHOR0)
	.loc 1 238 1
	sw	s0,8(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	.loc 1 238 1
	mv	s0,a0
	.loc 1 239 12
	call	memcpy
.LVL22:
	.loc 1 240 1
	lw	ra,12(sp)
	.cfi_restore 1
	mv	a0,s0
	lw	s0,8(sp)
	.cfi_restore 8
.LVL23:
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE9:
	.size	m_switch_get_instance, .-m_switch_get_instance
	.comm	respond_msg,256,4
	.section	.bss.s_switch_obj,"aw",@nobits
	.align	2
	.set	.LANCHOR0,. + 0
	.type	s_switch_obj, @object
	.size	s_switch_obj, 24
s_switch_obj:
	.zero	24
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint-gcc.h"
	.file 3 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/app_config/system_typedef.h"
	.file 4 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_switch.h"
	.file 5 "<built-in>"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x42d
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.byte	0x1
	.4byte	.LASF49
	.byte	0xc
	.4byte	.LASF50
	.4byte	.LASF51
	.4byte	.Ldebug_ranges0+0
	.4byte	0
	.4byte	.Ldebug_line0
	.byte	0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF0
	.byte	0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF1
	.byte	0x2
	.byte	0x4
	.byte	0x5
	.4byte	.LASF2
	.byte	0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF3
	.byte	0x3
	.4byte	.LASF6
	.byte	0x2
	.byte	0x2e
	.byte	0x17
	.4byte	0x52
	.byte	0x4
	.4byte	0x41
	.byte	0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF4
	.byte	0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF5
	.byte	0x3
	.4byte	.LASF7
	.byte	0x2
	.byte	0x34
	.byte	0x1b
	.4byte	0x6c
	.byte	0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF8
	.byte	0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF9
	.byte	0x5
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
	.byte	0x6
	.4byte	.LASF32
	.byte	0x3
	.byte	0x2f
	.byte	0x19
	.4byte	0x4d
	.byte	0x7
	.byte	0x7
	.byte	0x4
	.4byte	0x81
	.byte	0x3
	.byte	0x38
	.byte	0xe
	.4byte	0xda
	.byte	0x8
	.4byte	.LASF12
	.byte	0x1
	.byte	0x8
	.4byte	.LASF13
	.byte	0x2
	.byte	0x8
	.4byte	.LASF14
	.byte	0x3
	.byte	0x8
	.4byte	.LASF15
	.byte	0x4
	.byte	0x8
	.4byte	.LASF16
	.byte	0x5
	.byte	0x8
	.4byte	.LASF17
	.byte	0x6
	.byte	0x8
	.4byte	.LASF18
	.byte	0x7
	.byte	0x8
	.4byte	.LASF19
	.byte	0x8
	.byte	0
	.byte	0x3
	.4byte	.LASF20
	.byte	0x3
	.byte	0x41
	.byte	0x3
	.4byte	0x9b
	.byte	0x7
	.byte	0x7
	.byte	0x4
	.4byte	0x81
	.byte	0x4
	.byte	0xb
	.byte	0xe
	.4byte	0x10d
	.byte	0x8
	.4byte	.LASF21
	.byte	0
	.byte	0x8
	.4byte	.LASF22
	.byte	0x1
	.byte	0x8
	.4byte	.LASF23
	.byte	0x2
	.byte	0x8
	.4byte	.LASF24
	.byte	0x3
	.byte	0
	.byte	0x3
	.4byte	.LASF25
	.byte	0x4
	.byte	0x12
	.byte	0x19
	.4byte	0xda
	.byte	0x3
	.4byte	.LASF26
	.byte	0x4
	.byte	0x32
	.byte	0xf
	.4byte	0x125
	.byte	0x9
	.byte	0x4
	.4byte	0x12b
	.byte	0xa
	.4byte	0x136
	.byte	0xb
	.4byte	0x41
	.byte	0
	.byte	0x3
	.4byte	.LASF27
	.byte	0x4
	.byte	0x33
	.byte	0x12
	.4byte	0x142
	.byte	0x9
	.byte	0x4
	.4byte	0x148
	.byte	0xc
	.4byte	0x41
	.4byte	0x157
	.byte	0xb
	.4byte	0x41
	.byte	0
	.byte	0x3
	.4byte	.LASF28
	.byte	0x4
	.byte	0x34
	.byte	0xf
	.4byte	0x163
	.byte	0x9
	.byte	0x4
	.4byte	0x169
	.byte	0xa
	.4byte	0x179
	.byte	0xb
	.4byte	0x41
	.byte	0xb
	.4byte	0x41
	.byte	0
	.byte	0x3
	.4byte	.LASF29
	.byte	0x4
	.byte	0x35
	.byte	0x12
	.4byte	0x185
	.byte	0x9
	.byte	0x4
	.4byte	0x18b
	.byte	0xc
	.4byte	0x41
	.4byte	0x1a9
	.byte	0xb
	.4byte	0x60
	.byte	0xb
	.4byte	0x60
	.byte	0xb
	.4byte	0x1a9
	.byte	0xb
	.4byte	0x1a9
	.byte	0
	.byte	0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF30
	.byte	0x3
	.4byte	.LASF31
	.byte	0x4
	.byte	0x36
	.byte	0x12
	.4byte	0x1bc
	.byte	0x9
	.byte	0x4
	.4byte	0x1c2
	.byte	0xc
	.4byte	0x41
	.4byte	0x1e5
	.byte	0xb
	.4byte	0x60
	.byte	0xb
	.4byte	0x60
	.byte	0xb
	.4byte	0x60
	.byte	0xb
	.4byte	0x1a9
	.byte	0xb
	.4byte	0x1a9
	.byte	0
	.byte	0xd
	.byte	0x18
	.byte	0x4
	.byte	0x40
	.byte	0x9
	.4byte	0x23d
	.byte	0xe
	.4byte	.LASF32
	.byte	0x4
	.byte	0x42
	.byte	0xa
	.4byte	0x41
	.byte	0
	.byte	0xe
	.4byte	.LASF33
	.byte	0x4
	.byte	0x43
	.byte	0x11
	.4byte	0x119
	.byte	0x4
	.byte	0xe
	.4byte	.LASF34
	.byte	0x4
	.byte	0x44
	.byte	0x12
	.4byte	0x136
	.byte	0x8
	.byte	0xe
	.4byte	.LASF35
	.byte	0x4
	.byte	0x45
	.byte	0x13
	.4byte	0x157
	.byte	0xc
	.byte	0xe
	.4byte	.LASF36
	.byte	0x4
	.byte	0x48
	.byte	0x14
	.4byte	0x179
	.byte	0x10
	.byte	0xe
	.4byte	.LASF37
	.byte	0x4
	.byte	0x49
	.byte	0x15
	.4byte	0x1b0
	.byte	0x14
	.byte	0
	.byte	0x3
	.4byte	.LASF38
	.byte	0x4
	.byte	0x4a
	.byte	0x3
	.4byte	0x1e5
	.byte	0xf
	.4byte	0x88
	.4byte	0x259
	.byte	0x10
	.4byte	0x81
	.byte	0xff
	.byte	0
	.byte	0x11
	.4byte	.LASF40
	.byte	0x1
	.byte	0xa
	.byte	0x13
	.4byte	0x23d
	.byte	0x5
	.byte	0x3
	.4byte	s_switch_obj
	.byte	0x12
	.4byte	.LASF39
	.byte	0x1
	.byte	0xb
	.byte	0x6
	.4byte	0x249
	.byte	0x5
	.byte	0x3
	.4byte	respond_msg
	.byte	0x13
	.4byte	.LASF46
	.byte	0x1
	.byte	0xed
	.byte	0xc
	.4byte	0x23d
	.4byte	.LFB9
	.4byte	.LFE9-.LFB9
	.byte	0x1
	.byte	0x9c
	.4byte	0x2b6
	.byte	0x14
	.4byte	.LVL22
	.4byte	0x425
	.byte	0x15
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x78
	.byte	0
	.byte	0x15
	.byte	0x1
	.byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	.LANCHOR0
	.byte	0x15
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x48
	.byte	0
	.byte	0
	.byte	0x16
	.4byte	.LASF52
	.byte	0x1
	.byte	0xdd
	.byte	0x6
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.byte	0x1
	.byte	0x9c
	.4byte	0x2db
	.byte	0x17
	.string	"i"
	.byte	0x1
	.byte	0xe6
	.byte	0xe
	.4byte	0x60
	.4byte	.LLST7
	.byte	0
	.byte	0x18
	.4byte	.LASF53
	.byte	0x1
	.byte	0xa4
	.byte	0xd
	.byte	0x1
	.byte	0x19
	.4byte	.LASF54
	.byte	0x1
	.byte	0x67
	.byte	0x10
	.4byte	0x41
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.byte	0x1
	.byte	0x9c
	.4byte	0x357
	.byte	0x1a
	.4byte	.LASF41
	.byte	0x1
	.byte	0x67
	.byte	0x29
	.4byte	0x60
	.4byte	.LLST1
	.byte	0x1a
	.4byte	.LASF42
	.byte	0x1
	.byte	0x67
	.byte	0x39
	.4byte	0x60
	.4byte	.LLST2
	.byte	0x1a
	.4byte	.LASF43
	.byte	0x1
	.byte	0x67
	.byte	0x45
	.4byte	0x1a9
	.4byte	.LLST3
	.byte	0x1a
	.4byte	.LASF44
	.byte	0x1
	.byte	0x67
	.byte	0x53
	.4byte	0x1a9
	.4byte	.LLST4
	.byte	0x1b
	.4byte	.LASF55
	.byte	0x1
	.byte	0x69
	.byte	0xd
	.4byte	0x41
	.byte	0x1c
	.4byte	.LVL4
	.byte	0x15
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x78
	.byte	0
	.byte	0
	.byte	0
	.byte	0x1d
	.4byte	.LASF56
	.byte	0x1
	.byte	0x38
	.byte	0x10
	.4byte	0x41
	.byte	0x1
	.4byte	0x3a5
	.byte	0x1e
	.4byte	.LASF45
	.byte	0x1
	.byte	0x38
	.byte	0x2a
	.4byte	0x60
	.byte	0x1e
	.4byte	.LASF42
	.byte	0x1
	.byte	0x38
	.byte	0x3b
	.4byte	0x60
	.byte	0x1f
	.string	"uid"
	.byte	0x1
	.byte	0x38
	.byte	0x4c
	.4byte	0x60
	.byte	0x1e
	.4byte	.LASF43
	.byte	0x1
	.byte	0x38
	.byte	0x55
	.4byte	0x1a9
	.byte	0x1e
	.4byte	.LASF44
	.byte	0x1
	.byte	0x38
	.byte	0x63
	.4byte	0x1a9
	.byte	0
	.byte	0x13
	.4byte	.LASF47
	.byte	0x1
	.byte	0xd
	.byte	0x12
	.4byte	0x10d
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.byte	0x1
	.byte	0x9c
	.4byte	0x3ec
	.byte	0x1a
	.4byte	.LASF41
	.byte	0x1
	.byte	0xd
	.byte	0x36
	.4byte	0x41
	.4byte	.LLST5
	.byte	0x20
	.4byte	.LASF48
	.byte	0x1
	.byte	0xf
	.byte	0xd
	.4byte	0x41
	.4byte	.LLST6
	.byte	0x1c
	.4byte	.LVL7
	.byte	0x15
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x78
	.byte	0
	.byte	0
	.byte	0
	.byte	0x21
	.4byte	0x357
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.byte	0x1
	.byte	0x9c
	.4byte	0x425
	.byte	0x22
	.4byte	0x368
	.4byte	.LLST0
	.byte	0x23
	.4byte	0x374
	.byte	0x1
	.byte	0x5b
	.byte	0x23
	.4byte	0x380
	.byte	0x1
	.byte	0x5c
	.byte	0x23
	.4byte	0x38c
	.byte	0x1
	.byte	0x5d
	.byte	0x23
	.4byte	0x398
	.byte	0x1
	.byte	0x5e
	.byte	0
	.byte	0x24
	.4byte	.LASF57
	.4byte	.LASF58
	.byte	0x5
	.byte	0
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
	.byte	0x35
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x5
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
	.byte	0x8
	.byte	0x28
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x1c
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x9
	.byte	0xf
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xa
	.byte	0x15
	.byte	0x1
	.byte	0x27
	.byte	0x19
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xb
	.byte	0x5
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xc
	.byte	0x15
	.byte	0x1
	.byte	0x27
	.byte	0x19
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xd
	.byte	0x13
	.byte	0x1
	.byte	0xb
	.byte	0xb
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
	.byte	0xe
	.byte	0xd
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
	.byte	0x38
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0xf
	.byte	0x1
	.byte	0x1
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x10
	.byte	0x21
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0x2f
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x11
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
	.byte	0x12
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
	.byte	0x13
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
	.byte	0x14
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x15
	.byte	0x8a,0x82,0x1
	.byte	0
	.byte	0x2
	.byte	0x18
	.byte	0x91,0x42
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x16
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
	.byte	0x96,0x42
	.byte	0x19
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x17
	.byte	0x34
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
	.byte	0x18
	.byte	0x2e
	.byte	0
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
	.byte	0x20
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x19
	.byte	0x2e
	.byte	0x1
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
	.byte	0x1a
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
	.byte	0x1b
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
	.byte	0x1c
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x1d
	.byte	0x2e
	.byte	0x1
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
	.byte	0x20
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x1e
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
	.byte	0
	.byte	0
	.byte	0x1f
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
	.byte	0
	.byte	0
	.byte	0x20
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
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x21
	.byte	0x2e
	.byte	0x1
	.byte	0x31
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
	.byte	0x22
	.byte	0x5
	.byte	0
	.byte	0x31
	.byte	0x13
	.byte	0x2
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x23
	.byte	0x5
	.byte	0
	.byte	0x31
	.byte	0x13
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x24
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
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST7:
	.4byte	.LVL16
	.4byte	.LVL17
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL17
	.4byte	.LVL18
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL18
	.4byte	.LVL19
	.2byte	0x3
	.byte	0x78
	.byte	0x7f
	.byte	0x9f
	.4byte	.LVL19
	.4byte	.LVL20
	.2byte	0x1
	.byte	0x58
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LVL2
	.4byte	.LVL3
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL3
	.4byte	.LVL5
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL5
	.4byte	.LFE6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL2
	.4byte	.LVL4-1
	.2byte	0x1
	.byte	0x5b
	.4byte	.LVL4-1
	.4byte	.LFE6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5b
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST3:
	.4byte	.LVL2
	.4byte	.LVL4-1
	.2byte	0x1
	.byte	0x5c
	.4byte	.LVL4-1
	.4byte	.LFE6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5c
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST4:
	.4byte	.LVL2
	.4byte	.LVL4-1
	.2byte	0x1
	.byte	0x5d
	.4byte	.LVL4-1
	.4byte	.LFE6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5d
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST5:
	.4byte	.LVL6
	.4byte	.LVL7-1
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL7-1
	.4byte	.LFE4
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST6:
	.4byte	.LVL8
	.4byte	.LVL9
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL10
	.4byte	.LVL11
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL12
	.4byte	.LVL13
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL14
	.4byte	.LVL15
	.2byte	0x1
	.byte	0x5a
	.4byte	0
	.4byte	0
.LLST0:
	.4byte	.LVL0
	.4byte	.LVL1
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL1
	.4byte	.LFE5
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
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
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
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
	.4byte	.LFB4
	.4byte	.LFE4
	.4byte	.LFB8
	.4byte	.LFE8
	.4byte	.LFB9
	.4byte	.LFE9
	.4byte	0
	.4byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF21:
	.string	"APP_SWITCH_ONE"
.LASF37:
	.string	"_control_switch_cb"
.LASF30:
	.string	"_Bool"
.LASF47:
	.string	"m_control_get_status_switch"
.LASF31:
	.string	"p_control_switch_t"
.LASF5:
	.string	"short unsigned int"
.LASF18:
	.string	"CH4_ON"
.LASF26:
	.string	"p_switch_cfg_t"
.LASF51:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/middle"
.LASF40:
	.string	"s_switch_obj"
.LASF42:
	.string	"act_id"
.LASF43:
	.string	"respond"
.LASF13:
	.string	"CH1_OFF"
.LASF33:
	.string	"_switch_cfg_cb"
.LASF27:
	.string	"p_read_switch_t"
.LASF4:
	.string	"unsigned char"
.LASF35:
	.string	"_write_switch_cb"
.LASF55:
	.string	"pin_value"
.LASF45:
	.string	"action"
.LASF56:
	.string	"m_control_switch"
.LASF24:
	.string	"APP_SWITCH_FOUR"
.LASF38:
	.string	"m_switch_t"
.LASF48:
	.string	"status_pin"
.LASF50:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_switch.c"
.LASF15:
	.string	"CH2_OFF"
.LASF36:
	.string	"_toggle_switch_cb"
.LASF12:
	.string	"CH1_ON"
.LASF34:
	.string	"_read_switch_cb"
.LASF8:
	.string	"long unsigned int"
.LASF10:
	.string	"unsigned int"
.LASF22:
	.string	"APP_SWITCH_TWO"
.LASF9:
	.string	"long long unsigned int"
.LASF6:
	.string	"uint8_t"
.LASF53:
	.string	"control_switch_default_state"
.LASF58:
	.string	"__builtin_memcpy"
.LASF28:
	.string	"p_write_switch_t"
.LASF20:
	.string	"switch_action_t"
.LASF17:
	.string	"CH3_OFF"
.LASF14:
	.string	"CH2_ON"
.LASF54:
	.string	"m_toggle_switch"
.LASF3:
	.string	"long long int"
.LASF49:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
.LASF11:
	.string	"char"
.LASF19:
	.string	"CH4_OFF"
.LASF41:
	.string	"index"
.LASF23:
	.string	"APP_SWITCH_THREE"
.LASF57:
	.string	"memcpy"
.LASF25:
	.string	"m_switch_state_t"
.LASF1:
	.string	"short int"
.LASF46:
	.string	"m_switch_get_instance"
.LASF44:
	.string	"set_even_release"
.LASF7:
	.string	"uint32_t"
.LASF2:
	.string	"long int"
.LASF0:
	.string	"signed char"
.LASF32:
	.string	"max_number_channel"
.LASF16:
	.string	"CH3_ON"
.LASF52:
	.string	"m_switch_init"
.LASF39:
	.string	"respond_msg"
.LASF29:
	.string	"p_toggle_switch_t"
	.ident	"GCC: (SiFive GCC-Metal 10.2.0-2020.12.8) 10.2.0"
