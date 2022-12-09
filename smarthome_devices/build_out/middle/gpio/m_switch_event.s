	.file	"m_switch_event.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.start_cylcic_timer,"ax",@progbits
	.align	1
	.globl	start_cylcic_timer
	.type	start_cylcic_timer, @function
start_cylcic_timer:
.LFB4:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_switch_event.c"
	.loc 1 12 1
	.cfi_startproc
.LVL0:
	.loc 1 13 5
	.loc 1 15 5
	.loc 1 12 1 is_stmt 0
	addi	sp,sp,-48
	.cfi_def_cfa_offset 48
	sw	s0,40(sp)
	.cfi_offset 8, -8
	.loc 1 13 37
	addi	s0,a0,-1
	.loc 1 12 1
	sw	s1,36(sp)
	sw	s2,32(sp)
	.loc 1 13 42
	srli	s0,s0,1
	.cfi_offset 9, -12
	.cfi_offset 18, -16
	lui	s1,%hi(.LANCHOR0)
	.loc 1 12 1
	sw	ra,44(sp)
	.cfi_offset 1, -4
	.loc 1 15 16
	andi	a5,a0,1
	.loc 1 12 1
	mv	s2,a0
	.loc 1 17 40
	andi	s0,s0,255
	addi	s1,s1,%lo(.LANCHOR0)
	.loc 1 15 8
	bne	a5,zero,.L2
	.loc 1 17 9 is_stmt 1
	.loc 1 17 49 is_stmt 0
	li	a5,12
	mul	a5,s0,a5
	add	a5,s1,a5
	sw	a2,4(a5)
	.loc 1 18 9 is_stmt 1
	.loc 1 18 50 is_stmt 0
	sw	a1,8(a5)
.L3:
	.loc 1 26 5 is_stmt 1
	addi	a0,sp,8
.LVL1:
	call	m_switch_get_instance
.LVL2:
	lw	a5,28(sp)
	li	a4,0
	li	a3,1
	li	a2,0
	li	a1,2
	mv	a0,s2
	jalr	a5
.LVL3:
	.loc 1 27 5
	.loc 1 27 51 is_stmt 0
	call	hw_get_tick_ms
.LVL4:
	.loc 1 27 49
	lui	a5,%hi(.LANCHOR1)
	slli	a4,s0,2
	addi	a5,a5,%lo(.LANCHOR1)
	add	a5,a5,a4
	sw	a0,0(a5)
	.loc 1 29 5 is_stmt 1
	.loc 1 29 44 is_stmt 0
	li	a5,12
	mul	s0,s0,a5
	li	a5,1
	add	s0,s1,s0
	sb	a5,0(s0)
	.loc 1 30 1
	lw	ra,44(sp)
	.cfi_remember_state
	.cfi_restore 1
	lw	s0,40(sp)
	.cfi_restore 8
	lw	s1,36(sp)
	.cfi_restore 9
	lw	s2,32(sp)
	.cfi_restore 18
.LVL5:
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
.LVL6:
.L2:
	.cfi_restore_state
	.loc 1 22 9 is_stmt 1
	.loc 1 22 49 is_stmt 0
	li	a5,12
	mul	a5,s0,a5
	add	a5,s1,a5
	sw	a1,4(a5)
	.loc 1 23 9 is_stmt 1
	.loc 1 23 50 is_stmt 0
	sw	a2,8(a5)
	j	.L3
	.cfi_endproc
.LFE4:
	.size	start_cylcic_timer, .-start_cylcic_timer
	.section	.text.stop_cylcic_timer,"ax",@progbits
	.align	1
	.globl	stop_cylcic_timer
	.type	stop_cylcic_timer, @function
stop_cylcic_timer:
.LFB5:
	.loc 1 33 1 is_stmt 1
	.cfi_startproc
.LVL7:
	.loc 1 34 5
	.loc 1 35 5
	.loc 1 37 5
	.loc 1 34 13 is_stmt 0
	addi	a0,a0,-1
.LVL8:
	.loc 1 37 28
	andi	a5,a0,0xff
	.loc 1 37 36
	li	a0,12
.LVL9:
	mul	a0,a5,a0
	lui	a5,%hi(.LANCHOR0)
	addi	a5,a5,%lo(.LANCHOR0)
	add	a5,a5,a0
	sb	zero,0(a5)
	.loc 1 38 5 is_stmt 1
	.loc 1 38 37 is_stmt 0
	sw	zero,4(a5)
	.loc 1 39 5 is_stmt 1
	.loc 1 39 38 is_stmt 0
	sw	zero,8(a5)
	.loc 1 40 1
	ret
	.cfi_endproc
.LFE5:
	.size	stop_cylcic_timer, .-stop_cylcic_timer
	.section	.text.clear_cyclic_timer,"ax",@progbits
	.align	1
	.globl	clear_cyclic_timer
	.type	clear_cyclic_timer, @function
clear_cyclic_timer:
.LFB6:
	.loc 1 44 1 is_stmt 1
	.cfi_startproc
	.loc 1 45 5
	.loc 1 48 5
.LBB2:
	.loc 1 48 10
.LVL10:
.LBE2:
	.loc 1 44 1 is_stmt 0
	addi	sp,sp,-48
	.cfi_def_cfa_offset 48
	sw	s0,40(sp)
	sw	ra,44(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
.LBB3:
	.loc 1 48 18
	li	s0,1
.LVL11:
.L7:
	.loc 1 48 25 is_stmt 1 discriminator 1
	.loc 1 48 30 is_stmt 0 discriminator 1
	addi	a0,sp,8
	call	m_switch_get_instance
.LVL12:
	.loc 1 48 5 discriminator 1
	lbu	a5,8(sp)
	bgeu	a5,s0,.L8
.LBE3:
	.loc 1 50 1
	lw	ra,44(sp)
	.cfi_remember_state
	.cfi_restore 1
	lw	s0,40(sp)
	.cfi_restore 8
.LVL13:
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
.LVL14:
.L8:
	.cfi_restore_state
.LBB4:
	.loc 1 49 9 is_stmt 1 discriminator 3
	mv	a0,s0
	.loc 1 48 75 is_stmt 0 discriminator 3
	addi	s0,s0,1
.LVL15:
	.loc 1 49 9 discriminator 3
	call	stop_cylcic_timer
.LVL16:
	.loc 1 48 74 is_stmt 1 discriminator 3
	.loc 1 48 75 is_stmt 0 discriminator 3
	andi	s0,s0,0xff
.LVL17:
	j	.L7
.LBE4:
	.cfi_endproc
.LFE6:
	.size	clear_cyclic_timer, .-clear_cyclic_timer
	.section	.text.m_switch_cyclic_timer_loop,"ax",@progbits
	.align	1
	.globl	m_switch_cyclic_timer_loop
	.type	m_switch_cyclic_timer_loop, @function
m_switch_cyclic_timer_loop:
.LFB7:
	.loc 1 53 1 is_stmt 1
	.cfi_startproc
	.loc 1 55 5
.LBB5:
	.loc 1 55 10
.LVL18:
.LBE5:
	.loc 1 53 1 is_stmt 0
	addi	sp,sp,-112
	.cfi_def_cfa_offset 112
	sw	s3,92(sp)
	sw	s4,88(sp)
	sw	s5,84(sp)
	.cfi_offset 19, -20
	.cfi_offset 20, -24
	.cfi_offset 21, -28
.LBB6:
	.loc 1 57 32
	lui	s3,%hi(.LANCHOR0)
	.loc 1 59 72
	lui	s4,%hi(.LANCHOR1)
	.loc 1 61 32
	lui	s5,%hi(.LANCHOR2)
.LBE6:
	.loc 1 53 1
	sw	s0,104(sp)
	sw	ra,108(sp)
	sw	s1,100(sp)
	sw	s2,96(sp)
	sw	s6,80(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	.cfi_offset 9, -12
	.cfi_offset 18, -16
	.cfi_offset 22, -32
.LBB7:
	.loc 1 55 18
	li	s0,0
	.loc 1 57 32
	addi	s3,s3,%lo(.LANCHOR0)
	.loc 1 59 72
	addi	s4,s4,%lo(.LANCHOR1)
	.loc 1 61 32
	addi	s5,s5,%lo(.LANCHOR2)
.LVL19:
.L11:
	.loc 1 55 25 is_stmt 1 discriminator 1
	.loc 1 55 29 is_stmt 0 discriminator 1
	addi	a0,sp,56
	call	m_switch_get_instance
.LVL20:
	.loc 1 55 5 discriminator 1
	lbu	a5,56(sp)
	bgtu	a5,s0,.L17
.LBE7:
	.loc 1 90 1
	lw	ra,108(sp)
	.cfi_remember_state
	.cfi_restore 1
	lw	s0,104(sp)
	.cfi_restore 8
.LVL21:
	lw	s1,100(sp)
	.cfi_restore 9
	lw	s2,96(sp)
	.cfi_restore 18
	lw	s3,92(sp)
	.cfi_restore 19
	lw	s4,88(sp)
	.cfi_restore 20
	lw	s5,84(sp)
	.cfi_restore 21
	lw	s6,80(sp)
	.cfi_restore 22
	addi	sp,sp,112
	.cfi_def_cfa_offset 0
	jr	ra
.LVL22:
.L17:
	.cfi_restore_state
.LBB8:
	.loc 1 57 9 is_stmt 1
	.loc 1 57 32 is_stmt 0
	li	s2,12
	mul	s2,s0,s2
	add	s2,s3,s2
	.loc 1 57 12
	lbu	a5,0(s2)
	beq	a5,zero,.L13
	.loc 1 59 13 is_stmt 1
	.loc 1 59 72 is_stmt 0
	slli	s1,s0,2
	.loc 1 59 25
	call	hw_get_tick_ms
.LVL23:
	.loc 1 59 72
	add	s6,s4,s1
	.loc 1 59 42
	lw	a5,0(s6)
	.loc 1 61 32
	add	s1,s5,s1
	.loc 1 59 42
	sub	a0,a0,a5
	.loc 1 61 13 is_stmt 1
	.loc 1 61 32 is_stmt 0
	lw	a5,0(s1)
	.loc 1 61 13
	beq	a5,zero,.L14
	li	a4,1
	beq	a5,a4,.L15
.L13:
	.loc 1 55 73 is_stmt 1 discriminator 2
	.loc 1 55 74 is_stmt 0 discriminator 2
	addi	s0,s0,1
.LVL24:
	andi	s0,s0,0xff
.LVL25:
	j	.L11
.L14:
	.loc 1 65 17 is_stmt 1
	.loc 1 65 61 is_stmt 0
	lw	a4,4(s2)
	li	a5,1000
	mul	a5,a5,a4
	.loc 1 65 20
	bleu	a0,a5,.L13
	.loc 1 67 21 is_stmt 1
	addi	a0,sp,8
	call	m_switch_get_instance
.LVL26:
	lw	a5,24(sp)
	li	a3,0
	li	a2,1
	li	a1,2
	mv	a0,s0
.L19:
	.loc 1 78 21 is_stmt 0
	jalr	a5
.LVL27:
	.loc 1 79 21 is_stmt 1
	.loc 1 79 54 is_stmt 0
	lw	a5,0(s1)
	addi	a5,a5,1
	.loc 1 79 59
	andi	a5,a5,1
	.loc 1 79 36
	sw	a5,0(s1)
	.loc 1 80 21 is_stmt 1
	.loc 1 80 55 is_stmt 0
	call	hw_get_tick_ms
.LVL28:
	.loc 1 80 53
	sw	a0,0(s6)
	j	.L13
.L15:
	.loc 1 76 17 is_stmt 1
	.loc 1 76 62 is_stmt 0
	lw	a4,8(s2)
	li	a5,1000
	mul	a5,a5,a4
	.loc 1 76 20
	bleu	a0,a5,.L13
	.loc 1 78 21 is_stmt 1
	addi	a0,sp,32
	call	m_switch_get_instance
.LVL29:
	lw	a5,48(sp)
	li	a3,0
	li	a2,1
	li	a1,2
	mv	a0,s0
	j	.L19
.LBE8:
	.cfi_endproc
.LFE7:
	.size	m_switch_cyclic_timer_loop, .-m_switch_cyclic_timer_loop
	.section	.text.set_user_trigger_press_event,"ax",@progbits
	.align	1
	.globl	set_user_trigger_press_event
	.type	set_user_trigger_press_event, @function
set_user_trigger_press_event:
.LFB8:
	.loc 1 100 1
	.cfi_startproc
.LVL30:
	.loc 1 102 5
	.loc 1 100 1 is_stmt 0
	addi	sp,sp,-48
	.cfi_def_cfa_offset 48
	sw	s0,40(sp)
	.cfi_offset 8, -8
	mv	s0,a0
	.loc 1 102 26
	addi	a0,sp,8
.LVL31:
	.loc 1 100 1
	sw	s1,36(sp)
	sw	ra,44(sp)
	.cfi_offset 9, -12
	.cfi_offset 1, -4
	.loc 1 100 1
	mv	s1,a1
	.loc 1 102 26
	call	m_switch_get_instance
.LVL32:
	.loc 1 102 8
	lbu	a5,8(sp)
	bleu	a5,s0,.L20
	.loc 1 105 5 is_stmt 1
	.loc 1 105 8 is_stmt 0
	beq	s1,zero,.L20
	.loc 1 108 5 is_stmt 1
	.loc 1 108 45 is_stmt 0
	call	hw_get_tick_ms
.LVL33:
	.loc 1 108 43
	lui	a4,%hi(.LANCHOR3)
	addi	a4,a4,%lo(.LANCHOR3)
	slli	a5,s0,2
	add	a5,a4,a5
	sw	a0,0(a5)
	.loc 1 110 5 is_stmt 1
	.loc 1 110 21 is_stmt 0
	call	m_db_get_instance
.LVL34:
	.loc 1 111 5 is_stmt 1
	.loc 1 110 14 is_stmt 0
	addi	a5,s0,196
.LVL35:
	slli	a5,a5,2
.LVL36:
	add	a0,a0,a5
.LVL37:
	.loc 1 111 8
	lw	a5,12(a0)
	beq	a5,zero,.L20
	.loc 1 114 5 is_stmt 1
	.loc 1 114 46 is_stmt 0
	lui	a0,%hi(.LANCHOR4)
	addi	a0,a0,%lo(.LANCHOR4)
	add	s0,a0,s0
	li	a5,1
	sb	a5,0(s0)
.L20:
	.loc 1 115 1
	lw	ra,44(sp)
	.cfi_restore 1
	lw	s0,40(sp)
	.cfi_restore 8
	lw	s1,36(sp)
	.cfi_restore 9
	addi	sp,sp,48
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE8:
	.size	set_user_trigger_press_event, .-set_user_trigger_press_event
	.section	.text.m_switch_press_release_loop,"ax",@progbits
	.align	1
	.globl	m_switch_press_release_loop
	.type	m_switch_press_release_loop, @function
m_switch_press_release_loop:
.LFB9:
	.loc 1 118 1 is_stmt 1
	.cfi_startproc
	.loc 1 119 5
	.loc 1 120 5
.LVL38:
	.loc 1 121 5
	.loc 1 122 5
	.loc 1 125 5
.LBB9:
	.loc 1 125 10
.LBE9:
	.loc 1 118 1 is_stmt 0
	addi	sp,sp,-112
	.cfi_def_cfa_offset 112
	sw	s1,100(sp)
	sw	s2,96(sp)
	.cfi_offset 9, -12
	.cfi_offset 18, -16
.LBB10:
	.loc 1 127 70
	lui	s1,%hi(.LANCHOR3)
	.loc 1 130 38
	lui	s2,%hi(.LANCHOR4)
.LBE10:
	.loc 1 118 1
	sw	s0,104(sp)
	sw	s3,92(sp)
	sw	ra,108(sp)
	sw	s4,88(sp)
	.cfi_offset 8, -8
	.cfi_offset 19, -20
	.cfi_offset 1, -4
	.cfi_offset 20, -24
.LBB11:
	.loc 1 125 18
	li	s0,0
	.loc 1 127 27
	lui	s3,%hi(.LANCHOR5)
	.loc 1 127 70
	addi	s1,s1,%lo(.LANCHOR3)
	.loc 1 130 38
	addi	s2,s2,%lo(.LANCHOR4)
.LVL39:
.L32:
	.loc 1 125 25 is_stmt 1 discriminator 1
	.loc 1 125 29 is_stmt 0 discriminator 1
	addi	a0,sp,56
	call	m_switch_get_instance
.LVL40:
	.loc 1 125 5 discriminator 1
	lbu	a5,56(sp)
	bgtu	a5,s0,.L39
.LBE11:
	.loc 1 151 1
	lw	ra,108(sp)
	.cfi_remember_state
	.cfi_restore 1
	lw	s0,104(sp)
	.cfi_restore 8
.LVL41:
	lw	s1,100(sp)
	.cfi_restore 9
	lw	s2,96(sp)
	.cfi_restore 18
	lw	s3,92(sp)
	.cfi_restore 19
	lw	s4,88(sp)
	.cfi_restore 20
	addi	sp,sp,112
	.cfi_def_cfa_offset 0
	jr	ra
.LVL42:
.L39:
	.cfi_restore_state
.LBB12:
	.loc 1 127 9 is_stmt 1
	.loc 1 127 29 is_stmt 0
	call	hw_get_tick_ms
.LVL43:
	.loc 1 127 70
	slli	a5,s0,2
	add	a5,s1,a5
	.loc 1 127 46
	lw	a5,0(a5)
	.loc 1 127 27
	addi	s4,s3,%lo(.LANCHOR5)
	.loc 1 127 46
	sub	a0,a0,a5
	.loc 1 127 27
	sw	a0,0(s4)
	.loc 1 128 9 is_stmt 1
	.loc 1 128 22 is_stmt 0
	call	m_db_get_instance
.LVL44:
	.loc 1 130 9 is_stmt 1
	.loc 1 130 38 is_stmt 0
	add	a4,s2,s0
	.loc 1 130 12
	lbu	a5,0(a4)
	beq	a5,zero,.L34
	.loc 1 128 78 discriminator 1
	addi	a5,s0,192
.LVL45:
	slli	a5,a5,2
.LVL46:
	add	a0,a0,a5
.LVL47:
	.loc 1 128 20 discriminator 1
	lw	a3,12(a0)
	li	a5,1000
	mul	a5,a5,a3
	.loc 1 130 42 discriminator 1
	lw	a3,0(s4)
	bleu	a3,a5,.L34
	.loc 1 132 13 is_stmt 1
	.loc 1 132 42 is_stmt 0
	sb	zero,0(a4)
	.loc 1 133 13 is_stmt 1
	.loc 1 133 20 is_stmt 0
	call	m_db_get_instance
.LVL48:
	.loc 1 133 18
	addi	a5,s0,196
	slli	a5,a5,2
	add	a0,a0,a5
	lw	s4,12(a0)
.LVL49:
	.loc 1 134 13 is_stmt 1
	.loc 1 134 30 is_stmt 0
	mv	a0,s0
	call	m_control_get_status_switch
.LVL50:
	.loc 1 134 61
	andi	a0,a0,1
	.loc 1 134 70
	seqz	a0,a0
	.loc 1 138 16
	li	a5,3
	.loc 1 134 70
	addi	a0,a0,1
.LVL51:
	.loc 1 136 13 is_stmt 1
	.loc 1 138 13
	.loc 1 138 16 is_stmt 0
	bne	s4,a5,.L38
	.loc 1 140 17 is_stmt 1
	addi	a0,sp,8
.LVL52:
	call	m_switch_get_instance
.LVL53:
	lw	a5,24(sp)
	li	a3,0
	li	a2,1
	li	a1,3
	mv	a0,s0
.L41:
	.loc 1 146 21 is_stmt 0
	jalr	a5
.LVL54:
.L34:
	.loc 1 125 73 is_stmt 1 discriminator 2
	.loc 1 125 74 is_stmt 0 discriminator 2
	addi	s0,s0,1
.LVL55:
	andi	s0,s0,0xff
.LVL56:
	j	.L32
.LVL57:
.L38:
	.loc 1 144 17 is_stmt 1
	.loc 1 144 20 is_stmt 0
	bne	a0,s4,.L34
	.loc 1 146 21 is_stmt 1
	addi	a0,sp,32
.LVL58:
	call	m_switch_get_instance
.LVL59:
	lw	a5,48(sp)
	li	a3,0
	li	a2,1
	li	a1,3
	mv	a0,s0
	j	.L41
.LBE12:
	.cfi_endproc
.LFE9:
	.size	m_switch_press_release_loop, .-m_switch_press_release_loop
	.section	.bss.cyclic_timer_arr,"aw",@nobits
	.align	2
	.set	.LANCHOR0,. + 0
	.type	cyclic_timer_arr, @object
	.size	cyclic_timer_arr, 48
cyclic_timer_arr:
	.zero	48
	.section	.bss.ms_tick_control_switch,"aw",@nobits
	.align	2
	.set	.LANCHOR3,. + 0
	.type	ms_tick_control_switch, @object
	.size	ms_tick_control_switch, 16
ms_tick_control_switch:
	.zero	16
	.section	.bss.previous_cyclic_timer_active,"aw",@nobits
	.align	2
	.set	.LANCHOR1,. + 0
	.type	previous_cyclic_timer_active, @object
	.size	previous_cyclic_timer_active, 16
previous_cyclic_timer_active:
	.zero	16
	.section	.bss.step_toggle,"aw",@nobits
	.align	2
	.set	.LANCHOR2,. + 0
	.type	step_toggle, @object
	.size	step_toggle, 16
step_toggle:
	.zero	16
	.section	.sbss.time_diff_release.0,"aw",@nobits
	.align	2
	.set	.LANCHOR5,. + 0
	.type	time_diff_release.0, @object
	.size	time_diff_release.0, 4
time_diff_release.0:
	.zero	4
	.section	.sbss.user_control_switch_event,"aw",@nobits
	.align	2
	.set	.LANCHOR4,. + 0
	.type	user_control_switch_event, @object
	.size	user_control_switch_event, 4
user_control_switch_event:
	.zero	4
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint-gcc.h"
	.file 3 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/app_config/system_typedef.h"
	.file 4 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_switch.h"
	.file 5 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.h"
	.file 6 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/database/m_database.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x4ef
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.byte	0x1
	.4byte	.LASF55
	.byte	0xc
	.4byte	.LASF56
	.4byte	.LASF57
	.4byte	.Ldebug_ranges0+0x70
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
	.4byte	0x4d
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
	.4byte	0x67
	.byte	0x2
	.byte	0x4
	.byte	0x7
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
	.byte	0x5
	.byte	0x7
	.byte	0x4
	.4byte	0x7c
	.byte	0x3
	.byte	0x38
	.byte	0xe
	.4byte	0xc9
	.byte	0x6
	.4byte	.LASF12
	.byte	0x1
	.byte	0x6
	.4byte	.LASF13
	.byte	0x2
	.byte	0x6
	.4byte	.LASF14
	.byte	0x3
	.byte	0x6
	.4byte	.LASF15
	.byte	0x4
	.byte	0x6
	.4byte	.LASF16
	.byte	0x5
	.byte	0x6
	.4byte	.LASF17
	.byte	0x6
	.byte	0x6
	.4byte	.LASF18
	.byte	0x7
	.byte	0x6
	.4byte	.LASF19
	.byte	0x8
	.byte	0
	.byte	0x3
	.4byte	.LASF20
	.byte	0x3
	.byte	0x41
	.byte	0x3
	.4byte	0x8a
	.byte	0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF21
	.byte	0x7
	.4byte	0x5b
	.4byte	0xec
	.byte	0x8
	.4byte	0x7c
	.byte	0x3
	.byte	0
	.byte	0x5
	.byte	0x7
	.byte	0x4
	.4byte	0x7c
	.byte	0x4
	.byte	0x52
	.byte	0xe
	.4byte	0x113
	.byte	0x6
	.4byte	.LASF22
	.byte	0
	.byte	0x6
	.4byte	.LASF23
	.byte	0x1
	.byte	0x6
	.4byte	.LASF24
	.byte	0x2
	.byte	0x6
	.4byte	.LASF25
	.byte	0x3
	.byte	0
	.byte	0x9
	.byte	0xc
	.byte	0x4
	.byte	0x59
	.byte	0x9
	.4byte	0x144
	.byte	0xa
	.4byte	.LASF26
	.byte	0x4
	.byte	0x5a
	.byte	0xd
	.4byte	0x41
	.byte	0
	.byte	0xa
	.4byte	.LASF27
	.byte	0x4
	.byte	0x5b
	.byte	0xe
	.4byte	0x5b
	.byte	0x4
	.byte	0xa
	.4byte	.LASF28
	.byte	0x4
	.byte	0x5c
	.byte	0xe
	.4byte	0x5b
	.byte	0x8
	.byte	0
	.byte	0x3
	.4byte	.LASF29
	.byte	0x4
	.byte	0x5d
	.byte	0x3
	.4byte	0x113
	.byte	0x7
	.4byte	0x144
	.4byte	0x160
	.byte	0x8
	.4byte	0x7c
	.byte	0x3
	.byte	0
	.byte	0xb
	.4byte	.LASF30
	.byte	0x1
	.byte	0x5
	.byte	0x1e
	.4byte	0x150
	.byte	0x5
	.byte	0x3
	.4byte	cyclic_timer_arr
	.byte	0xb
	.4byte	.LASF31
	.byte	0x1
	.byte	0x6
	.byte	0x11
	.4byte	0xdc
	.byte	0x5
	.byte	0x3
	.4byte	previous_cyclic_timer_active
	.byte	0xb
	.4byte	.LASF32
	.byte	0x1
	.byte	0x7
	.byte	0x11
	.4byte	0xdc
	.byte	0x5
	.byte	0x3
	.4byte	step_toggle
	.byte	0xc
	.4byte	.LASF58
	.byte	0x1
	.byte	0x8
	.byte	0x11
	.4byte	0x5b
	.byte	0xb
	.4byte	.LASF33
	.byte	0x1
	.byte	0x60
	.byte	0x11
	.4byte	0xdc
	.byte	0x5
	.byte	0x3
	.4byte	ms_tick_control_switch
	.byte	0x7
	.4byte	0x41
	.4byte	0x1c4
	.byte	0x8
	.4byte	0x7c
	.byte	0x3
	.byte	0
	.byte	0xb
	.4byte	.LASF34
	.byte	0x1
	.byte	0x61
	.byte	0x10
	.4byte	0x1b4
	.byte	0x5
	.byte	0x3
	.4byte	user_control_switch_event
	.byte	0xd
	.4byte	.LASF39
	.byte	0x1
	.byte	0x75
	.byte	0x6
	.4byte	.LFB9
	.4byte	.LFE9-.LFB9
	.byte	0x1
	.byte	0x9c
	.4byte	0x2ac
	.byte	0xb
	.4byte	.LASF35
	.byte	0x1
	.byte	0x77
	.byte	0x15
	.4byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	time_diff_release.0
	.byte	0xe
	.4byte	.LASF36
	.byte	0x1
	.byte	0x78
	.byte	0xe
	.4byte	0x5b
	.4byte	.LLST12
	.byte	0xe
	.4byte	.LASF37
	.byte	0x1
	.byte	0x79
	.byte	0xe
	.4byte	0x5b
	.4byte	.LLST13
	.byte	0xe
	.4byte	.LASF38
	.byte	0x1
	.byte	0x7a
	.byte	0xe
	.4byte	0x5b
	.4byte	.LLST14
	.byte	0xf
	.4byte	.Ldebug_ranges0+0x48
	.byte	0x10
	.string	"i"
	.byte	0x1
	.byte	0x7d
	.byte	0x12
	.4byte	0x41
	.4byte	.LLST15
	.byte	0x11
	.4byte	.LVL40
	.4byte	0x4c2
	.4byte	0x255
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x91
	.byte	0x48
	.byte	0
	.byte	0x13
	.4byte	.LVL43
	.4byte	0x4ce
	.byte	0x13
	.4byte	.LVL44
	.4byte	0x4da
	.byte	0x13
	.4byte	.LVL48
	.4byte	0x4da
	.byte	0x11
	.4byte	.LVL50
	.4byte	0x4e6
	.4byte	0x284
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x78
	.byte	0
	.byte	0
	.byte	0x11
	.4byte	.LVL53
	.4byte	0x4c2
	.4byte	0x299
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0x98,0x7f
	.byte	0
	.byte	0x14
	.4byte	.LVL59
	.4byte	0x4c2
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xb0,0x7f
	.byte	0
	.byte	0
	.byte	0
	.byte	0x15
	.4byte	.LASF40
	.byte	0x1
	.byte	0x63
	.byte	0x6
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.byte	0x1
	.byte	0x9c
	.4byte	0x329
	.byte	0x16
	.4byte	.LASF41
	.byte	0x1
	.byte	0x63
	.byte	0x2b
	.4byte	0x41
	.4byte	.LLST8
	.byte	0x16
	.4byte	.LASF42
	.byte	0x1
	.byte	0x63
	.byte	0x3e
	.4byte	0xd5
	.4byte	.LLST9
	.byte	0x16
	.4byte	.LASF43
	.byte	0x1
	.byte	0x63
	.byte	0x52
	.4byte	0x5b
	.4byte	.LLST10
	.byte	0xe
	.4byte	.LASF38
	.byte	0x1
	.byte	0x6e
	.byte	0xe
	.4byte	0x5b
	.4byte	.LLST11
	.byte	0x11
	.4byte	.LVL32
	.4byte	0x4c2
	.4byte	0x316
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x91
	.byte	0x58
	.byte	0
	.byte	0x13
	.4byte	.LVL33
	.4byte	0x4ce
	.byte	0x13
	.4byte	.LVL34
	.4byte	0x4da
	.byte	0
	.byte	0xd
	.4byte	.LASF44
	.byte	0x1
	.byte	0x34
	.byte	0x6
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.byte	0x1
	.byte	0x9c
	.4byte	0x3a0
	.byte	0xf
	.4byte	.Ldebug_ranges0+0x20
	.byte	0x10
	.string	"i"
	.byte	0x1
	.byte	0x37
	.byte	0x12
	.4byte	0x41
	.4byte	.LLST7
	.byte	0x11
	.4byte	.LVL20
	.4byte	0x4c2
	.4byte	0x366
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x91
	.byte	0x48
	.byte	0
	.byte	0x13
	.4byte	.LVL23
	.4byte	0x4ce
	.byte	0x11
	.4byte	.LVL26
	.4byte	0x4c2
	.4byte	0x384
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0x98,0x7f
	.byte	0
	.byte	0x13
	.4byte	.LVL28
	.4byte	0x4ce
	.byte	0x14
	.4byte	.LVL29
	.4byte	0x4c2
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xb0,0x7f
	.byte	0
	.byte	0
	.byte	0
	.byte	0x15
	.4byte	.LASF45
	.byte	0x1
	.byte	0x2b
	.byte	0x6
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.byte	0x1
	.byte	0x9c
	.4byte	0x3ef
	.byte	0xf
	.4byte	.Ldebug_ranges0+0
	.byte	0x10
	.string	"i"
	.byte	0x1
	.byte	0x30
	.byte	0x12
	.4byte	0x41
	.4byte	.LLST6
	.byte	0x11
	.4byte	.LVL12
	.4byte	0x4c2
	.4byte	0x3dd
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x91
	.byte	0x58
	.byte	0
	.byte	0x14
	.4byte	.LVL16
	.4byte	0x3ef
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x78
	.byte	0x7f
	.byte	0
	.byte	0
	.byte	0
	.byte	0x15
	.4byte	.LASF46
	.byte	0x1
	.byte	0x20
	.byte	0x6
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.byte	0x1
	.byte	0x9c
	.4byte	0x426
	.byte	0x16
	.4byte	.LASF41
	.byte	0x1
	.byte	0x20
	.byte	0x21
	.4byte	0x5b
	.4byte	.LLST4
	.byte	0xe
	.4byte	.LASF47
	.byte	0x1
	.byte	0x22
	.byte	0xd
	.4byte	0x41
	.4byte	.LLST5
	.byte	0
	.byte	0x15
	.4byte	.LASF48
	.byte	0x1
	.byte	0xb
	.byte	0x6
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.byte	0x1
	.byte	0x9c
	.4byte	0x4c2
	.byte	0x16
	.4byte	.LASF43
	.byte	0x1
	.byte	0xb
	.byte	0x29
	.4byte	0xc9
	.4byte	.LLST0
	.byte	0x16
	.4byte	.LASF49
	.byte	0x1
	.byte	0xb
	.byte	0x3a
	.4byte	0x5b
	.4byte	.LLST1
	.byte	0x16
	.4byte	.LASF50
	.byte	0x1
	.byte	0xb
	.byte	0x4e
	.4byte	0x5b
	.4byte	.LLST2
	.byte	0xe
	.4byte	.LASF41
	.byte	0x1
	.byte	0xd
	.byte	0xd
	.4byte	0x41
	.4byte	.LLST3
	.byte	0x11
	.4byte	.LVL2
	.4byte	0x4c2
	.4byte	0x490
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x91
	.byte	0x58
	.byte	0
	.byte	0x17
	.4byte	.LVL3
	.byte	0x3
	.byte	0x91
	.byte	0x6c
	.byte	0x6
	.4byte	0x4b8
	.byte	0x12
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x82
	.byte	0
	.byte	0x12
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x32
	.byte	0x12
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0x12
	.byte	0x1
	.byte	0x5d
	.byte	0x1
	.byte	0x31
	.byte	0x12
	.byte	0x1
	.byte	0x5e
	.byte	0x1
	.byte	0x30
	.byte	0
	.byte	0x13
	.4byte	.LVL4
	.4byte	0x4ce
	.byte	0
	.byte	0x18
	.4byte	.LASF51
	.4byte	.LASF51
	.byte	0x4
	.byte	0x4e
	.byte	0xc
	.byte	0x18
	.4byte	.LASF52
	.4byte	.LASF52
	.byte	0x5
	.byte	0x27
	.byte	0xa
	.byte	0x18
	.4byte	.LASF53
	.4byte	.LASF53
	.byte	0x6
	.byte	0x4d
	.byte	0xf
	.byte	0x18
	.4byte	.LASF54
	.4byte	.LASF54
	.byte	0x4
	.byte	0x4f
	.byte	0x12
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
	.byte	0x6
	.byte	0x28
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x1c
	.byte	0xb
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
	.byte	0x49
	.byte	0x13
	.byte	0x2f
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x9
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
	.byte	0xa
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
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0xc
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
	.byte	0xd
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
	.byte	0xe
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
	.byte	0xf
	.byte	0xb
	.byte	0x1
	.byte	0x55
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x10
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
	.byte	0x11
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
	.byte	0x12
	.byte	0x8a,0x82,0x1
	.byte	0
	.byte	0x2
	.byte	0x18
	.byte	0x91,0x42
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x13
	.byte	0x89,0x82,0x1
	.byte	0
	.byte	0x11
	.byte	0x1
	.byte	0x31
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
	.byte	0x16
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
	.byte	0x17
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x93,0x42
	.byte	0x18
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x18
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
.LLST12:
	.4byte	.LVL38
	.4byte	.LVL39
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL44
	.4byte	.LVL45
	.2byte	0x10
	.byte	0x78
	.byte	0xc0,0x1
	.byte	0x32
	.byte	0x24
	.byte	0x7a
	.byte	0
	.byte	0x22
	.byte	0x23
	.byte	0xc
	.byte	0x6
	.byte	0xa
	.2byte	0x3e8
	.byte	0x1e
	.byte	0x9f
	.4byte	.LVL45
	.4byte	.LVL46
	.2byte	0xf
	.byte	0x7f
	.byte	0
	.byte	0x32
	.byte	0x24
	.byte	0x7a
	.byte	0
	.byte	0x22
	.byte	0x23
	.byte	0xc
	.byte	0x6
	.byte	0xa
	.2byte	0x3e8
	.byte	0x1e
	.byte	0x9f
	.4byte	.LVL46
	.4byte	.LVL47
	.2byte	0x10
	.byte	0x78
	.byte	0xc0,0x1
	.byte	0x32
	.byte	0x24
	.byte	0x7a
	.byte	0
	.byte	0x22
	.byte	0x23
	.byte	0xc
	.byte	0x6
	.byte	0xa
	.2byte	0x3e8
	.byte	0x1e
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST13:
	.4byte	.LVL38
	.4byte	.LVL39
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL51
	.4byte	.LVL52
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL57
	.4byte	.LVL58
	.2byte	0x1
	.byte	0x5a
	.4byte	0
	.4byte	0
.LLST14:
	.4byte	.LVL49
	.4byte	.LVL54
	.2byte	0x1
	.byte	0x64
	.4byte	.LVL57
	.4byte	.LFE9
	.2byte	0x1
	.byte	0x64
	.4byte	0
	.4byte	0
.LLST15:
	.4byte	.LVL38
	.4byte	.LVL39
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL39
	.4byte	.LVL41
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL42
	.4byte	.LVL55
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL56
	.4byte	.LFE9
	.2byte	0x1
	.byte	0x58
	.4byte	0
	.4byte	0
.LLST8:
	.4byte	.LVL30
	.4byte	.LVL31
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL31
	.4byte	.LFE8
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST9:
	.4byte	.LVL30
	.4byte	.LVL32-1
	.2byte	0x1
	.byte	0x5b
	.4byte	.LVL32-1
	.4byte	.LFE8
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5b
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST10:
	.4byte	.LVL30
	.4byte	.LVL32-1
	.2byte	0x1
	.byte	0x5c
	.4byte	.LVL32-1
	.4byte	.LFE8
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5c
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST11:
	.4byte	.LVL34
	.4byte	.LVL35
	.2byte	0xa
	.byte	0x78
	.byte	0xc4,0x1
	.byte	0x32
	.byte	0x24
	.byte	0x7a
	.byte	0
	.byte	0x22
	.byte	0x23
	.byte	0xc
	.4byte	.LVL35
	.4byte	.LVL36
	.2byte	0x9
	.byte	0x7f
	.byte	0
	.byte	0x32
	.byte	0x24
	.byte	0x7a
	.byte	0
	.byte	0x22
	.byte	0x23
	.byte	0xc
	.4byte	.LVL36
	.4byte	.LVL37
	.2byte	0xa
	.byte	0x78
	.byte	0xc4,0x1
	.byte	0x32
	.byte	0x24
	.byte	0x7a
	.byte	0
	.byte	0x22
	.byte	0x23
	.byte	0xc
	.4byte	0
	.4byte	0
.LLST7:
	.4byte	.LVL18
	.4byte	.LVL19
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL19
	.4byte	.LVL21
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL22
	.4byte	.LVL24
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL25
	.4byte	.LFE7
	.2byte	0x1
	.byte	0x58
	.4byte	0
	.4byte	0
.LLST6:
	.4byte	.LVL10
	.4byte	.LVL11
	.2byte	0x2
	.byte	0x31
	.byte	0x9f
	.4byte	.LVL11
	.4byte	.LVL13
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL14
	.4byte	.LVL15
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL17
	.4byte	.LFE6
	.2byte	0x1
	.byte	0x58
	.4byte	0
	.4byte	0
.LLST4:
	.4byte	.LVL7
	.4byte	.LVL8
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL8
	.4byte	.LVL9
	.2byte	0x3
	.byte	0x7a
	.byte	0x1
	.byte	0x9f
	.4byte	.LVL9
	.4byte	.LFE5
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST5:
	.4byte	.LVL7
	.4byte	.LVL8
	.2byte	0x3
	.byte	0x7a
	.byte	0x7f
	.byte	0x9f
	.4byte	.LVL8
	.4byte	.LVL9
	.2byte	0x3
	.byte	0x7a
	.byte	0
	.byte	0x9f
	.4byte	.LVL9
	.4byte	.LFE5
	.2byte	0x6
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x31
	.byte	0x1c
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST0:
	.4byte	.LVL0
	.4byte	.LVL1
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL1
	.4byte	.LVL5
	.2byte	0x1
	.byte	0x62
	.4byte	.LVL5
	.4byte	.LVL6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL6
	.4byte	.LFE4
	.2byte	0x1
	.byte	0x5a
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LVL0
	.4byte	.LVL2-1
	.2byte	0x1
	.byte	0x5b
	.4byte	.LVL2-1
	.4byte	.LVL6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5b
	.byte	0x9f
	.4byte	.LVL6
	.4byte	.LFE4
	.2byte	0x1
	.byte	0x5b
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL0
	.4byte	.LVL2-1
	.2byte	0x1
	.byte	0x5c
	.4byte	.LVL2-1
	.4byte	.LVL6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5c
	.byte	0x9f
	.4byte	.LVL6
	.4byte	.LFE4
	.2byte	0x1
	.byte	0x5c
	.4byte	0
	.4byte	0
.LLST3:
	.4byte	.LVL0
	.4byte	.LVL1
	.2byte	0x5
	.byte	0x7a
	.byte	0x7f
	.byte	0x31
	.byte	0x25
	.byte	0x9f
	.4byte	.LVL1
	.4byte	.LVL5
	.2byte	0x5
	.byte	0x82
	.byte	0x7f
	.byte	0x31
	.byte	0x25
	.byte	0x9f
	.4byte	.LVL5
	.4byte	.LVL6
	.2byte	0x8
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x31
	.byte	0x1c
	.byte	0x31
	.byte	0x25
	.byte	0x9f
	.4byte	.LVL6
	.4byte	.LFE4
	.2byte	0x5
	.byte	0x7a
	.byte	0x7f
	.byte	0x31
	.byte	0x25
	.byte	0x9f
	.4byte	0
	.4byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x44
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
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.4byte	.LFB9
	.4byte	.LFE9-.LFB9
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.4byte	.LBB2
	.4byte	.LBE2
	.4byte	.LBB3
	.4byte	.LBE3
	.4byte	.LBB4
	.4byte	.LBE4
	.4byte	0
	.4byte	0
	.4byte	.LBB5
	.4byte	.LBE5
	.4byte	.LBB6
	.4byte	.LBE6
	.4byte	.LBB7
	.4byte	.LBE7
	.4byte	.LBB8
	.4byte	.LBE8
	.4byte	0
	.4byte	0
	.4byte	.LBB9
	.4byte	.LBE9
	.4byte	.LBB10
	.4byte	.LBE10
	.4byte	.LBB11
	.4byte	.LBE11
	.4byte	.LBB12
	.4byte	.LBE12
	.4byte	0
	.4byte	0
	.4byte	.LFB4
	.4byte	.LFE4
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
.LASF3:
	.string	"long long int"
.LASF35:
	.string	"time_diff_release"
.LASF13:
	.string	"CH1_OFF"
.LASF42:
	.string	"set_event"
.LASF36:
	.string	"time_event"
.LASF21:
	.string	"_Bool"
.LASF54:
	.string	"m_control_get_status_switch"
.LASF49:
	.string	"time_act1"
.LASF50:
	.string	"time_act2"
.LASF40:
	.string	"set_user_trigger_press_event"
.LASF18:
	.string	"CH4_ON"
.LASF57:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/middle"
.LASF26:
	.string	"enable"
.LASF28:
	.string	"time_off"
.LASF37:
	.string	"current_state"
.LASF15:
	.string	"CH2_OFF"
.LASF29:
	.string	"cyclic_event_switch_t"
.LASF39:
	.string	"m_switch_press_release_loop"
.LASF4:
	.string	"unsigned char"
.LASF24:
	.string	"MODE_OFF"
.LASF5:
	.string	"short unsigned int"
.LASF53:
	.string	"m_db_get_instance"
.LASF45:
	.string	"clear_cyclic_timer"
.LASF43:
	.string	"action"
.LASF44:
	.string	"m_switch_cyclic_timer_loop"
.LASF1:
	.string	"short int"
.LASF22:
	.string	"NOT_USE"
.LASF32:
	.string	"step_toggle"
.LASF12:
	.string	"CH1_ON"
.LASF41:
	.string	"channel_index"
.LASF8:
	.string	"long unsigned int"
.LASF10:
	.string	"unsigned int"
.LASF23:
	.string	"MODE_ON"
.LASF9:
	.string	"long long unsigned int"
.LASF6:
	.string	"uint8_t"
.LASF38:
	.string	"mode"
.LASF20:
	.string	"switch_action_t"
.LASF17:
	.string	"CH3_OFF"
.LASF14:
	.string	"CH2_ON"
.LASF34:
	.string	"user_control_switch_event"
.LASF56:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_switch_event.c"
.LASF55:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
.LASF11:
	.string	"char"
.LASF19:
	.string	"CH4_OFF"
.LASF47:
	.string	"index"
.LASF52:
	.string	"hw_get_tick_ms"
.LASF31:
	.string	"previous_cyclic_timer_active"
.LASF33:
	.string	"ms_tick_control_switch"
.LASF51:
	.string	"m_switch_get_instance"
.LASF58:
	.string	"time_diff"
.LASF7:
	.string	"uint32_t"
.LASF2:
	.string	"long int"
.LASF0:
	.string	"signed char"
.LASF25:
	.string	"MODE_BOTH"
.LASF46:
	.string	"stop_cylcic_timer"
.LASF16:
	.string	"CH3_ON"
.LASF48:
	.string	"start_cylcic_timer"
.LASF30:
	.string	"cyclic_timer_arr"
.LASF27:
	.string	"time_on"
	.ident	"GCC: (SiFive GCC-Metal 10.2.0-2020.12.8) 10.2.0"
