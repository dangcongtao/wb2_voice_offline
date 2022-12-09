	.file	"m_button.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.on_button_event_cb,"ax",@progbits
	.align	1
	.globl	on_button_event_cb
	.type	on_button_event_cb, @function
on_button_event_cb:
.LFB4:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.c"
	.loc 1 25 1
	.cfi_startproc
.LVL0:
	.loc 1 26 5
	.loc 1 28 5
	.loc 1 25 1 is_stmt 0
	addi	sp,sp,-304
	.cfi_def_cfa_offset 304
	sw	s0,296(sp)
	sw	s1,292(sp)
	.cfi_offset 8, -8
	.cfi_offset 9, -12
	mv	s0,a0
	sw	ra,300(sp)
	sw	s2,288(sp)
	.cfi_offset 1, -4
	.cfi_offset 18, -16
	.loc 1 25 1
	mv	s1,a1
	.loc 1 28 9
	call	m_db_get_instance
.LVL1:
	.loc 1 28 71
	addi	a5,s0,188
	slli	a5,a5,2
	add	a0,a0,a5
	.loc 1 28 8
	lw	a4,12(a0)
	li	a5,1
	beq	a4,a5,.L2
.L7:
	.loc 1 32 5 is_stmt 1
	li	s2,2
	beq	s1,s2,.L3
	li	a5,3
	beq	s1,a5,.L4
	li	a5,1
	bne	s1,a5,.L1
	.loc 1 41 9
	.loc 1 41 33 is_stmt 0
	lui	a4,%hi(.LANCHOR0)
	addi	a4,a4,%lo(.LANCHOR0)
	slli	a5,s0,2
	add	a5,a4,a5
	.loc 1 41 12
	lw	a4,0(a5)
	beq	a4,zero,.L8
	.loc 1 43 13 is_stmt 1
	.loc 1 43 46 is_stmt 0
	sw	zero,0(a5)
	.loc 1 44 13 is_stmt 1
	j	.L1
.L2:
	.loc 1 29 9
	.loc 1 29 13 is_stmt 0
	addi	a0,sp,120
	call	m_wifi_get_instance
.LVL2:
	lw	a5,152(sp)
	jalr	a5
.LVL3:
	.loc 1 29 12
	li	a5,10
	bne	a0,a5,.L7
.LVL4:
.L1:
	.loc 1 88 1
	lw	ra,300(sp)
	.cfi_remember_state
	.cfi_restore 1
	lw	s0,296(sp)
	.cfi_restore 8
	lw	s1,292(sp)
	.cfi_restore 9
.LVL5:
	lw	s2,288(sp)
	.cfi_restore 18
	addi	sp,sp,304
	.cfi_def_cfa_offset 0
	jr	ra
.LVL6:
.L8:
	.cfi_restore_state
	.loc 1 47 9 is_stmt 1
	addi	a0,sp,8
	call	m_switch_get_instance
.LVL7:
	mv	a0,s0
	.loc 1 88 1 is_stmt 0
	lw	s0,296(sp)
	.cfi_remember_state
	.cfi_restore 8
.LVL8:
	.loc 1 47 9
	lw	a5,24(sp)
	.loc 1 88 1
	lw	ra,300(sp)
	.cfi_restore 1
	lw	s1,292(sp)
	.cfi_restore 9
.LVL9:
	lw	s2,288(sp)
	.cfi_restore 18
	.loc 1 47 9
	li	a3,1
	.loc 1 88 1
	.loc 1 47 9
	li	a2,1
	li	a1,1
	.loc 1 88 1
	addi	sp,sp,304
	.cfi_def_cfa_offset 0
	.loc 1 47 9
	jr	a5
.LVL10:
.L3:
	.cfi_restore_state
	.loc 1 54 9 is_stmt 1
	addi	a0,sp,32
	call	m_led_stt_get_instance
.LVL11:
	lw	a5,64(sp)
	jalr	a5
.LVL12:
	.loc 1 55 9
	addi	a0,sp,176
	call	m_wifi_get_instance
.LVL13:
	lw	a5,196(sp)
	jalr	a5
.LVL14:
	.loc 1 56 9
	call	m_db_get_instance
.LVL15:
	lw	a5,856(a0)
	jalr	a5
.LVL16:
	.loc 1 57 9
	.loc 1 57 42 is_stmt 0
	slli	a0,s0,2
	lui	s0,%hi(.LANCHOR0)
.LVL17:
	addi	s0,s0,%lo(.LANCHOR0)
	add	s0,s0,a0
	li	a5,1
	sw	a5,0(s0)
	.loc 1 59 5 is_stmt 1
	j	.L1
.LVL18:
.L4:
	.loc 1 63 9
	.loc 1 63 33 is_stmt 0
	lui	a0,%hi(.LANCHOR0)
	slli	s0,s0,2
.LVL19:
	addi	a0,a0,%lo(.LANCHOR0)
	add	s0,a0,s0
	.loc 1 63 12
	lw	a5,0(s0)
	beq	a5,s2,.L1
	.loc 1 67 9 is_stmt 1
	addi	a0,sp,76
	call	m_led_stt_get_instance
.LVL20:
	lw	a5,104(sp)
	jalr	a5
.LVL21:
	.loc 1 68 9
	addi	a0,sp,232
	call	m_wifi_get_instance
.LVL22:
	lw	a5,256(sp)
	jalr	a5
.LVL23:
	.loc 1 69 9
	.loc 1 69 42 is_stmt 0
	sw	s2,0(s0)
	.loc 1 71 5 is_stmt 1
	j	.L1
	.cfi_endproc
.LFE4:
	.size	on_button_event_cb, .-on_button_event_cb
	.section	.text.button_scan_task,"ax",@progbits
	.align	1
	.type	button_scan_task, @function
button_scan_task:
.LFB6:
	.loc 1 116 1
	.cfi_startproc
.LVL24:
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	ra,12(sp)
	.cfi_offset 1, -4
.LVL25:
.L12:
	.loc 1 117 5 discriminator 1
	.loc 1 119 9 discriminator 1
	call	m_switch_press_release_loop
.LVL26:
	.loc 1 120 9 discriminator 1
	call	m_switch_cyclic_timer_loop
.LVL27:
	.loc 1 121 9 discriminator 1
	li	a0,0
	call	app_btn_scan
.LVL28:
	.loc 1 123 9 discriminator 1
	li	a0,50
	call	vTaskDelay
.LVL29:
	.loc 1 117 11 discriminator 1
	j	.L12
	.cfi_endproc
.LFE6:
	.size	button_scan_task, .-button_scan_task
	.section	.rodata.m_button_init.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.string	"led_button_task"
	.section	.text.m_button_init,"ax",@progbits
	.align	1
	.globl	m_button_init
	.type	m_button_init, @function
m_button_init:
.LFB7:
	.loc 1 129 1
	.cfi_startproc
	.loc 1 130 5
.LBB5:
.LBB6:
	.loc 1 92 5
	.loc 1 93 5
.LBE6:
.LBE5:
	.loc 1 93 10
.LVL30:
	.loc 1 93 26
.LBB11:
.LBB7:
	.loc 1 100 5
	.loc 1 101 5
	.loc 1 101 17 is_stmt 0
	lui	a5,%hi(.LANCHOR1)
.LBE7:
.LBE11:
	.loc 1 129 1
	addi	sp,sp,-48
	.cfi_def_cfa_offset 48
.LBB12:
.LBB8:
	.loc 1 101 17
	addi	a5,a5,%lo(.LANCHOR1)
	sw	a5,12(sp)
	.loc 1 102 5 is_stmt 1
	.loc 1 102 20 is_stmt 0
	lui	a5,%hi(max_number_channel)
	lbu	a5,%lo(max_number_channel)(a5)
	.loc 1 107 5
	addi	a0,sp,12
.LBE8:
.LBE12:
	.loc 1 129 1
	sw	ra,44(sp)
.LBB13:
.LBB9:
	.loc 1 102 20
	sb	a5,16(sp)
	.loc 1 103 5 is_stmt 1
	.loc 1 103 22 is_stmt 0
	lui	a5,%hi(hw_get_tick_ms)
	addi	a5,a5,%lo(hw_get_tick_ms)
	sw	a5,20(sp)
	.loc 1 104 5 is_stmt 1
	.loc 1 104 25 is_stmt 0
	lui	a5,%hi(hw_button_config)
	addi	a5,a5,%lo(hw_button_config)
	sw	a5,24(sp)
	.loc 1 105 5 is_stmt 1
	.loc 1 105 19 is_stmt 0
	lui	a5,%hi(hw_btn_read)
	addi	a5,a5,%lo(hw_btn_read)
	sw	a5,28(sp)
	.loc 1 107 5 is_stmt 1
.LBE9:
.LBE13:
	.loc 1 129 1 is_stmt 0
	sw	s0,40(sp)
	.cfi_offset 1, -4
	.cfi_offset 8, -8
.LBB14:
.LBB10:
	.loc 1 107 5
	call	app_btn_initialize
.LVL31:
	.loc 1 108 5 is_stmt 1
	lui	s0,%hi(on_button_event_cb)
	addi	a1,s0,%lo(on_button_event_cb)
	li	a2,0
	li	a0,0
	call	app_btn_register_callback
.LVL32:
	.loc 1 109 5
	addi	a1,s0,%lo(on_button_event_cb)
	li	a2,0
	li	a0,1
	call	app_btn_register_callback
.LVL33:
	.loc 1 110 5
	addi	a1,s0,%lo(on_button_event_cb)
	li	a2,0
	li	a0,2
	call	app_btn_register_callback
.LVL34:
	.loc 1 111 5
	addi	a1,s0,%lo(on_button_event_cb)
	li	a2,0
	li	a0,3
	call	app_btn_register_callback
.LVL35:
.LBE10:
.LBE14:
	.loc 1 132 5
	.loc 1 138 1 is_stmt 0
	lw	s0,40(sp)
	.cfi_restore 8
	lw	ra,44(sp)
	.cfi_restore 1
	.loc 1 132 5
	li	a2,4096
	lui	a1,%hi(.LC0)
	lui	a0,%hi(button_scan_task)
	.loc 1 138 1
	.loc 1 132 5
	li	a5,0
	li	a4,4
	li	a3,0
	addi	a2,a2,-1024
	addi	a1,a1,%lo(.LC0)
	addi	a0,a0,%lo(button_scan_task)
	.loc 1 138 1
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	.loc 1 132 5
	tail	xTaskCreate
.LVL36:
	.cfi_endproc
.LFE7:
	.size	m_button_init, .-m_button_init
	.section	.text.m_button_get_instace,"ax",@progbits
	.align	1
	.globl	m_button_get_instace
	.type	m_button_get_instace, @function
m_button_get_instace:
.LFB8:
	.loc 1 141 1 is_stmt 1
	.cfi_startproc
.LVL37:
	.loc 1 142 5
	.loc 1 142 12 is_stmt 0
	lui	a1,%hi(.LANCHOR2)
	.loc 1 141 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	.loc 1 142 12
	li	a2,24
	addi	a1,a1,%lo(.LANCHOR2)
	.loc 1 141 1
	sw	s0,8(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	.loc 1 141 1
	mv	s0,a0
	.loc 1 142 12
	call	memcpy
.LVL38:
	.loc 1 143 1
	lw	ra,12(sp)
	.cfi_restore 1
	mv	a0,s0
	lw	s0,8(sp)
	.cfi_restore 8
.LVL39:
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE8:
	.size	m_button_get_instace, .-m_button_get_instace
	.globl	ignore_event_release
	.section	.bss.ignore_event_release,"aw",@nobits
	.align	2
	.set	.LANCHOR0,. + 0
	.type	ignore_event_release, @object
	.size	ignore_event_release, 16
ignore_event_release:
	.zero	16
	.section	.bss.m_hw_button_initialize_value.0,"aw",@nobits
	.align	2
	.set	.LANCHOR1,. + 0
	.type	m_hw_button_initialize_value.0, @object
	.size	m_hw_button_initialize_value.0, 48
m_hw_button_initialize_value.0:
	.zero	48
	.section	.rodata.s_button_obj,"a"
	.align	2
	.set	.LANCHOR2,. + 0
	.type	s_button_obj, @object
	.size	s_button_obj, 24
s_button_obj:
	.zero	24
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint-gcc.h"
	.file 3 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.h"
	.file 4 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/../../third_party/huytv_button/app_btn.h"
	.file 5 "/home/dungnt98/Ai-Thinker-WB2/components/platform/soc/bl602/freertos_riscv_ram/portable/GCC/RISC-V/portmacro.h"
	.file 6 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/wifi/m_wifi.h"
	.file 7 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.h"
	.file 8 "/home/dungnt98/Ai-Thinker-WB2/components/platform/soc/bl602/freertos_riscv_ram/config/task.h"
	.file 9 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_switch.h"
	.file 10 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/database/m_database.h"
	.file 11 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_led_stt.h"
	.file 12 "<built-in>"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x6c3
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.byte	0x1
	.4byte	.LASF80
	.byte	0xc
	.4byte	.LASF81
	.4byte	.LASF82
	.4byte	.Ldebug_ranges0+0x30
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
	.byte	0x3
	.4byte	.LASF11
	.byte	0x3
	.byte	0x5
	.byte	0x10
	.4byte	0x94
	.byte	0x6
	.byte	0x4
	.4byte	0x9a
	.byte	0x7
	.4byte	0xa5
	.byte	0x8
	.4byte	0x60
	.byte	0
	.byte	0x3
	.4byte	.LASF12
	.byte	0x3
	.byte	0x6
	.byte	0x14
	.4byte	0xb1
	.byte	0x6
	.byte	0x4
	.4byte	0xb7
	.byte	0x9
	.4byte	0x60
	.4byte	0xc6
	.byte	0x8
	.4byte	0x60
	.byte	0
	.byte	0x3
	.4byte	.LASF13
	.byte	0x3
	.byte	0x7
	.byte	0x14
	.4byte	0xd2
	.byte	0x6
	.byte	0x4
	.4byte	0xd8
	.byte	0xa
	.4byte	0x60
	.byte	0x3
	.4byte	.LASF14
	.byte	0x3
	.byte	0x8
	.byte	0x10
	.4byte	0xe9
	.byte	0x6
	.byte	0x4
	.4byte	0xef
	.byte	0xb
	.byte	0xc
	.byte	0x18
	.byte	0x3
	.byte	0xa
	.byte	0x9
	.4byte	0x148
	.byte	0xd
	.4byte	.LASF15
	.byte	0x3
	.byte	0xc
	.byte	0xe
	.4byte	0x60
	.byte	0
	.byte	0xd
	.4byte	.LASF16
	.byte	0x3
	.byte	0xd
	.byte	0xe
	.4byte	0x60
	.byte	0x4
	.byte	0xd
	.4byte	.LASF17
	.byte	0x3
	.byte	0x10
	.byte	0x14
	.4byte	0x88
	.byte	0x8
	.byte	0xd
	.4byte	.LASF18
	.byte	0x3
	.byte	0x11
	.byte	0x12
	.4byte	0xa5
	.byte	0xc
	.byte	0xd
	.4byte	.LASF19
	.byte	0x3
	.byte	0x12
	.byte	0x18
	.4byte	0xdd
	.byte	0x10
	.byte	0xd
	.4byte	.LASF20
	.byte	0x3
	.byte	0x13
	.byte	0x15
	.4byte	0xc6
	.byte	0x14
	.byte	0
	.byte	0x3
	.4byte	.LASF21
	.byte	0x3
	.byte	0x14
	.byte	0x3
	.4byte	0xf0
	.byte	0xe
	.byte	0x7
	.byte	0x4
	.4byte	0x81
	.byte	0x4
	.byte	0x1d
	.byte	0x1
	.4byte	0x199
	.byte	0xf
	.4byte	.LASF22
	.byte	0
	.byte	0xf
	.4byte	.LASF23
	.byte	0x1
	.byte	0xf
	.4byte	.LASF24
	.byte	0x2
	.byte	0xf
	.4byte	.LASF25
	.byte	0x3
	.byte	0xf
	.4byte	.LASF26
	.byte	0x4
	.byte	0xf
	.4byte	.LASF27
	.byte	0x5
	.byte	0xf
	.4byte	.LASF28
	.byte	0x6
	.byte	0xf
	.4byte	.LASF29
	.byte	0x7
	.byte	0xf
	.4byte	.LASF30
	.byte	0x8
	.byte	0
	.byte	0xc
	.byte	0xc
	.byte	0x4
	.byte	0x29
	.byte	0x9
	.4byte	0x1ca
	.byte	0x10
	.string	"pin"
	.byte	0x4
	.byte	0x2b
	.byte	0xe
	.4byte	0x60
	.byte	0
	.byte	0xd
	.4byte	.LASF31
	.byte	0x4
	.byte	0x2c
	.byte	0xd
	.4byte	0x41
	.byte	0x4
	.byte	0xd
	.4byte	.LASF32
	.byte	0x4
	.byte	0x2d
	.byte	0xe
	.4byte	0x60
	.byte	0x8
	.byte	0
	.byte	0x3
	.4byte	.LASF33
	.byte	0x4
	.byte	0x2e
	.byte	0x3
	.4byte	0x199
	.byte	0x3
	.4byte	.LASF34
	.byte	0x4
	.byte	0x30
	.byte	0x14
	.4byte	0xd2
	.byte	0x3
	.4byte	.LASF35
	.byte	0x4
	.byte	0x31
	.byte	0x10
	.4byte	0x94
	.byte	0x3
	.4byte	.LASF36
	.byte	0x4
	.byte	0x32
	.byte	0x14
	.4byte	0xb1
	.byte	0xc
	.byte	0x14
	.byte	0x4
	.byte	0x34
	.byte	0x9
	.4byte	0x245
	.byte	0xd
	.4byte	.LASF37
	.byte	0x4
	.byte	0x36
	.byte	0x1b
	.4byte	0x245
	.byte	0
	.byte	0xd
	.4byte	.LASF38
	.byte	0x4
	.byte	0x37
	.byte	0xd
	.4byte	0x41
	.byte	0x4
	.byte	0xd
	.4byte	.LASF39
	.byte	0x4
	.byte	0x38
	.byte	0x19
	.4byte	0x1d6
	.byte	0x8
	.byte	0xd
	.4byte	.LASF40
	.byte	0x4
	.byte	0x39
	.byte	0x1b
	.4byte	0x1e2
	.byte	0xc
	.byte	0xd
	.4byte	.LASF41
	.byte	0x4
	.byte	0x3a
	.byte	0x1a
	.4byte	0x1ee
	.byte	0x10
	.byte	0
	.byte	0x6
	.byte	0x4
	.4byte	0x1ca
	.byte	0x3
	.4byte	.LASF42
	.byte	0x4
	.byte	0x3b
	.byte	0x3
	.4byte	0x1fa
	.byte	0x11
	.byte	0x4
	.byte	0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF43
	.byte	0x12
	.4byte	0x259
	.byte	0x6
	.byte	0x4
	.4byte	0x260
	.byte	0x13
	.4byte	.LASF57
	.byte	0x7
	.byte	0x20
	.byte	0x19
	.4byte	0x4d
	.byte	0x3
	.4byte	.LASF44
	.byte	0x5
	.byte	0x41
	.byte	0x12
	.4byte	0x60
	.byte	0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF45
	.byte	0x14
	.4byte	0x60
	.4byte	0x29a
	.byte	0x15
	.4byte	0x81
	.byte	0x3
	.byte	0
	.byte	0xe
	.byte	0x7
	.byte	0x4
	.4byte	0x81
	.byte	0x6
	.byte	0xa
	.byte	0xe
	.4byte	0x2eb
	.byte	0xf
	.4byte	.LASF46
	.byte	0
	.byte	0xf
	.4byte	.LASF47
	.byte	0x1
	.byte	0xf
	.4byte	.LASF48
	.byte	0x2
	.byte	0xf
	.4byte	.LASF49
	.byte	0x3
	.byte	0xf
	.4byte	.LASF50
	.byte	0x4
	.byte	0xf
	.4byte	.LASF51
	.byte	0x5
	.byte	0xf
	.4byte	.LASF52
	.byte	0x6
	.byte	0xf
	.4byte	.LASF53
	.byte	0x7
	.byte	0xf
	.4byte	.LASF54
	.byte	0x8
	.byte	0xf
	.4byte	.LASF55
	.byte	0x9
	.byte	0xf
	.4byte	.LASF56
	.byte	0xa
	.byte	0
	.byte	0x16
	.4byte	.LASF59
	.byte	0x1
	.byte	0x11
	.byte	0x13
	.4byte	0x148
	.byte	0x5
	.byte	0x3
	.4byte	s_button_obj
	.byte	0x17
	.4byte	.LASF58
	.byte	0x1
	.byte	0x12
	.byte	0xa
	.4byte	0x28a
	.byte	0x5
	.byte	0x3
	.4byte	ignore_event_release
	.byte	0x14
	.4byte	0x265
	.4byte	0x31f
	.byte	0x15
	.4byte	0x81
	.byte	0x8
	.byte	0
	.byte	0x18
	.4byte	.LASF61
	.byte	0x1
	.byte	0x14
	.byte	0x14
	.4byte	0x30f
	.byte	0x19
	.4byte	.LASF83
	.byte	0x1
	.byte	0x8c
	.byte	0xc
	.4byte	0x148
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.byte	0x1
	.byte	0x9c
	.4byte	0x364
	.byte	0x1a
	.4byte	.LVL38
	.4byte	0x635
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x78
	.byte	0
	.byte	0x1b
	.byte	0x1
	.byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	.LANCHOR2
	.byte	0x1b
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x48
	.byte	0
	.byte	0
	.byte	0x1c
	.4byte	.LASF63
	.byte	0x1
	.byte	0x80
	.byte	0x6
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.byte	0x1
	.byte	0x9c
	.4byte	0x464
	.byte	0x1d
	.4byte	0x4c0
	.4byte	.LBB5
	.4byte	.Ldebug_ranges0+0
	.byte	0x1
	.byte	0x82
	.byte	0x5
	.4byte	0x431
	.byte	0x1e
	.4byte	.Ldebug_ranges0+0
	.byte	0x1f
	.4byte	0x4df
	.byte	0x2
	.byte	0x91
	.byte	0x5c
	.byte	0x20
	.4byte	.LVL31
	.4byte	0x640
	.4byte	0x3af
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x91
	.byte	0x5c
	.byte	0
	.byte	0x20
	.4byte	.LVL32
	.4byte	0x64c
	.4byte	0x3d0
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x30
	.byte	0x1b
	.byte	0x1
	.byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	on_button_event_cb
	.byte	0x1b
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0x20
	.4byte	.LVL33
	.4byte	0x64c
	.4byte	0x3f1
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x31
	.byte	0x1b
	.byte	0x1
	.byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	on_button_event_cb
	.byte	0x1b
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0x20
	.4byte	.LVL34
	.4byte	0x64c
	.4byte	0x412
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x32
	.byte	0x1b
	.byte	0x1
	.byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	on_button_event_cb
	.byte	0x1b
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0x1a
	.4byte	.LVL35
	.4byte	0x64c
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x33
	.byte	0x1b
	.byte	0x1
	.byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	on_button_event_cb
	.byte	0x1b
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0
	.byte	0
	.byte	0x21
	.4byte	.LVL36
	.4byte	0x658
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x5
	.byte	0x3
	.4byte	button_scan_task
	.byte	0x1b
	.byte	0x1
	.byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	.LC0
	.byte	0x1b
	.byte	0x1
	.byte	0x5c
	.byte	0x3
	.byte	0xa
	.2byte	0xc00
	.byte	0x1b
	.byte	0x1
	.byte	0x5d
	.byte	0x1
	.byte	0x30
	.byte	0x1b
	.byte	0x1
	.byte	0x5e
	.byte	0x1
	.byte	0x34
	.byte	0x1b
	.byte	0x1
	.byte	0x5f
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0
	.byte	0x22
	.4byte	.LASF84
	.byte	0x1
	.byte	0x73
	.byte	0xd
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.byte	0x1
	.byte	0x9c
	.4byte	0x4c0
	.byte	0x23
	.4byte	.LASF65
	.byte	0x1
	.byte	0x73
	.byte	0x24
	.4byte	0x257
	.4byte	.LLST3
	.byte	0x24
	.4byte	.LVL26
	.4byte	0x665
	.byte	0x24
	.4byte	.LVL27
	.4byte	0x671
	.byte	0x20
	.4byte	.LVL28
	.4byte	0x67d
	.4byte	0x4af
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0x1a
	.4byte	.LVL29
	.4byte	0x689
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x8
	.byte	0x32
	.byte	0
	.byte	0
	.byte	0x25
	.4byte	.LASF85
	.byte	0x1
	.byte	0x5a
	.byte	0xd
	.byte	0x1
	.4byte	0x4f8
	.byte	0x16
	.4byte	.LASF60
	.byte	0x1
	.byte	0x5c
	.byte	0x20
	.4byte	0x4f8
	.byte	0x5
	.byte	0x3
	.4byte	m_hw_button_initialize_value.0
	.byte	0x18
	.4byte	.LASF62
	.byte	0x1
	.byte	0x64
	.byte	0x16
	.4byte	0x24b
	.byte	0x26
	.byte	0x27
	.string	"i"
	.byte	0x1
	.byte	0x5d
	.byte	0x13
	.4byte	0x60
	.byte	0
	.byte	0
	.byte	0x14
	.4byte	0x1ca
	.4byte	0x508
	.byte	0x15
	.4byte	0x81
	.byte	0x3
	.byte	0
	.byte	0x28
	.4byte	.LASF64
	.byte	0x1
	.byte	0x18
	.byte	0x6
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.byte	0x1
	.byte	0x9c
	.4byte	0x635
	.byte	0x23
	.4byte	.LASF66
	.byte	0x1
	.byte	0x18
	.byte	0x1d
	.4byte	0x7a
	.4byte	.LLST0
	.byte	0x23
	.4byte	.LASF67
	.byte	0x1
	.byte	0x18
	.byte	0x2d
	.4byte	0x7a
	.4byte	.LLST1
	.byte	0x23
	.4byte	.LASF68
	.byte	0x1
	.byte	0x18
	.byte	0x3a
	.4byte	0x257
	.4byte	.LLST2
	.byte	0x24
	.4byte	.LVL1
	.4byte	0x696
	.byte	0x20
	.4byte	.LVL2
	.4byte	0x6a2
	.4byte	0x56c
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xc8,0x7e
	.byte	0
	.byte	0x29
	.4byte	.LVL3
	.byte	0x4
	.byte	0x91
	.byte	0xe8,0x7e
	.byte	0x6
	.byte	0x20
	.4byte	.LVL7
	.4byte	0x6ae
	.4byte	0x58b
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xd8,0x7d
	.byte	0
	.byte	0x2a
	.4byte	.LVL10
	.byte	0x4
	.byte	0x72
	.byte	0xe8,0x7d
	.byte	0x6
	.4byte	0x5b0
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x1b
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x31
	.byte	0x1b
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x31
	.byte	0x1b
	.byte	0x1
	.byte	0x5d
	.byte	0x1
	.byte	0x31
	.byte	0
	.byte	0x20
	.4byte	.LVL11
	.4byte	0x6ba
	.4byte	0x5c5
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xf0,0x7d
	.byte	0
	.byte	0x29
	.4byte	.LVL12
	.byte	0x4
	.byte	0x91
	.byte	0x90,0x7e
	.byte	0x6
	.byte	0x20
	.4byte	.LVL13
	.4byte	0x6a2
	.4byte	0x5e4
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0x80,0x7f
	.byte	0
	.byte	0x29
	.4byte	.LVL14
	.byte	0x4
	.byte	0x91
	.byte	0x94,0x7f
	.byte	0x6
	.byte	0x24
	.4byte	.LVL15
	.4byte	0x696
	.byte	0x20
	.4byte	.LVL20
	.4byte	0x6ba
	.4byte	0x60c
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0x9c,0x7e
	.byte	0
	.byte	0x29
	.4byte	.LVL21
	.byte	0x4
	.byte	0x91
	.byte	0xb8,0x7e
	.byte	0x6
	.byte	0x20
	.4byte	.LVL22
	.4byte	0x6a2
	.4byte	0x62b
	.byte	0x1b
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xb8,0x7f
	.byte	0
	.byte	0x29
	.4byte	.LVL23
	.byte	0x3
	.byte	0x91
	.byte	0x50
	.byte	0x6
	.byte	0
	.byte	0x2b
	.4byte	.LASF86
	.4byte	.LASF87
	.byte	0xc
	.byte	0
	.byte	0x2c
	.4byte	.LASF69
	.4byte	.LASF69
	.byte	0x4
	.byte	0x45
	.byte	0x6
	.byte	0x2c
	.4byte	.LASF70
	.4byte	.LASF70
	.byte	0x4
	.byte	0x53
	.byte	0x6
	.byte	0x2d
	.4byte	.LASF71
	.4byte	.LASF71
	.byte	0x8
	.2byte	0x14a
	.byte	0xd
	.byte	0x2c
	.4byte	.LASF72
	.4byte	.LASF72
	.byte	0x9
	.byte	0x63
	.byte	0x6
	.byte	0x2c
	.4byte	.LASF73
	.4byte	.LASF73
	.byte	0x9
	.byte	0x62
	.byte	0x6
	.byte	0x2c
	.4byte	.LASF74
	.4byte	.LASF74
	.byte	0x4
	.byte	0x4b
	.byte	0x6
	.byte	0x2d
	.4byte	.LASF75
	.4byte	.LASF75
	.byte	0x8
	.2byte	0x2f6
	.byte	0x6
	.byte	0x2c
	.4byte	.LASF76
	.4byte	.LASF76
	.byte	0xa
	.byte	0x4d
	.byte	0xf
	.byte	0x2c
	.4byte	.LASF77
	.4byte	.LASF77
	.byte	0x6
	.byte	0x45
	.byte	0xa
	.byte	0x2c
	.4byte	.LASF78
	.4byte	.LASF78
	.byte	0x9
	.byte	0x4e
	.byte	0xc
	.byte	0x2c
	.4byte	.LASF79
	.4byte	.LASF79
	.byte	0xb
	.byte	0x36
	.byte	0xd
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
	.byte	0xf
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x7
	.byte	0x15
	.byte	0x1
	.byte	0x27
	.byte	0x19
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x8
	.byte	0x5
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x9
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
	.byte	0xa
	.byte	0x15
	.byte	0
	.byte	0x27
	.byte	0x19
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0xb
	.byte	0x15
	.byte	0
	.byte	0x27
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0xc
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
	.byte	0xd
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
	.byte	0xe
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
	.byte	0xf
	.byte	0x28
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x1c
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x10
	.byte	0xd
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
	.byte	0x38
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x11
	.byte	0xf
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x12
	.byte	0x26
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x13
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
	.byte	0x14
	.byte	0x1
	.byte	0x1
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x15
	.byte	0x21
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0x2f
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x16
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
	.byte	0x17
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
	.byte	0x18
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
	.byte	0x19
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
	.byte	0x1a
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x1b
	.byte	0x8a,0x82,0x1
	.byte	0
	.byte	0x2
	.byte	0x18
	.byte	0x91,0x42
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x1c
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
	.byte	0x1d
	.byte	0x1d
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0x52
	.byte	0x1
	.byte	0x55
	.byte	0x17
	.byte	0x58
	.byte	0xb
	.byte	0x59
	.byte	0xb
	.byte	0x57
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x1e
	.byte	0xb
	.byte	0x1
	.byte	0x55
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x1f
	.byte	0x34
	.byte	0
	.byte	0x31
	.byte	0x13
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x20
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
	.byte	0x21
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
	.byte	0x22
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
	.byte	0x23
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
	.byte	0x24
	.byte	0x89,0x82,0x1
	.byte	0
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x25
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
	.byte	0x20
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x26
	.byte	0xb
	.byte	0x1
	.byte	0
	.byte	0
	.byte	0x27
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
	.byte	0
	.byte	0
	.byte	0x28
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
	.byte	0x29
	.byte	0x89,0x82,0x1
	.byte	0
	.byte	0x11
	.byte	0x1
	.byte	0x93,0x42
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x2a
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x95,0x42
	.byte	0x19
	.byte	0x93,0x42
	.byte	0x18
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x2b
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
	.byte	0x2c
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
	.byte	0x2d
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
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST3:
	.4byte	.LVL24
	.4byte	.LVL25
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL25
	.4byte	.LFE6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST0:
	.4byte	.LVL0
	.4byte	.LVL1-1
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL1-1
	.4byte	.LVL4
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL4
	.4byte	.LVL6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL6
	.4byte	.LVL8
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL8
	.4byte	.LVL10-1
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL10-1
	.4byte	.LVL10
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL10
	.4byte	.LVL17
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL17
	.4byte	.LVL18
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL18
	.4byte	.LVL19
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL19
	.4byte	.LFE4
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LVL0
	.4byte	.LVL1-1
	.2byte	0x1
	.byte	0x5b
	.4byte	.LVL1-1
	.4byte	.LVL5
	.2byte	0x1
	.byte	0x59
	.4byte	.LVL5
	.4byte	.LVL6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5b
	.byte	0x9f
	.4byte	.LVL6
	.4byte	.LVL9
	.2byte	0x1
	.byte	0x59
	.4byte	.LVL9
	.4byte	.LVL10
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5b
	.byte	0x9f
	.4byte	.LVL10
	.4byte	.LFE4
	.2byte	0x1
	.byte	0x59
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL0
	.4byte	.LVL1-1
	.2byte	0x1
	.byte	0x5c
	.4byte	.LVL1-1
	.4byte	.LFE4
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5c
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
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.4byte	.LBB5
	.4byte	.LBE5
	.4byte	.LBB11
	.4byte	.LBE11
	.4byte	.LBB12
	.4byte	.LBE12
	.4byte	.LBB13
	.4byte	.LBE13
	.4byte	.LBB14
	.4byte	.LBE14
	.4byte	0
	.4byte	0
	.4byte	.LFB4
	.4byte	.LFE4
	.4byte	.LFB6
	.4byte	.LFE6
	.4byte	.LFB7
	.4byte	.LFE7
	.4byte	.LFB8
	.4byte	.LFE8
	.4byte	0
	.4byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF14:
	.string	"p_btn_timer_init_t"
.LASF23:
	.string	"APP_BTN_EVT_RELEASED"
.LASF87:
	.string	"__builtin_memcpy"
.LASF74:
	.string	"app_btn_scan"
.LASF32:
	.string	"idle_level"
.LASF50:
	.string	"WIFI_WAIT_SMARTCONFIG_DONE"
.LASF39:
	.string	"get_tick_cb"
.LASF55:
	.string	"WIFI_WAIT_EVENT"
.LASF16:
	.string	"_btn_idle_level"
.LASF57:
	.string	"max_number_channel"
.LASF1:
	.string	"short int"
.LASF28:
	.string	"APP_BTN_EVT_ILDE"
.LASF75:
	.string	"vTaskDelay"
.LASF35:
	.string	"app_btn_initialize_cb"
.LASF86:
	.string	"memcpy"
.LASF72:
	.string	"m_switch_press_release_loop"
.LASF67:
	.string	"event"
.LASF69:
	.string	"app_btn_initialize"
.LASF13:
	.string	"p_get_tick_ms_t"
.LASF51:
	.string	"WIFI_APCONFIG"
.LASF37:
	.string	"config"
.LASF30:
	.string	"APP_BTN_EVT_MAX"
.LASF6:
	.string	"uint8_t"
.LASF62:
	.string	"conf"
.LASF11:
	.string	"p_btn_config_t"
.LASF25:
	.string	"APP_BTN_EVT_HOLD_SO_LONG"
.LASF53:
	.string	"WIFI_APCONFIG_DONE"
.LASF81:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_button.c"
.LASF36:
	.string	"app_btn_get_level_cb"
.LASF38:
	.string	"btn_count"
.LASF3:
	.string	"long long int"
.LASF44:
	.string	"TickType_t"
.LASF78:
	.string	"m_switch_get_instance"
.LASF2:
	.string	"long int"
.LASF19:
	.string	"_button_timer_init_cb"
.LASF26:
	.string	"APP_BTN_EVT_DOUBLE_CLICK"
.LASF22:
	.string	"APP_BTN_EVT_PRESSED"
.LASF47:
	.string	"WIFI_DISCONNECT"
.LASF79:
	.string	"m_led_stt_get_instance"
.LASF31:
	.string	"last_state"
.LASF84:
	.string	"button_scan_task"
.LASF54:
	.string	"WIFI_CONFIG_DONE"
.LASF4:
	.string	"unsigned char"
.LASF85:
	.string	"m_button_config"
.LASF41:
	.string	"btn_read"
.LASF73:
	.string	"m_switch_cyclic_timer_loop"
.LASF34:
	.string	"app_btn_get_tick_cb"
.LASF60:
	.string	"m_hw_button_initialize_value"
.LASF52:
	.string	"WIFI_WAIT_APCONFIG_DONE"
.LASF77:
	.string	"m_wifi_get_instance"
.LASF9:
	.string	"long long unsigned int"
.LASF7:
	.string	"uint32_t"
.LASF66:
	.string	"button_pin"
.LASF10:
	.string	"unsigned int"
.LASF42:
	.string	"app_btn_config_t"
.LASF59:
	.string	"s_button_obj"
.LASF65:
	.string	"param"
.LASF40:
	.string	"btn_initialize"
.LASF5:
	.string	"short unsigned int"
.LASF33:
	.string	"app_btn_hw_config_t"
.LASF20:
	.string	"_button_get_tick_ms_cb"
.LASF24:
	.string	"APP_BTN_EVT_HOLD"
.LASF29:
	.string	"APP_BTN_EVT_ILDE_BREAK"
.LASF43:
	.string	"char"
.LASF58:
	.string	"ignore_event_release"
.LASF15:
	.string	"_btn_max_number_channel"
.LASF17:
	.string	"_button_conf_cb"
.LASF45:
	.string	"_Bool"
.LASF71:
	.string	"xTaskCreate"
.LASF68:
	.string	"data"
.LASF49:
	.string	"WIFI_SMARTCONFIG"
.LASF76:
	.string	"m_db_get_instance"
.LASF61:
	.string	"event_name"
.LASF8:
	.string	"long unsigned int"
.LASF82:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/middle"
.LASF56:
	.string	"WIFI_DEV_ACTIVE"
.LASF63:
	.string	"m_button_init"
.LASF18:
	.string	"_button_read_cb"
.LASF70:
	.string	"app_btn_register_callback"
.LASF21:
	.string	"m_button_t"
.LASF27:
	.string	"APP_BTN_EVT_TRIPLE_CLICK"
.LASF12:
	.string	"p_btn_read_t"
.LASF0:
	.string	"signed char"
.LASF64:
	.string	"on_button_event_cb"
.LASF46:
	.string	"WIFI_START"
.LASF48:
	.string	"WIFI_CONNECTED"
.LASF83:
	.string	"m_button_get_instace"
.LASF80:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
	.ident	"GCC: (SiFive GCC-Metal 10.2.0-2020.12.8) 10.2.0"
