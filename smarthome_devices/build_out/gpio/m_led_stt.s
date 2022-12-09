	.file	"m_led_stt.c"
	.option nopic
	.attribute arch, "rv32i2p0_m2p0_f2p0_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.m_stop_timer,"ax",@progbits
	.align	1
	.type	m_stop_timer, @function
m_stop_timer:
.LFB6:
	.file 1 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_led_stt.c"
	.loc 1 39 1
	.cfi_startproc
	.loc 1 40 5
	.loc 1 40 17 is_stmt 0
	lui	a5,%hi(.LANCHOR0)
	lw	a0,%lo(.LANCHOR0)(a5)
	.loc 1 40 8
	beq	a0,zero,.L1
	.loc 1 41 9 is_stmt 1
	li	a4,10
	li	a3,0
	li	a2,0
	li	a1,3
	tail	xTimerGenericCommand
.LVL0:
.L1:
	.loc 1 43 1 is_stmt 0
	ret
	.cfi_endproc
.LFE6:
	.size	m_stop_timer, .-m_stop_timer
	.section	.text.m_start_timer,"ax",@progbits
	.align	1
	.type	m_start_timer, @function
m_start_timer:
.LFB5:
	.loc 1 31 1 is_stmt 1
	.cfi_startproc
.LVL1:
	.loc 1 32 5
	.loc 1 31 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	.cfi_offset 8, -8
	.loc 1 32 17
	lui	s0,%hi(.LANCHOR0)
	.loc 1 31 1
	sw	ra,12(sp)
	.cfi_offset 1, -4
	.loc 1 32 17
	addi	s0,s0,%lo(.LANCHOR0)
	.loc 1 31 1
	mv	a5,a0
	.loc 1 32 17
	lw	a0,0(s0)
.LVL2:
	.loc 1 32 8
	beq	a0,zero,.L3
	.loc 1 33 9 is_stmt 1
	.loc 1 33 116 is_stmt 0
	li	a2,1000
	mul	a5,a5,a2
.LVL3:
	.loc 1 33 9
	li	a4,10
	li	a3,0
	li	a1,4
	divu	a2,a5,a2
	call	xTimerGenericCommand
.LVL4:
	.loc 1 34 9 is_stmt 1
	lw	s0,0(s0)
	call	xTaskGetTickCount
.LVL5:
	mv	a2,a0
	mv	a0,s0
	.loc 1 36 1 is_stmt 0
	lw	s0,8(sp)
	.cfi_remember_state
	.cfi_restore 8
	lw	ra,12(sp)
	.cfi_restore 1
	.loc 1 34 9
	li	a4,10
	.loc 1 36 1
	.loc 1 34 9
	li	a3,0
	li	a1,1
	.loc 1 36 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	.loc 1 34 9
	tail	xTimerGenericCommand
.LVL6:
.L3:
	.cfi_restore_state
	.loc 1 36 1
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s0,8(sp)
	.cfi_restore 8
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE5:
	.size	m_start_timer, .-m_start_timer
	.section	.text.vTimerCallback,"ax",@progbits
	.align	1
	.type	vTimerCallback, @function
vTimerCallback:
.LFB4:
	.loc 1 23 1 is_stmt 1
	.cfi_startproc
.LVL7:
	.loc 1 24 5
	.loc 1 23 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	.cfi_offset 8, -8
	.loc 1 24 18
	lui	s0,%hi(.LANCHOR1)
	addi	s0,s0,%lo(.LANCHOR1)
	.loc 1 24 5
	lw	a5,20(s0)
	.loc 1 23 1
	sw	ra,12(sp)
	.cfi_offset 1, -4
	.loc 1 24 5
	jalr	a5
.LVL8:
	.loc 1 25 5 is_stmt 1
	.loc 1 25 53 is_stmt 0
	lbu	a5,0(s0)
	.loc 1 25 92
	lbu	a4,1(s0)
	.loc 1 25 72
	addi	a5,a5,1
	.loc 1 25 77
	rem	a5,a5,a4
	.loc 1 25 37
	sb	a5,0(s0)
	.loc 1 26 5 is_stmt 1
	call	m_stop_timer
.LVL9:
	.loc 1 27 5
	.loc 1 27 48 is_stmt 0
	lbu	a5,0(s0)
	.loc 1 28 1
	lw	s0,8(sp)
	.cfi_restore 8
	lw	ra,12(sp)
	.cfi_restore 1
	.loc 1 27 34
	slli	a4,a5,1
	lui	a5,%hi(.LANCHOR2)
	addi	a5,a5,%lo(.LANCHOR2)
	add	a5,a5,a4
	.loc 1 27 5
	lhu	a0,0(a5)
	.loc 1 28 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	.loc 1 27 5
	tail	m_start_timer
.LVL10:
	.cfi_endproc
.LFE4:
	.size	vTimerCallback, .-vTimerCallback
	.section	.text.set_led_stt_device_active,"ax",@progbits
	.align	1
	.type	set_led_stt_device_active, @function
set_led_stt_device_active:
.LFB11:
	.loc 1 137 1 is_stmt 1
	.cfi_startproc
	.loc 1 138 5
	.loc 1 138 22 is_stmt 0
	lui	a5,%hi(device_type)
	lbu	a4,%lo(device_type)(a5)
	.loc 1 137 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	.cfi_offset 8, -8
	.loc 1 147 26
	lui	s0,%hi(.LANCHOR1)
	.loc 1 137 1
	sw	s1,4(sp)
	sw	ra,12(sp)
	.cfi_offset 9, -12
	.cfi_offset 1, -4
	.loc 1 147 26
	addi	s1,s0,%lo(.LANCHOR1)
	.loc 1 138 22
	andi	a4,a4,0xff
	.loc 1 147 26
	lw	a3,4(s1)
	addi	s0,s0,%lo(.LANCHOR1)
	.loc 1 138 8
	beq	a4,zero,.L9
	.loc 1 138 48 discriminator 1
	lbu	a4,%lo(device_type)(a5)
	.loc 1 138 33 discriminator 1
	li	a2,2
	.loc 1 138 48 discriminator 1
	andi	a4,a4,0xff
	.loc 1 138 33 discriminator 1
	beq	a4,a2,.L9
	.loc 1 138 75 discriminator 2
	lbu	a5,%lo(device_type)(a5)
	andi	a5,a5,0xff
	.loc 1 138 60 discriminator 2
	bne	a5,a2,.L10
.L9:
	.loc 1 139 9 is_stmt 1
	.loc 1 139 12 is_stmt 0
	li	s1,5
	beq	a3,s1,.L8
	.loc 1 140 13 is_stmt 1
	.loc 1 142 13
	call	m_stop_timer
.LVL11:
	.loc 1 143 13
	lw	a5,16(s0)
	jalr	a5
.LVL12:
	.loc 1 144 13
	.loc 1 144 41 is_stmt 0
	sw	s1,4(s0)
.L8:
	.loc 1 155 1
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
.L10:
	.cfi_restore_state
	.loc 1 147 9 is_stmt 1
	.loc 1 147 12 is_stmt 0
	li	s0,4
	beq	a3,s0,.L8
.LBB4:
.LBB5:
	.loc 1 148 13 is_stmt 1
	.loc 1 150 13
	call	m_stop_timer
.LVL13:
	.loc 1 151 13
	lw	a5,12(s1)
	jalr	a5
.LVL14:
	.loc 1 152 13
	.loc 1 152 41 is_stmt 0
	sw	s0,4(s1)
.LBE5:
.LBE4:
	.loc 1 155 1
	j	.L8
	.cfi_endproc
.LFE11:
	.size	set_led_stt_device_active, .-set_led_stt_device_active
	.section	.text.set_led_stt_update_ota,"ax",@progbits
	.align	1
	.type	set_led_stt_update_ota, @function
set_led_stt_update_ota:
.LFB9:
	.loc 1 86 1 is_stmt 1
	.cfi_startproc
	.loc 1 87 4
	.loc 1 86 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	.cfi_offset 8, -8
	.loc 1 87 21
	lui	s0,%hi(.LANCHOR1)
	addi	s0,s0,%lo(.LANCHOR1)
	.loc 1 87 7
	lw	a5,4(s0)
	.loc 1 86 1
	sw	s1,4(sp)
	sw	ra,12(sp)
	.cfi_offset 9, -12
	.cfi_offset 1, -4
	.loc 1 87 7
	li	s1,6
	beq	a5,s1,.L16
.LBB8:
.LBB9:
	.loc 1 88 9 is_stmt 1
	.loc 1 90 9
	call	m_stop_timer
.LVL15:
	.loc 1 93 9
	lw	a5,16(s0)
	jalr	a5
.LVL16:
	.loc 1 96 9
	.loc 1 96 28 is_stmt 0
	li	a5,32768000
	addi	a5,a5,500
	lui	a4,%hi(.LANCHOR2)
	sw	a5,%lo(.LANCHOR2)(a4)
	.loc 1 97 9 is_stmt 1
	.loc 1 98 9
	.loc 1 97 41 is_stmt 0
	li	a5,512
	.loc 1 99 37
	sw	s1,4(s0)
	.loc 1 97 41
	sh	a5,0(s0)
	.loc 1 99 9 is_stmt 1
	.loc 1 102 9
.LBE9:
.LBE8:
	.loc 1 104 1 is_stmt 0
	lw	s0,8(sp)
	.cfi_remember_state
	.cfi_restore 8
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s1,4(sp)
	.cfi_restore 9
.LBB12:
.LBB10:
	.loc 1 102 9
	li	a0,500
.LBE10:
.LBE12:
	.loc 1 104 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
.LBB13:
.LBB11:
	.loc 1 102 9
	tail	m_start_timer
.LVL17:
.L16:
	.cfi_restore_state
.LBE11:
.LBE13:
	.loc 1 104 1
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s0,8(sp)
	.cfi_restore 8
	lw	s1,4(sp)
	.cfi_restore 9
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE9:
	.size	set_led_stt_update_ota, .-set_led_stt_update_ota
	.section	.text.set_led_stt_smart_config,"ax",@progbits
	.align	1
	.type	set_led_stt_smart_config, @function
set_led_stt_smart_config:
.LFB7:
	.loc 1 46 1 is_stmt 1
	.cfi_startproc
	.loc 1 47 5
	.loc 1 46 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	.cfi_offset 8, -8
	.loc 1 47 22
	lui	s0,%hi(.LANCHOR1)
	addi	s0,s0,%lo(.LANCHOR1)
	.loc 1 47 8
	lw	a5,4(s0)
	.loc 1 46 1
	sw	s1,4(sp)
	sw	ra,12(sp)
	.cfi_offset 9, -12
	.cfi_offset 1, -4
	.loc 1 47 8
	li	s1,2
	beq	a5,s1,.L19
.LBB16:
.LBB17:
	.loc 1 48 9 is_stmt 1
	.loc 1 50 9
	call	m_stop_timer
.LVL18:
	.loc 1 53 9
	.loc 1 53 28 is_stmt 0
	li	a5,13107200
	addi	a5,a5,200
	lui	a4,%hi(.LANCHOR2)
	sw	a5,%lo(.LANCHOR2)(a4)
	.loc 1 54 9 is_stmt 1
	.loc 1 55 9
	.loc 1 54 41 is_stmt 0
	li	a5,512
	.loc 1 56 37
	sw	s1,4(s0)
	.loc 1 54 41
	sh	a5,0(s0)
	.loc 1 56 9 is_stmt 1
	.loc 1 59 9
.LBE17:
.LBE16:
	.loc 1 61 1 is_stmt 0
	lw	s0,8(sp)
	.cfi_remember_state
	.cfi_restore 8
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s1,4(sp)
	.cfi_restore 9
.LBB20:
.LBB18:
	.loc 1 59 9
	li	a0,200
.LBE18:
.LBE20:
	.loc 1 61 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
.LBB21:
.LBB19:
	.loc 1 59 9
	tail	m_start_timer
.LVL19:
.L19:
	.cfi_restore_state
.LBE19:
.LBE21:
	.loc 1 61 1
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s0,8(sp)
	.cfi_restore 8
	lw	s1,4(sp)
	.cfi_restore 9
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE7:
	.size	set_led_stt_smart_config, .-set_led_stt_smart_config
	.section	.text.set_led_stt_ap_config,"ax",@progbits
	.align	1
	.type	set_led_stt_ap_config, @function
set_led_stt_ap_config:
.LFB8:
	.loc 1 64 1 is_stmt 1
	.cfi_startproc
	.loc 1 65 4
	.loc 1 64 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	.cfi_offset 8, -8
	.loc 1 65 21
	lui	s0,%hi(.LANCHOR1)
	addi	s0,s0,%lo(.LANCHOR1)
	.loc 1 65 7
	lw	a5,4(s0)
	.loc 1 64 1
	sw	s1,4(sp)
	sw	ra,12(sp)
	.cfi_offset 9, -12
	.cfi_offset 1, -4
	.loc 1 65 7
	li	s1,3
	beq	a5,s1,.L22
.LBB24:
.LBB25:
	.loc 1 66 9 is_stmt 1
	.loc 1 68 9
	call	m_stop_timer
.LVL20:
	.loc 1 71 9
	lw	a5,16(s0)
	jalr	a5
.LVL21:
	.loc 1 74 9
	.loc 1 75 9
	.loc 1 74 28 is_stmt 0
	li	a4,13107200
	lui	a5,%hi(.LANCHOR2)
	addi	a5,a5,%lo(.LANCHOR2)
	addi	a3,a4,500
	.loc 1 75 49
	addi	a4,a4,200
	.loc 1 74 28
	sw	a3,0(a5)
	.loc 1 75 49
	sw	a4,4(a5)
	.loc 1 76 9 is_stmt 1
	.loc 1 77 9
	.loc 1 76 41 is_stmt 0
	li	a5,1024
	.loc 1 78 37
	sw	s1,4(s0)
	.loc 1 76 41
	sh	a5,0(s0)
	.loc 1 78 9 is_stmt 1
	.loc 1 81 9
.LBE25:
.LBE24:
	.loc 1 83 1 is_stmt 0
	lw	s0,8(sp)
	.cfi_remember_state
	.cfi_restore 8
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s1,4(sp)
	.cfi_restore 9
.LBB28:
.LBB26:
	.loc 1 81 9
	li	a0,500
.LBE26:
.LBE28:
	.loc 1 83 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
.LBB29:
.LBB27:
	.loc 1 81 9
	tail	m_start_timer
.LVL22:
.L22:
	.cfi_restore_state
.LBE27:
.LBE29:
	.loc 1 83 1
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s0,8(sp)
	.cfi_restore 8
	lw	s1,4(sp)
	.cfi_restore 9
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE8:
	.size	set_led_stt_ap_config, .-set_led_stt_ap_config
	.section	.text.set_led_stt_not_config,"ax",@progbits
	.align	1
	.type	set_led_stt_not_config, @function
set_led_stt_not_config:
.LFB10:
	.loc 1 108 1 is_stmt 1
	.cfi_startproc
	.loc 1 109 5
	.loc 1 109 22 is_stmt 0
	lui	a4,%hi(device_type)
	lbu	a5,%lo(device_type)(a4)
	.loc 1 108 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	sw	s0,8(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	lui	s0,%hi(.LANCHOR1)
	sw	s1,4(sp)
	.cfi_offset 9, -12
	.loc 1 109 22
	andi	a5,a5,0xff
	addi	s0,s0,%lo(.LANCHOR1)
	.loc 1 109 8
	beq	a5,zero,.L26
	.loc 1 109 48 discriminator 1
	lbu	a5,%lo(device_type)(a4)
	.loc 1 109 33 discriminator 1
	li	a4,2
	.loc 1 109 48 discriminator 1
	andi	a5,a5,0xff
	.loc 1 109 33 discriminator 1
	bne	a5,a4,.L27
.L26:
	.loc 1 110 9 is_stmt 1
	.loc 1 110 12 is_stmt 0
	lw	a5,4(s0)
	li	s1,4
	beq	a5,s1,.L25
	.loc 1 111 13 is_stmt 1
	.loc 1 113 13
	call	m_stop_timer
.LVL23:
	.loc 1 114 13
	lw	a5,12(s0)
	jalr	a5
.LVL24:
	.loc 1 115 13
	.loc 1 115 41 is_stmt 0
	sw	s1,4(s0)
.L25:
	.loc 1 134 1
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
.L27:
	.cfi_restore_state
.LBB32:
.LBB33:
	.loc 1 119 9 is_stmt 1
	call	m_stop_timer
.LVL25:
	.loc 1 122 9
	lw	a5,16(s0)
	jalr	a5
.LVL26:
	.loc 1 125 9
	.loc 1 126 9
	.loc 1 125 28 is_stmt 0
	li	a5,13111296
	addi	a5,a5,904
	lui	a4,%hi(.LANCHOR2)
	sw	a5,%lo(.LANCHOR2)(a4)
	.loc 1 127 9 is_stmt 1
	.loc 1 128 9
	.loc 1 127 41 is_stmt 0
	li	a5,512
	sh	a5,0(s0)
	.loc 1 129 9 is_stmt 1
	.loc 1 129 37 is_stmt 0
	li	a5,6
	sw	a5,4(s0)
	.loc 1 132 9 is_stmt 1
.LBE33:
.LBE32:
	.loc 1 134 1 is_stmt 0
	lw	s0,8(sp)
	.cfi_restore 8
	lw	ra,12(sp)
	.cfi_restore 1
	lw	s1,4(sp)
	.cfi_restore 9
.LBB37:
.LBB34:
	.loc 1 132 9
	li	a0,4096
.LBE34:
.LBE37:
	.loc 1 134 1
.LBB38:
.LBB35:
	.loc 1 132 9
	addi	a0,a0,904
.LBE35:
.LBE38:
	.loc 1 134 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
.LBB39:
.LBB36:
	.loc 1 132 9
	tail	m_start_timer
.LVL27:
.LBE36:
.LBE39:
	.cfi_endproc
.LFE10:
	.size	set_led_stt_not_config, .-set_led_stt_not_config
	.section	.rodata.m_led_stt_init.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.string	"timer_handler_led_stt"
	.section	.text.m_led_stt_init,"ax",@progbits
	.align	1
	.globl	m_led_stt_init
	.type	m_led_stt_init, @function
m_led_stt_init:
.LFB12:
	.loc 1 160 1 is_stmt 1
	.cfi_startproc
	.loc 1 162 5
	.loc 1 160 1 is_stmt 0
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	.loc 1 162 33
	lui	a5,%hi(.LANCHOR1)
	.loc 1 160 1
	sw	ra,12(sp)
	.cfi_offset 1, -4
	.loc 1 162 33
	addi	a5,a5,%lo(.LANCHOR1)
	li	a4,-1
	sw	a4,4(a5)
	.loc 1 163 5 is_stmt 1
	.loc 1 164 5
	.loc 1 164 36 is_stmt 0
	lui	a4,%hi(hw_led_stt_cfg)
	addi	a4,a4,%lo(hw_led_stt_cfg)
	sw	a4,8(a5)
	.loc 1 165 5 is_stmt 1
	.loc 1 165 38 is_stmt 0
	lui	a4,%hi(hw_led_stt_on)
	addi	a4,a4,%lo(hw_led_stt_on)
	sw	a4,12(a5)
	.loc 1 166 5 is_stmt 1
	.loc 1 166 39 is_stmt 0
	lui	a4,%hi(hw_led_stt_off)
	addi	a4,a4,%lo(hw_led_stt_off)
	sw	a4,16(a5)
	.loc 1 167 5 is_stmt 1
	.loc 1 167 38 is_stmt 0
	lui	a4,%hi(hw_toggle_led_stt)
	addi	a4,a4,%lo(hw_toggle_led_stt)
	sw	a4,20(a5)
	.loc 1 169 5 is_stmt 1
	.loc 1 169 37 is_stmt 0
	li	a4,512
	sh	a4,0(a5)
	.loc 1 170 5 is_stmt 1
	.loc 1 170 46 is_stmt 0
	lui	a4,%hi(set_led_stt_not_config)
	addi	a4,a4,%lo(set_led_stt_not_config)
	sw	a4,24(a5)
	.loc 1 171 5 is_stmt 1
	.loc 1 171 45 is_stmt 0
	lui	a4,%hi(set_led_stt_ap_config)
	addi	a4,a4,%lo(set_led_stt_ap_config)
	sw	a4,28(a5)
	.loc 1 172 5 is_stmt 1
	.loc 1 172 48 is_stmt 0
	lui	a4,%hi(set_led_stt_smart_config)
	addi	a4,a4,%lo(set_led_stt_smart_config)
	sw	a4,32(a5)
	.loc 1 173 5 is_stmt 1
	.loc 1 173 49 is_stmt 0
	lui	a4,%hi(set_led_stt_device_active)
	addi	a4,a4,%lo(set_led_stt_device_active)
	sw	a4,40(a5)
	.loc 1 174 5 is_stmt 1
	.loc 1 174 46 is_stmt 0
	lui	a4,%hi(set_led_stt_update_ota)
	addi	a4,a4,%lo(set_led_stt_update_ota)
	sw	a4,36(a5)
	.loc 1 177 5 is_stmt 1
	call	hw_led_stt_cfg
.LVL28:
	.loc 1 180 5
	.loc 1 180 15 is_stmt 0
	lui	a4,%hi(vTimerCallback)
	lui	a0,%hi(.LC0)
	addi	a4,a4,%lo(vTimerCallback)
	li	a3,0
	li	a2,1
	li	a1,1
	addi	a0,a0,%lo(.LC0)
	call	xTimerCreate
.LVL29:
	.loc 1 180 13
	lui	a5,%hi(.LANCHOR0)
	sw	a0,%lo(.LANCHOR0)(a5)
	.loc 1 187 5 is_stmt 1
	lui	a5,%hi(.LANCHOR2)
	lhu	a0,%lo(.LANCHOR2)(a5)
	call	m_start_timer
.LVL30:
	.loc 1 190 5
	.loc 1 191 1 is_stmt 0
	lw	ra,12(sp)
	.cfi_restore 1
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	.loc 1 190 5
	tail	set_led_stt_not_config
.LVL31:
	.cfi_endproc
.LFE12:
	.size	m_led_stt_init, .-m_led_stt_init
	.section	.text.m_led_stt_get_instance,"ax",@progbits
	.align	1
	.globl	m_led_stt_get_instance
	.type	m_led_stt_get_instance, @function
m_led_stt_get_instance:
.LFB13:
	.loc 1 194 1 is_stmt 1
	.cfi_startproc
.LVL32:
	.loc 1 195 5
	.loc 1 195 12 is_stmt 0
	lui	a1,%hi(.LANCHOR1)
	.loc 1 194 1
	addi	sp,sp,-16
	.cfi_def_cfa_offset 16
	.loc 1 195 12
	li	a2,44
	addi	a1,a1,%lo(.LANCHOR1)
	.loc 1 194 1
	sw	s0,8(sp)
	sw	ra,12(sp)
	.cfi_offset 8, -8
	.cfi_offset 1, -4
	.loc 1 194 1
	mv	s0,a0
	.loc 1 195 12
	call	memcpy
.LVL33:
	.loc 1 196 1
	lw	ra,12(sp)
	.cfi_restore 1
	mv	a0,s0
	lw	s0,8(sp)
	.cfi_restore 8
.LVL34:
	addi	sp,sp,16
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE13:
	.size	m_led_stt_get_instance, .-m_led_stt_get_instance
	.section	.bss.s_led_stt_obj,"aw",@nobits
	.align	2
	.set	.LANCHOR1,. + 0
	.type	s_led_stt_obj, @object
	.size	s_led_stt_obj, 44
s_led_stt_obj:
	.zero	44
	.section	.data.timeline_status,"aw"
	.align	2
	.set	.LANCHOR2,. + 0
	.type	timeline_status, @object
	.size	timeline_status, 16
timeline_status:
	.half	200
	.half	200
	.half	200
	.half	200
	.half	200
	.half	200
	.half	200
	.half	200
	.section	.sbss.xTimers,"aw",@nobits
	.align	2
	.set	.LANCHOR0,. + 0
	.type	xTimers, @object
	.size	xTimers, 4
xTimers:
	.zero	4
	.text
.Letext0:
	.file 2 "/home/dungnt98/Ai-Thinker-WB2/toolchain/riscv/Linux/lib/gcc/riscv64-unknown-elf/10.2.0/include/stdint-gcc.h"
	.file 3 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_led_stt.h"
	.file 4 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/app_config/system_typedef.h"
	.file 5 "/home/dungnt98/Ai-Thinker-WB2/components/platform/soc/bl602/freertos_riscv_ram/portable/GCC/RISC-V/portmacro.h"
	.file 6 "/home/dungnt98/Ai-Thinker-WB2/components/platform/soc/bl602/freertos_riscv_ram/config/timers.h"
	.file 7 "/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/hardware/hardware.h"
	.file 8 "/home/dungnt98/Ai-Thinker-WB2/components/platform/soc/bl602/freertos_riscv_ram/config/task.h"
	.file 9 "<built-in>"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x5e7
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.byte	0x1
	.4byte	.LASF68
	.byte	0xc
	.4byte	.LASF69
	.4byte	.LASF70
	.4byte	.Ldebug_ranges0+0x88
	.4byte	0
	.4byte	.Ldebug_line0
	.byte	0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF0
	.byte	0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF1
	.byte	0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF2
	.byte	0x3
	.4byte	.LASF5
	.byte	0x2
	.byte	0x28
	.byte	0x12
	.4byte	0x46
	.byte	0x2
	.byte	0x4
	.byte	0x5
	.4byte	.LASF3
	.byte	0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF4
	.byte	0x3
	.4byte	.LASF6
	.byte	0x2
	.byte	0x2e
	.byte	0x17
	.4byte	0x65
	.byte	0x4
	.4byte	0x54
	.byte	0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF7
	.byte	0x3
	.4byte	.LASF8
	.byte	0x2
	.byte	0x31
	.byte	0x1c
	.4byte	0x78
	.byte	0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF9
	.byte	0x3
	.4byte	.LASF10
	.byte	0x2
	.byte	0x34
	.byte	0x1b
	.4byte	0x8b
	.byte	0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF11
	.byte	0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF12
	.byte	0x5
	.byte	0x4
	.byte	0x5
	.string	"int"
	.byte	0x6
	.byte	0x5
	.byte	0x4
	.4byte	0x99
	.byte	0x3
	.byte	0x8
	.byte	0xe
	.4byte	0xdf
	.byte	0x7
	.4byte	.LASF13
	.byte	0x7f
	.byte	0x8
	.4byte	.LASF14
	.byte	0
	.byte	0x8
	.4byte	.LASF15
	.byte	0x1
	.byte	0x8
	.4byte	.LASF16
	.byte	0x2
	.byte	0x8
	.4byte	.LASF17
	.byte	0x3
	.byte	0x8
	.4byte	.LASF18
	.byte	0x4
	.byte	0x8
	.4byte	.LASF19
	.byte	0x5
	.byte	0x8
	.4byte	.LASF20
	.byte	0x6
	.byte	0
	.byte	0x3
	.4byte	.LASF21
	.byte	0x3
	.byte	0x11
	.byte	0x3
	.4byte	0xa0
	.byte	0x3
	.4byte	.LASF22
	.byte	0x3
	.byte	0x13
	.byte	0x10
	.4byte	0xf7
	.byte	0x9
	.byte	0x4
	.4byte	0xfd
	.byte	0xa
	.byte	0x3
	.4byte	.LASF23
	.byte	0x3
	.byte	0x14
	.byte	0x10
	.4byte	0xf7
	.byte	0x3
	.4byte	.LASF24
	.byte	0x3
	.byte	0x15
	.byte	0x10
	.4byte	0xf7
	.byte	0x3
	.4byte	.LASF25
	.byte	0x3
	.byte	0x16
	.byte	0x10
	.4byte	0xf7
	.byte	0x3
	.4byte	.LASF26
	.byte	0x3
	.byte	0x18
	.byte	0x10
	.4byte	0xf7
	.byte	0x3
	.4byte	.LASF27
	.byte	0x3
	.byte	0x19
	.byte	0x10
	.4byte	0xf7
	.byte	0x3
	.4byte	.LASF28
	.byte	0x3
	.byte	0x1a
	.byte	0x10
	.4byte	0xf7
	.byte	0x3
	.4byte	.LASF29
	.byte	0x3
	.byte	0x1b
	.byte	0x10
	.4byte	0xf7
	.byte	0x3
	.4byte	.LASF30
	.byte	0x3
	.byte	0x1c
	.byte	0x10
	.4byte	0xf7
	.byte	0xb
	.byte	0x2c
	.byte	0x3
	.byte	0x1e
	.byte	0x9
	.4byte	0x204
	.byte	0xc
	.4byte	.LASF31
	.byte	0x3
	.byte	0x20
	.byte	0xc
	.4byte	0x54
	.byte	0
	.byte	0xc
	.4byte	.LASF32
	.byte	0x3
	.byte	0x21
	.byte	0xc
	.4byte	0x54
	.byte	0x1
	.byte	0xc
	.4byte	.LASF33
	.byte	0x3
	.byte	0x22
	.byte	0x13
	.4byte	0xdf
	.byte	0x4
	.byte	0xc
	.4byte	.LASF34
	.byte	0x3
	.byte	0x25
	.byte	0x15
	.4byte	0x116
	.byte	0x8
	.byte	0xc
	.4byte	.LASF35
	.byte	0x3
	.byte	0x26
	.byte	0x14
	.4byte	0xeb
	.byte	0xc
	.byte	0xc
	.4byte	.LASF36
	.byte	0x3
	.byte	0x27
	.byte	0x16
	.4byte	0xfe
	.byte	0x10
	.byte	0xc
	.4byte	.LASF37
	.byte	0x3
	.byte	0x28
	.byte	0x17
	.4byte	0x10a
	.byte	0x14
	.byte	0xc
	.4byte	.LASF38
	.byte	0x3
	.byte	0x2b
	.byte	0x1b
	.4byte	0x122
	.byte	0x18
	.byte	0xc
	.4byte	.LASF39
	.byte	0x3
	.byte	0x2c
	.byte	0x1a
	.4byte	0x12e
	.byte	0x1c
	.byte	0xc
	.4byte	.LASF40
	.byte	0x3
	.byte	0x2d
	.byte	0x1d
	.4byte	0x13a
	.byte	0x20
	.byte	0xc
	.4byte	.LASF41
	.byte	0x3
	.byte	0x2e
	.byte	0x1b
	.4byte	0x146
	.byte	0x24
	.byte	0xc
	.4byte	.LASF42
	.byte	0x3
	.byte	0x2f
	.byte	0x1e
	.4byte	0x152
	.byte	0x28
	.byte	0
	.byte	0x3
	.4byte	.LASF43
	.byte	0x3
	.byte	0x31
	.byte	0x3
	.4byte	0x15e
	.byte	0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF44
	.byte	0xd
	.4byte	.LASF71
	.byte	0x7
	.byte	0x20
	.byte	0x19
	.4byte	0x60
	.byte	0x6
	.byte	0x7
	.byte	0x4
	.4byte	0x25
	.byte	0x4
	.byte	0x27
	.byte	0xe
	.4byte	0x244
	.byte	0x8
	.4byte	.LASF45
	.byte	0
	.byte	0x8
	.4byte	.LASF46
	.byte	0x1
	.byte	0x8
	.4byte	.LASF47
	.byte	0x2
	.byte	0
	.byte	0x3
	.4byte	.LASF48
	.byte	0x5
	.byte	0x3f
	.byte	0x11
	.4byte	0x3a
	.byte	0x3
	.4byte	.LASF49
	.byte	0x5
	.byte	0x41
	.byte	0x12
	.4byte	0x7f
	.byte	0x3
	.4byte	.LASF50
	.byte	0x6
	.byte	0x4d
	.byte	0x22
	.4byte	0x268
	.byte	0x9
	.byte	0x4
	.4byte	0x26e
	.byte	0xe
	.4byte	.LASF72
	.byte	0xf
	.4byte	.LASF51
	.byte	0x1
	.byte	0xf
	.byte	0x14
	.4byte	0x204
	.byte	0x5
	.byte	0x3
	.4byte	s_led_stt_obj
	.byte	0xf
	.4byte	.LASF52
	.byte	0x1
	.byte	0x10
	.byte	0x16
	.4byte	0x25c
	.byte	0x5
	.byte	0x3
	.4byte	xTimers
	.byte	0x10
	.4byte	0x6c
	.4byte	0x2a7
	.byte	0x11
	.4byte	0x25
	.byte	0x7
	.byte	0
	.byte	0xf
	.4byte	.LASF53
	.byte	0x1
	.byte	0x11
	.byte	0x11
	.4byte	0x297
	.byte	0x5
	.byte	0x3
	.4byte	timeline_status
	.byte	0x12
	.4byte	.LASF73
	.byte	0x1
	.byte	0xc1
	.byte	0xd
	.4byte	0x204
	.4byte	.LFB13
	.4byte	.LFE13-.LFB13
	.byte	0x1
	.byte	0x9c
	.4byte	0x2f3
	.byte	0x13
	.4byte	.LVL33
	.4byte	0x5ad
	.byte	0x14
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x78
	.byte	0
	.byte	0x14
	.byte	0x1
	.byte	0x5b
	.byte	0x5
	.byte	0x3
	.4byte	.LANCHOR1
	.byte	0x14
	.byte	0x1
	.byte	0x5c
	.byte	0x2
	.byte	0x8
	.byte	0x2c
	.byte	0
	.byte	0
	.byte	0x15
	.4byte	.LASF74
	.byte	0x1
	.byte	0x9f
	.byte	0x6
	.4byte	.LFB12
	.4byte	.LFE12-.LFB12
	.byte	0x1
	.byte	0x9c
	.4byte	0x354
	.byte	0x16
	.4byte	.LVL28
	.4byte	0x5b8
	.byte	0x17
	.4byte	.LVL29
	.4byte	0x5c4
	.4byte	0x341
	.byte	0x14
	.byte	0x1
	.byte	0x5a
	.byte	0x5
	.byte	0x3
	.4byte	.LC0
	.byte	0x14
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x31
	.byte	0x14
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x31
	.byte	0x14
	.byte	0x1
	.byte	0x5d
	.byte	0x1
	.byte	0x30
	.byte	0x14
	.byte	0x1
	.byte	0x5e
	.byte	0x5
	.byte	0x3
	.4byte	vTimerCallback
	.byte	0
	.byte	0x16
	.4byte	.LVL30
	.4byte	0x3b6
	.byte	0x18
	.4byte	.LVL31
	.4byte	0x35d
	.byte	0
	.byte	0x19
	.4byte	.LASF54
	.byte	0x1
	.byte	0x88
	.byte	0xd
	.byte	0x1
	.byte	0x19
	.4byte	.LASF55
	.byte	0x1
	.byte	0x6b
	.byte	0xd
	.byte	0x1
	.byte	0x19
	.4byte	.LASF56
	.byte	0x1
	.byte	0x55
	.byte	0xd
	.byte	0x1
	.byte	0x19
	.4byte	.LASF57
	.byte	0x1
	.byte	0x3f
	.byte	0xd
	.byte	0x1
	.byte	0x19
	.4byte	.LASF58
	.byte	0x1
	.byte	0x2d
	.byte	0xd
	.byte	0x1
	.byte	0x1a
	.4byte	.LASF59
	.byte	0x1
	.byte	0x26
	.byte	0xd
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.byte	0x1
	.byte	0x9c
	.4byte	0x3b6
	.byte	0x1b
	.4byte	.LVL0
	.4byte	0x5d0
	.byte	0x14
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x33
	.byte	0x14
	.byte	0x1
	.byte	0x5c
	.byte	0x1
	.byte	0x30
	.byte	0x14
	.byte	0x1
	.byte	0x5d
	.byte	0x1
	.byte	0x30
	.byte	0x14
	.byte	0x1
	.byte	0x5e
	.byte	0x1
	.byte	0x3a
	.byte	0
	.byte	0
	.byte	0x1a
	.4byte	.LASF60
	.byte	0x1
	.byte	0x1e
	.byte	0xd
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.byte	0x1
	.byte	0x9c
	.4byte	0x431
	.byte	0x1c
	.4byte	.LASF62
	.byte	0x1
	.byte	0x1e
	.byte	0x24
	.4byte	0x7f
	.4byte	.LLST0
	.byte	0x17
	.4byte	.LVL4
	.4byte	0x5d0
	.4byte	0x40e
	.byte	0x14
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x34
	.byte	0x14
	.byte	0x1
	.byte	0x5c
	.byte	0x11
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0xa
	.2byte	0x3e8
	.byte	0x1e
	.byte	0xf7
	.byte	0x25
	.byte	0xa
	.2byte	0x3e8
	.byte	0xf7
	.byte	0x25
	.byte	0x1b
	.byte	0xf7
	.byte	0
	.byte	0x14
	.byte	0x1
	.byte	0x5d
	.byte	0x1
	.byte	0x30
	.byte	0x14
	.byte	0x1
	.byte	0x5e
	.byte	0x1
	.byte	0x3a
	.byte	0
	.byte	0x16
	.4byte	.LVL5
	.4byte	0x5dd
	.byte	0x1b
	.4byte	.LVL6
	.4byte	0x5d0
	.byte	0x14
	.byte	0x1
	.byte	0x5b
	.byte	0x1
	.byte	0x31
	.byte	0x14
	.byte	0x1
	.byte	0x5d
	.byte	0x1
	.byte	0x30
	.byte	0x14
	.byte	0x1
	.byte	0x5e
	.byte	0x1
	.byte	0x3a
	.byte	0
	.byte	0
	.byte	0x1d
	.4byte	.LASF61
	.byte	0x1
	.byte	0x16
	.byte	0xd
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.byte	0x1
	.byte	0x9c
	.4byte	0x46a
	.byte	0x1c
	.4byte	.LASF63
	.byte	0x1
	.byte	0x16
	.byte	0x2a
	.4byte	0x25c
	.4byte	.LLST1
	.byte	0x16
	.4byte	.LVL9
	.4byte	0x381
	.byte	0x18
	.4byte	.LVL10
	.4byte	0x3b6
	.byte	0
	.byte	0x1e
	.4byte	0x354
	.4byte	.LFB11
	.4byte	.LFE11-.LFB11
	.byte	0x1
	.byte	0x9c
	.4byte	0x4a5
	.byte	0x1f
	.4byte	0x354
	.4byte	.LBB4
	.4byte	.LBE4-.LBB4
	.byte	0x1
	.byte	0x88
	.byte	0xd
	.4byte	0x49b
	.byte	0x16
	.4byte	.LVL13
	.4byte	0x381
	.byte	0
	.byte	0x16
	.4byte	.LVL11
	.4byte	0x381
	.byte	0
	.byte	0x1e
	.4byte	0x366
	.4byte	.LFB9
	.4byte	.LFE9-.LFB9
	.byte	0x1
	.byte	0x9c
	.4byte	0x4e4
	.byte	0x20
	.4byte	0x366
	.4byte	.LBB8
	.4byte	.Ldebug_ranges0+0
	.byte	0x1
	.byte	0x55
	.byte	0xd
	.byte	0x16
	.4byte	.LVL15
	.4byte	0x381
	.byte	0x1b
	.4byte	.LVL17
	.4byte	0x3b6
	.byte	0x14
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0xa
	.2byte	0x1f4
	.byte	0
	.byte	0
	.byte	0
	.byte	0x21
	.4byte	0x378
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.byte	0x1
	.byte	0x9c
	.4byte	0x522
	.byte	0x20
	.4byte	0x378
	.4byte	.LBB16
	.4byte	.Ldebug_ranges0+0x20
	.byte	0x1
	.byte	0x2d
	.byte	0xd
	.byte	0x16
	.4byte	.LVL18
	.4byte	0x381
	.byte	0x1b
	.4byte	.LVL19
	.4byte	0x3b6
	.byte	0x14
	.byte	0x1
	.byte	0x5a
	.byte	0x2
	.byte	0x8
	.byte	0xc8
	.byte	0
	.byte	0
	.byte	0
	.byte	0x1e
	.4byte	0x36f
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.byte	0x1
	.byte	0x9c
	.4byte	0x561
	.byte	0x20
	.4byte	0x36f
	.4byte	.LBB24
	.4byte	.Ldebug_ranges0+0x40
	.byte	0x1
	.byte	0x3f
	.byte	0xd
	.byte	0x16
	.4byte	.LVL20
	.4byte	0x381
	.byte	0x1b
	.4byte	.LVL22
	.4byte	0x3b6
	.byte	0x14
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0xa
	.2byte	0x1f4
	.byte	0
	.byte	0
	.byte	0
	.byte	0x1e
	.4byte	0x35d
	.4byte	.LFB10
	.4byte	.LFE10-.LFB10
	.byte	0x1
	.byte	0x9c
	.4byte	0x5ad
	.byte	0x22
	.4byte	0x35d
	.4byte	.LBB32
	.4byte	.Ldebug_ranges0+0x60
	.byte	0x1
	.byte	0x6b
	.byte	0xd
	.4byte	0x5a3
	.byte	0x16
	.4byte	.LVL25
	.4byte	0x381
	.byte	0x1b
	.4byte	.LVL27
	.4byte	0x3b6
	.byte	0x14
	.byte	0x1
	.byte	0x5a
	.byte	0x3
	.byte	0xa
	.2byte	0x1388
	.byte	0
	.byte	0
	.byte	0x16
	.4byte	.LVL23
	.4byte	0x381
	.byte	0
	.byte	0x23
	.4byte	.LASF75
	.4byte	.LASF76
	.byte	0x9
	.byte	0
	.byte	0x24
	.4byte	.LASF64
	.4byte	.LASF64
	.byte	0x7
	.byte	0x2c
	.byte	0x6
	.byte	0x24
	.4byte	.LASF65
	.4byte	.LASF65
	.byte	0x6
	.byte	0xe4
	.byte	0x10
	.byte	0x25
	.4byte	.LASF66
	.4byte	.LASF66
	.byte	0x6
	.2byte	0x502
	.byte	0xc
	.byte	0x25
	.4byte	.LASF67
	.4byte	.LASF67
	.byte	0x8
	.2byte	0x547
	.byte	0xc
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
	.byte	0x7
	.byte	0x28
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x1c
	.byte	0xd
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
	.byte	0
	.byte	0x27
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0xb
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
	.byte	0xc
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
	.byte	0x3f
	.byte	0x19
	.byte	0x3c
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0xe
	.byte	0x13
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3c
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0xf
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
	.byte	0x10
	.byte	0x1
	.byte	0x1
	.byte	0x49
	.byte	0x13
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x11
	.byte	0x21
	.byte	0
	.byte	0x49
	.byte	0x13
	.byte	0x2f
	.byte	0xb
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
	.byte	0x89,0x82,0x1
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x31
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
	.byte	0x89,0x82,0x1
	.byte	0
	.byte	0x11
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x17
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
	.byte	0x18
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
	.byte	0x19
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
	.byte	0x1a
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
	.byte	0x1b
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
	.byte	0x1c
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
	.byte	0x1e
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
	.byte	0x96,0x42
	.byte	0x19
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x1f
	.byte	0x1d
	.byte	0x1
	.byte	0x31
	.byte	0x13
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x6
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
	.byte	0x20
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
	.byte	0x23
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
	.byte	0x39
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x25
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
.LLST0:
	.4byte	.LVL1
	.4byte	.LVL2
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL2
	.4byte	.LVL3
	.2byte	0x1
	.byte	0x5f
	.4byte	.LVL3
	.4byte	.LVL6
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	.LVL6
	.4byte	.LFE5
	.2byte	0x1
	.byte	0x5f
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LVL7
	.4byte	.LVL8-1
	.2byte	0x1
	.byte	0x5a
	.4byte	.LVL8-1
	.4byte	.LFE4
	.2byte	0x4
	.byte	0xf3
	.byte	0x1
	.byte	0x5a
	.byte	0x9f
	.4byte	0
	.4byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x64
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.LFB6
	.4byte	.LFE6-.LFB6
	.4byte	.LFB5
	.4byte	.LFE5-.LFB5
	.4byte	.LFB4
	.4byte	.LFE4-.LFB4
	.4byte	.LFB11
	.4byte	.LFE11-.LFB11
	.4byte	.LFB9
	.4byte	.LFE9-.LFB9
	.4byte	.LFB7
	.4byte	.LFE7-.LFB7
	.4byte	.LFB8
	.4byte	.LFE8-.LFB8
	.4byte	.LFB10
	.4byte	.LFE10-.LFB10
	.4byte	.LFB12
	.4byte	.LFE12-.LFB12
	.4byte	.LFB13
	.4byte	.LFE13-.LFB13
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.4byte	.LBB8
	.4byte	.LBE8
	.4byte	.LBB12
	.4byte	.LBE12
	.4byte	.LBB13
	.4byte	.LBE13
	.4byte	0
	.4byte	0
	.4byte	.LBB16
	.4byte	.LBE16
	.4byte	.LBB20
	.4byte	.LBE20
	.4byte	.LBB21
	.4byte	.LBE21
	.4byte	0
	.4byte	0
	.4byte	.LBB24
	.4byte	.LBE24
	.4byte	.LBB28
	.4byte	.LBE28
	.4byte	.LBB29
	.4byte	.LBE29
	.4byte	0
	.4byte	0
	.4byte	.LBB32
	.4byte	.LBE32
	.4byte	.LBB37
	.4byte	.LBE37
	.4byte	.LBB38
	.4byte	.LBE38
	.4byte	.LBB39
	.4byte	.LBE39
	.4byte	0
	.4byte	0
	.4byte	.LFB6
	.4byte	.LFE6
	.4byte	.LFB5
	.4byte	.LFE5
	.4byte	.LFB4
	.4byte	.LFE4
	.4byte	.LFB11
	.4byte	.LFE11
	.4byte	.LFB9
	.4byte	.LFE9
	.4byte	.LFB7
	.4byte	.LFE7
	.4byte	.LFB8
	.4byte	.LFE8
	.4byte	.LFB10
	.4byte	.LFE10
	.4byte	.LFB12
	.4byte	.LFE12
	.4byte	.LFB13
	.4byte	.LFE13
	.4byte	0
	.4byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF69:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/components/middle/gpio/m_led_stt.c"
.LASF55:
	.string	"set_led_stt_not_config"
.LASF57:
	.string	"set_led_stt_ap_config"
.LASF76:
	.string	"__builtin_memcpy"
.LASF23:
	.string	"p_reset_led_stt_t"
.LASF22:
	.string	"p_set_led_stt_t"
.LASF71:
	.string	"device_type"
.LASF51:
	.string	"s_led_stt_obj"
.LASF66:
	.string	"xTimerGenericCommand"
.LASF50:
	.string	"TimerHandle_t"
.LASF2:
	.string	"short int"
.LASF15:
	.string	"LED_STYLE_NORMAL"
.LASF21:
	.string	"led_stt_type_t"
.LASF30:
	.string	"p_led_stt_device_active_t"
.LASF59:
	.string	"m_stop_timer"
.LASF4:
	.string	"long long int"
.LASF75:
	.string	"memcpy"
.LASF46:
	.string	"LHSWITCH"
.LASF39:
	.string	"_set_led_stt_ap_config_cb"
.LASF6:
	.string	"uint8_t"
.LASF40:
	.string	"_set_led_stt_smart_config_cb"
.LASF35:
	.string	"_set_led_stt_on_cb"
.LASF65:
	.string	"xTimerCreate"
.LASF43:
	.string	"m_led_stt_t"
.LASF38:
	.string	"_set_led_stt_not_config_cb"
.LASF41:
	.string	"_set_led_stt_update_ota_cb"
.LASF32:
	.string	"_max_number_handle"
.LASF49:
	.string	"TickType_t"
.LASF58:
	.string	"set_led_stt_smart_config"
.LASF61:
	.string	"vTimerCallback"
.LASF47:
	.string	"WSDATIC"
.LASF48:
	.string	"BaseType_t"
.LASF67:
	.string	"xTaskGetTickCount"
.LASF24:
	.string	"p_toggle_led_stt_t"
.LASF73:
	.string	"m_led_stt_get_instance"
.LASF19:
	.string	"LED_STYLE_OFF"
.LASF7:
	.string	"unsigned char"
.LASF63:
	.string	"xTimer"
.LASF54:
	.string	"set_led_stt_device_active"
.LASF25:
	.string	"p_init_led_stt_t"
.LASF26:
	.string	"p_led_stt_not_config_t"
.LASF29:
	.string	"p_led_stt_update_ota_t"
.LASF1:
	.string	"signed char"
.LASF45:
	.string	"WSWITCH"
.LASF12:
	.string	"long long unsigned int"
.LASF10:
	.string	"uint32_t"
.LASF74:
	.string	"m_led_stt_init"
.LASF0:
	.string	"unsigned int"
.LASF14:
	.string	"LED_STYLE_NOT_CONFIGURED"
.LASF8:
	.string	"uint16_t"
.LASF31:
	.string	"_led_handle_count"
.LASF16:
	.string	"LED_STYLE_SMARTCONFIG"
.LASF9:
	.string	"short unsigned int"
.LASF60:
	.string	"m_start_timer"
.LASF44:
	.string	"char"
.LASF3:
	.string	"long int"
.LASF36:
	.string	"_set_led_stt_off_cb"
.LASF5:
	.string	"int32_t"
.LASF28:
	.string	"p_led_stt_smart_config_t"
.LASF62:
	.string	"time_ms"
.LASF70:
	.string	"/home/dungnt98/ai-wb2-baremetal/smarthome_devices/build_out/gpio"
.LASF20:
	.string	"LED_STYLE_UPDATE"
.LASF11:
	.string	"long unsigned int"
.LASF42:
	.string	"_set_led_stt_device_active_cb"
.LASF37:
	.string	"_toggle_led_stt_cb"
.LASF56:
	.string	"set_led_stt_update_ota"
.LASF52:
	.string	"xTimers"
.LASF72:
	.string	"tmrTimerControl"
.LASF33:
	.string	"_led_stt_type"
.LASF53:
	.string	"timeline_status"
.LASF27:
	.string	"p_led_stt_ap_config_t"
.LASF18:
	.string	"LED_STYLE_ON"
.LASF34:
	.string	"_init_led_stt_cb"
.LASF64:
	.string	"hw_led_stt_cfg"
.LASF17:
	.string	"LED_STYLE_APCONFIG"
.LASF13:
	.string	"LED_STYLE_UNDEFINED"
.LASF68:
	.string	"GNU C99 10.2.0 -march=rv32imfc -mabi=ilp32f -march=rv32imfc -gdwarf -Os -std=gnu99 -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -fcommon -ffreestanding -fno-strict-aliasing"
	.ident	"GCC: (SiFive GCC-Metal 10.2.0-2020.12.8) 10.2.0"
