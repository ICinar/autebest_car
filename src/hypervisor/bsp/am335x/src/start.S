/*
 * start.S
 *
 * Assembler startup code for ARM
 *
 * NOTE: this is common code for both ARMv6 and v7 processors
 * The additional ISBs and DSBs do not hurt on ARMv6 processors.
 *
 * azuepke, 2013-09-12: initial
 * azuepke, 2014-05-06: adapted to MPU
 */

#include <board_stuff.h>
#include <arm_cr.h>
#include <assembler.h>


	.global _start
	.global __board_halt
	.global armv7_flush_dcache_all

	.text
	.section .text.start, "ax"

	/* entry point from boot loader */
	/* NOTE: the boot loader always calls us in 32-bit SVC or MON mode! */
	.arm
_start:
	/* disable interrupts and switch to SVC mode */
	cpsid	iaf, #CPSR_MODE_SVC

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

#if 0
_copy_vectors:
	ldr		r0, =_vectors
	ldr		r1, =0x80000000
	ldr		r2, =0x80000000 + 32

3:	ldrd	r4, [r0], #8
	strd	r4, [r1], #8
	cmp		r1, r2
	bcc		3b
#endif

_common_start:
	/* NOTE: this sequence deliberately lacks ISBs and DSBs!
	 * These are required at the time we finally go virtual!
	 */

	/* invalidate instruction cache and unified TLB */
	mov		r0, #0
	mcr		p15, 0, r0, c7, c5, 0
	mcr		p15, 0, r0, c8, c7, 0
	ISB
	DSB

	/* invalidate branch prediction */
	mcr		p15, 0, r0, c7, c5, 6

	/* invalidate data cache */
	/* ARMv7 requires complex code to invalidate the data cache */
	/*
	* based on ARM example code
	* ARM ARM DDI 0406A, "Example code for cache maintenance operations"
	* clobbers r0 .. r5
	*/
	mov		r0, #0					@ select cache level 0
	mcr		p15, 2, r0, c0, c0, 0	@ write the Cache Size selection register
	isb								@ ISB to sync the change to the CacheSizeID reg
	mrc		p15, 1, r0, c0, c0, 0	@ reads current Cache Size ID register
	and		r2, r0, #0x7			@ extract the line length field
	add		r2, r2, #4				@ add 4 for the line length offset (log2 16 bytes)
	ldr		r4, =0x3ff
	ands	r4, r4, r0, lsr #3		@ r4 is the max number on the way size (right aligned)
	clz		r5, r4					@ r5 is the bit position of the way size increment
	ldr		r1, =0x7fff
	ands	r1, r1, r0, lsr #13		@ r1 is the max number of the index size (right aligned)
1:
	mov		r3, r4					@ r3 working copy of the max way size (right aligned)
2:
	mov		r0, r3, lsl r5			@ factor in the way number into r0 (for cache number #0)
	orr		r0, r0, r1, lsl r2		@ factor in the index number
	mcr		p15, 0, r0, c7, c6, 2	@ invalidate by set/way
	subs	r3, r3, #1				@ decrement the way number
	bge		2b
	subs	r1, r1, #1				@ decrement the index
	bge		1b


	/* MMU setup */

	/* set domain */
	ldr		r0, =0x55555555
	mcr		p15, 0, r0, c3, c0, 0

	/* set TTBR0, TTBR1 and TTBCR */
	ldr		r0, =mpu_part_cfg
	ldr		r0, [r0]	/* just use first partition's page tables */
	orr		r0, #0x02b	/* inner: WA + shareable, outer: WA + not shareable */
	mcr		p15, 0, r0, c2, c0, 0
	/* disable TTBR1 */
	mov		r0, #0
	mcr		p15, 0, r0, c2, c0, 1
	mcr		p15, 0, r0, c2, c0, 2

	/* use ASID 0, disable FCSR */
	mcr		p15, 0, r0, c13, c0, 0
	mcr		p15, 0, r0, c13, c0, 1


	/* enable MMU and caches */
	mrc		p15, 0, r0, c1, c0, 0
	ldr		r1, =SCTLR_CLR
	bic		r0, r1
	ldr		r1, =(SCTLR_SET | SCTLR_C | SCTLR_I | SCTLR_W)
	orr		r0, r1

	/* this enables the MMU */
	orr		r0, #SCTLR_M

	/* this enables icache */
	orr		r0, #SCTLR_I

	/* this enables dcache */
	orr		r0, #SCTLR_C

	/* this enables write buffer */
	orr		r0, #SCTLR_W


	/* magic moment! */
	ISB
	DSB

	/* update SCTLR */
	mcr		p15, 0, r0, c1, c0, 0
	DSB
	ISB

#if 1
	/* Huh? on Cortex A8??? */
	/* set VBAR to our vectors (we have exception vectors in RAM) */
	adr		r0, _vectors
	mcr		p15, 0, r0, c12, c0, 0
#endif

	/* init FPU */
	/* allow user to access VFP */
	mov		r0, #CPACR_CP10|CPACR_CP11
	mcr		p15, 0, r0, c1, c0, 2

	/* enable FPU */
	mov		r0, #FPEXC_EN
	mcr		p10, 7, r0, c8, c0, 0

	/* finally ... */
	ISB
	DSB

	/* ... invoke C level, put invalid return address on stack */
	ldr		sp, =__kern_stack
	bl		board_init
	/* does not return */


FUNC(__board_halt)
	cpsid	iaf, #CPSR_MODE_SVC
1:	DSB
	WFI
	b		1b

	/* the exception vectors */
	.balign 32
_vectors:
	b		.					/* reset */
	b		arm_vector_undef	/* undef */
	b		arm_vector_svc		/* SVC */
	b		arm_vector_pabt		/* prefetch abort */
	b		arm_vector_dabt		/* data abort */
	b		.					/* unused vector */
	b		arm_vector_irq		/* IRQ */
	b		arm_vector_fiq		/* FIQ */

/*
 * based on ARM example code
 * ARM ARM DDI 0406A, "Example code for cache maintenance operations"
 * clobbers r0 .. r5, r7, r9 .. r11
 */
FUNC(armv7_flush_dcache_all)
	push	{r4, r5, r7, r9, r10, r11, lr}
	dmb

	mrc		p15, 1, r0, c0, c0, 1	@ Read CLIDR
	ands	r3, r0, #0x7000000
	mov		r3, r3, lsr #23			@ Cache level value (naturally aligned)
	beq		5f
	mov		r10, #0
1:
	add		r2, r10, r10, lsr #1	@ Work out 3xcachelevel
	mov		r1, r0, lsr r2			@ bottom 3 bits are the Cache type for this level
	and		r1, r1, #7				@ get those 3 bits alone
	cmp		r1, #2
	blt		4f						@ no cache or only instruction cache at this level
	mcr		p15, 2, r10, c0, c0, 0	@ write the Cache Size selection register
	isb								@ ISB to sync the change to the CacheSizeID reg
	mrc		p15, 1, r1, c0, c0, 0	@ reads current Cache Size ID register
	and		r2, r1, #0x7			@ extract the line length field
	add		r2, r2, #4				@ add 4 for the line length offset (log2 16 bytes)
	ldr		r4, =0x3ff
	ands	r4, r4, r1, lsr #3		@ r4 is the max number on the way size (right aligned)
	clz		r5, r4					@ r5 is the bit position of the way size increment
	ldr		r7, =0x7fff
	ands	r7, r7, r1, lsr #13		@ r7 is the max number of the index size (right aligned)
2:
	mov		r9, r4					@ r9 working copy of the max way size (right aligned)
3:
	orr		r11, r10, r9, lsl r5	@ factor in the way number and cache number into r11
	orr		r11, r11, r7, lsl r2	@ factor in the index number
	mcr		p15, 0, r11, c7, c14, 2	@ clean and invalidate by set/way
	subs	r9, r9, #1				@ decrement the way number
	bge		3b
	subs	r7, r7, #1				@ decrement the index
	bge		2b
4:
	add		r10, r10, #2			@ increment the cache number
	cmp		r3, r10
	bgt		1b
5:
	mov		r10, #0					@ select cache level 0
	mcr		p15, 2, r10, c0, c0, 0	@ write the Cache Size selection register
	isb

	/* returns 0 */
	mov		r0, #0
	pop		{r4, r5, r7, r9, r10, r11, pc}


	/* last symbol, for better debugging */
_const_vars:
