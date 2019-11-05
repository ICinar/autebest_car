/*
 * start.S
 *
 * Assembler startup code for ARM Cortex M3/M4
 *
 * azuepke, 2015-06-26: initial
 */

#include <board_stuff.h>
#include <arm_cr.h>
#include <assembler.h>
#include <hv_compiler.h>

	.global _start
	.global __board_halt

	.syntax unified

	.section .text.start, "ax"
	/* vector table */
	/* 0 */
	.word	__kern_stack	/* stack for reset vector */
	.word	_start
	.word	arm_vector_nmi
	.word	arm_vector_hard
	.word	arm_vector_memory
	.word	arm_vector_bus
	.word	arm_vector_usage
	.word	0	/* reserved */

	/* 8 */
	.word	0	/* reserved */
	.word	0	/* reserved */
	.word	0	/* reserved */
	.word	arm_vector_svc
	.word	arm_vector_debug
	.word	0	/* reserved */
	.word	arm_vector_irq	/* pendsv */
	.word	arm_vector_irq	/* systick */

	/* 16..255 */
	irq = 16
	.rept NUM_IRQS
		.word	arm_vector_irq
		irq = irq + 1
	.endr

	/* entry point from reset */
	/* starting in Thread mode on Main stack */
	.section .text, "ax"
	.thumb
FUNC(_start)
	/* reload stack pointer (when loaded via debugger) */
	ldr		sp, =__kern_stack
	cpsid	if

	/*
	 * copy .data and clear .bss
	 */
	ldr		r0, =__rom_data_start
	ldr		r1, =__data_start
	ldr		r2, =__data_end

1:	cmp		r1, r2
	ldrdcc	r4, [r0], #8
	strdcc	r4, [r1], #8
	bcc		1b

	mov		r4, #0
	mov		r5, #0
	ldr		r2, =__bss_end

2:	cmp		r1, r2
	strdcc	r4, [r1], #8
	bcc		2b

_common_start:
	/* ... invoke C level, put invalid return address on stack */
	b		board_init
	/* does not return */

FUNC(__board_halt)
	cpsid	if
1:	DSB
	WFI
	b		1b

_const_vars:
