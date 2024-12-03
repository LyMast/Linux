	.arch armv8-a
	.file	"main.c"
	.text
	.global	g_flag
	.bss
	.align	3
	.type	g_flag, %object
	.size	g_flag, 8
g_flag:
	.zero	8
	.global	g_turn
	.align	2
	.type	g_turn, %object
	.size	g_turn, 4
g_turn:
	.zero	4
	.global	g_count
	.align	2
	.type	g_count, %object
	.size	g_count, 4
g_count:
	.zero	4
	.text
	.align	2
	.global	test1
	.type	test1, %function
test1:
.LFB6:
	.cfi_startproc
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, 8]
	str	wzr, [sp, 28]
.L6:
	adrp	x0, g_flag
	add	x0, x0, :lo12:g_flag
	mov	w1, 1
	str	w1, [x0]
	adrp	x0, g_turn
	add	x0, x0, :lo12:g_turn
	mov	w1, 1
	str	w1, [x0]
	dmb	ish
	nop
.L3:
	adrp	x0, g_flag
	add	x0, x0, :lo12:g_flag
	ldr	w0, [x0, 4]
	cmp	w0, 0
	beq	.L2
	adrp	x0, g_turn
	add	x0, x0, :lo12:g_turn
	ldr	w0, [x0]
	cmp	w0, 1
	beq	.L3
.L2:
	adrp	x0, g_count
	add	x0, x0, :lo12:g_count
	ldr	w0, [x0]
	add	w1, w0, 1
	adrp	x0, g_count
	add	x0, x0, :lo12:g_count
	str	w1, [x0]
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
	adrp	x0, g_flag
	add	x0, x0, :lo12:g_flag
	str	wzr, [x0]
	ldr	w1, [sp, 28]
	mov	w0, 57600
	movk	w0, 0x5f5, lsl 16
	cmp	w1, w0
	beq	.L9
	b	.L6
.L9:
	nop
	mov	x0, 0
	add	sp, sp, 32
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE6:
	.size	test1, .-test1
	.align	2
	.global	test2
	.type	test2, %function
test2:
.LFB7:
	.cfi_startproc
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, 8]
	str	wzr, [sp, 28]
.L15:
	adrp	x0, g_flag
	add	x0, x0, :lo12:g_flag
	mov	w1, 1
	str	w1, [x0, 4]
	adrp	x0, g_turn
	add	x0, x0, :lo12:g_turn
	str	wzr, [x0]
	dmb	ish
	nop
.L12:
	adrp	x0, g_flag
	add	x0, x0, :lo12:g_flag
	ldr	w0, [x0]
	cmp	w0, 0
	beq	.L11
	adrp	x0, g_turn
	add	x0, x0, :lo12:g_turn
	ldr	w0, [x0]
	cmp	w0, 0
	beq	.L12
.L11:
	adrp	x0, g_count
	add	x0, x0, :lo12:g_count
	ldr	w0, [x0]
	add	w1, w0, 1
	adrp	x0, g_count
	add	x0, x0, :lo12:g_count
	str	w1, [x0]
	ldr	w0, [sp, 28]
	add	w0, w0, 1
	str	w0, [sp, 28]
	adrp	x0, g_flag
	add	x0, x0, :lo12:g_flag
	str	wzr, [x0, 4]
	ldr	w1, [sp, 28]
	mov	w0, 57600
	movk	w0, 0x5f5, lsl 16
	cmp	w1, w0
	beq	.L18
	b	.L15
.L18:
	nop
	mov	x0, 0
	add	sp, sp, 32
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE7:
	.size	test2, .-test2
	.section	.rodata
	.align	3
.LC0:
	.string	"g_count : %d\n"
	.text
	.align	2
	.global	main
	.type	main, %function
main:
.LFB8:
	.cfi_startproc
	stp	x29, x30, [sp, -48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 29, -48
	.cfi_offset 30, -40
	mov	x29, sp
	add	x4, sp, 24
	mov	x3, 0
	adrp	x0, test1
	add	x2, x0, :lo12:test1
	mov	x1, 0
	mov	x0, x4
	bl	pthread_create
	str	w0, [sp, 40]
	add	x0, sp, 24
	add	x4, x0, 8
	mov	x3, 0
	adrp	x0, test2
	add	x2, x0, :lo12:test2
	mov	x1, 0
	mov	x0, x4
	bl	pthread_create
	str	w0, [sp, 40]
	str	wzr, [sp, 44]
	b	.L20
.L21:
	ldrsw	x0, [sp, 44]
	lsl	x0, x0, 3
	add	x1, sp, 24
	ldr	x0, [x1, x0]
	mov	x1, 0
	bl	pthread_join
	ldr	w0, [sp, 44]
	add	w0, w0, 1
	str	w0, [sp, 44]
.L20:
	ldr	w0, [sp, 44]
	cmp	w0, 1
	ble	.L21
	adrp	x0, g_count
	add	x0, x0, :lo12:g_count
	ldr	w0, [x0]
	mov	w1, w0
	adrp	x0, .LC0
	add	x0, x0, :lo12:.LC0
	bl	printf
	mov	w0, 0
	ldp	x29, x30, [sp], 48
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE8:
	.size	main, .-main
	.ident	"GCC: (Debian 12.2.0-14) 12.2.0"
	.section	.note.GNU-stack,"",@progbits
