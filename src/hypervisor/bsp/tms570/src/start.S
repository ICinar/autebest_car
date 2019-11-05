/*
 * start.S
 *
 * Assembler startup code for Cortex R4 on TMS570
 *
 * azuepke, 2013-10-31: initial
 * azuepke, 2013-11-11: add resetting of core registers
 * azuepke, 2014-04-28: made similar to simulator startup code
 */

#include <board_stuff.h>
#include <arm_cr.h>
#include <arm_perf.h>
#include <assembler.h>


	.global _start
	.global __board_halt
	.global __board_idle

	.text
	.section .text.start, "ax"

	.arm
FUNC(_start)
	/* the exception vectors reside at address 0x00000000 */
	b		arm_vector_reset	/* reset */
	b		arm_vector_undef	/* undef */
	b		arm_vector_svc		/* SVC */
	b		arm_vector_pabt		/* prefetch abort */
	b		arm_vector_dabt		/* data abort */
	b		.					/* unused vector */
	b		arm_vector_irq		/* IRQ */
	b		arm_vector_fiq		/* FIQ */
	@ldr		pc, [pc, #-0x1b0]	/* IRQ -- vector from controller */
	@ldr		pc, [pc, #-0x1b0]	/* FIQ -- vector from controller */

arm_vector_reset:
	/* TI recommended sequence to initialize the Cortex R4F core, see:
	 *   Recommended Inintializations for TMS570LS20x/10x Microcontrollers
	 *   SPNA119 - March 2010
	 *   http://www.ti.com/lit/an/spna119/spna119.pdf
	 */

	/* USER and SYSTEM modes share the same registers */

	movw	r0, #(CPSR_E|CPSR_A|CPSR_I|CPSR_F|CPSR_MODE_SYSTEM)
	msr		cpsr, r0
	mov		r1, #0
	mov		r2, #0
	mov		r3, #0
	mov		r4, #0
	mov		r5, #0
	mov		r6, #0
	mov		r7, #0
	mov		r8, #0
	mov		r9, #0
	mov		r10, #0
	mov		r11, #0
	mov		r12, #0
	mov		r13, #0
	mov		r14, #0

	/* FIQ has banked r8..r14 */
	cps		#CPSR_MODE_FIQ
	mov		r8, #0
	mov		r9, #0
	mov		r10, #0
	mov		r11, #0
	mov		r12, #0
	mov		r13, #0
	mov		r14, #0
	msr		spsr_cxsf, r0

	/* all other modes: IRQ, ABORT, UNDEF, SVC */
	cps		#CPSR_MODE_IRQ
	mov		r13, #0
	mov		r14, #0
	msr		spsr_cxsf, r0
	cps		#CPSR_MODE_ABORT
	mov		r13, #0
	mov		r14, #0
	msr		spsr_cxsf, r0
	cps		#CPSR_MODE_UNDEF
	mov		r13, #0
	mov		r14, #0
	msr		spsr_cxsf, r0
	cps		#CPSR_MODE_SVC
	mov		r13, #0
	mov		r14, #0
	msr		spsr_cxsf, r0

	/*
	 * enable FPU:
	 *  1. program CPACR
	 *  2. enable FPU in FPEXC
	 */
	mov		r0, #CPACR_CP10|CPACR_CP11
	mcr		p15, 0, r0, c1, c0, 2
	@mrc		p15, 0, r0, c0, c0, 0	/* FIXME: reading a control register flushes the pipeline */
	dsb

	/* enable FPU */
	mov		r0, #FPEXC_EN
	mcr		p10, 7, r0, c8, c0, 0
	@vmrs	r1, FPSID					/* FIXME: reading a control register flushes the pipeline */
	dsb

	/* no need to initialize FPSCR, zero on reset */
	fmdrr	d0, r1, r1
	fmdrr	d1, r1, r1
	fmdrr	d2, r1, r1
	fmdrr	d3, r1, r1
	fmdrr	d4, r1, r1
	fmdrr	d5, r1, r1
	fmdrr	d6, r1, r1
	fmdrr	d7, r1, r1
	fmdrr	d8, r1, r1
	fmdrr	d9, r1, r1
	fmdrr	d10, r1, r1
	fmdrr	d11, r1, r1
	fmdrr	d12, r1, r1
	fmdrr	d13, r1, r1
	fmdrr	d14, r1, r1
	fmdrr	d15, r1, r1

	/* initialize return stack */
	bl		1f
1:	bl		2f
2:	bl		3f
3:	bl		4f
4:	mov		lr, #0

	/*
	 * Now we are done with the low level CPU initialization
	 */


	/*
	 * ARM Erratum 737195:
	 *   Conditional VMRS APSR_nzcv, FPSCR may evaluate with incorrect flags
	 *
	 * Workaround:
	 *   Disable out-of-order single-precision floating point
	 *   multiply-accumulate (SP-MAC) instruction completion.
	 */
	/* set bit 16 (DOOFMACS) in Secondary Auxiliary Control Register */
	mrc		p15, 0, r0, c15, c0, 0
	orr		r0, #0x10000
	mcr		p15, 0, r0, c15, c0, 0

	/*
	 * ARM Erratum 754269:
	 *   Register corruption during a load-multiple instruction
	 *   at an exception vector
	 *
	 * Workaround:
	 *   Disable out-of-order completion for divide instructions.
	 */
	/* set bit 7 in Auxiliary Control Register */
	mrc		p15, 0, r0, c1, c0, 1
	orr		r0, #0x80
	mcr		p15, 0, r0, c1, c0, 1


	/*
	 * Enable CPU event export as well for hardware debugging
	 */
	/* set bit 4 (X) in Performance Monitor Control Register (PMNC) */
	mrc		p15, 0, r0, c9, c12, 0
	orr		r0, #ARM_PERF_PMCR_X
	mcr		p15, 0, r0, c9, c12, 0


	/*
	 * initialize SRAM: MINIT...
	 */
	mov		r0, #0xffffff00

	/* write magic 0xa to enable memory initialization */
	mov		r1, #0xa
	str		r1, [r0, #0x5C]

	/* start memory initialization */
	@movw	r1, #0x657F
	mov		r1, #0x1	/* FIXME: just RAM for now, hangs otherwise */
	str		r1, [r0, #0x60]

	/* wait until memory init is complete */
1:	ldr		r2, [r0, #0x68]
	tst		r2, #0x100
	beq		1b
	/* write magic 0x5 to disable memory initialization */
	mov		r1, #5
	str		r1, [r0, #0x5c]

#if 1
	/*
	 * enable ECC support for B1TCMPCEN (bit 27) and B0TCMPCEN (bit 26)
	 * in Auxiliary Control Register.
	 * enable ECC support for Flash as well in ATCMPCEN (bit 25)
	 */
	mrc		p15, 0, r0, c1, c0, 1
	orr		r0, #0x0c000000
	orr		r0, #0x02000000
	mcr		p15, 0, r0, c1, c0, 1
	isb
#endif

#if 0
	/*
	 * enable vectored interrupt mode, VIC controller provides handler address
	 * in System Control Register
	 */
	mrc		p15, 0, r0, c1, c0, 0
	orr		r0, #0x01000000
	mcr		p15, 0, r0, c1, c0, 0
#endif


	/*
	 * copy .data and clear .bss
	 */
_copy_data:
	ldr		r0, =__rom_data_start
	ldr		r1, =__data_start
	ldr		r2, =__data_end

1:	cmp		r1, r2
	ldrccd	r4, [r0], #8
	strccd	r4, [r1], #8
	bcc		1b

	mov		r4, #0
	mov		r5, #0
	ldr		r2, =__bss_end

2:	cmp		r1, r2
	strccd	r4, [r1], #8
	bcc		2b


	/* enable MPU, no caches */
	mrc		p15, 0, r0, c1, c0, 0
	# enable MPU and MPU background region
	orr		r0, #SCTLR_M
	orr		r0, #SCTLR_BR
	# exceptions in ARM mode, not thumb
	bic		r0, #SCTLR_TE
	# disable vectored interrupts
	bic		r0, #SCTLR_VE
	# enable strict alignment
	orr		r0, #SCTLR_A

	/* magic moment, update SCTLR */
	isb
	dsb
	mcr		p15, 0, r0, c1, c0, 0
	dsb
	isb

	/* ... invoke C level, put invalid return address on stack */
	ldr		sp, =__kern_stack
	bl		board_init
	/* does not return */

FUNC(__board_halt)
	cpsid	iaf, #CPSR_MODE_SVC
1:	dsb
	wfi
	b		1b

FUNC(__board_idle)
	wfi
	/* FIXME: TI code has 4x nop after WFI, why? */
	nop
	nop
	nop
	nop
	bx		lr

	/* last symbol, for better debugging */
_const_vars:
