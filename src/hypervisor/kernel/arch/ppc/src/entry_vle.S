/*
 * entry.S
 *
 * PowerPC exception handling kernel entry/exit code
 *
 * azuepke, 2013-11-21: initial PPC port
 * azuepke, 2014-05-16: more exception handlers
 * azuepke, 2014-06-03: reworked for MPU and single kernel stack
 * azuepke, 2014-09-02: support for IRQ stack
 * azuepke, 2015-05-27: simplified to single kernel stack
 */

#include <syscalls.h>
#include <ppc_msr.h>
#include <ppc_spr.h>
#include <ppc_asm.h>
#include <sched_state.h>
#include <arch_state.h>

	.global ppc_switch_to_kernel_stack
	.global ppc_spe_save
	.global ppc_spe_restore
	.global ppc_set_ivors
	.global _ivor_0
    .global _ivor_1
    .global _ivor_2
    .global _ivor_3
    .global _ivor_4
    .global _ivor_5
    .global _ivor_6
    .global _ivor_8
    .global _ivor_15
    .global _ivor_33
    .global _ivor_34

    .global _syscalls
    .global _ppc_exception_table
	.global _hardware_exception_table

	.text

/*
 * Register usage
 *
 * - SPRG0 and SPRG1 are used as scratch registers in low-level exception code
 * - SPRG2 unused
 * - SPRG3 points to sched_state
 * - SPRG4 debug scratch?
 * - SPRG5 debug scratch?
 * - SPRG6 crit_irq stack + save area (points to save area)
 * - SPRG7 mce stack + save area (points to save area)
 *
 * - On entering the kernel, r2 must point to sched_state.
 */

/* Register frame: we save 40 integer registers.
 * When saving on stack, we need additional 4 empty slots
 * for a valid stack frame
 */
#define FRAMESIZE	(40*4)
#define REG(r, b)	(r)*4(b)

/* register layout -- see struct arch_reg_frame in arch_regs.h */
#define R_SRR0		32
#define R_SRR1		33
#define R_LR		34
#define R_CTR		35
#define R_CR		36
#define R_XER		37
#define R_FAULT		38
#define R_EXC		39

/*
 * NOTE: all IVORs must be kept in a 64K aligned memory region!
 * Make sure the kernel does not grow beyond 64K in size!
 */
#define IVOR_ENTRY(num, label)	\
	.balign 16	;\
_ivor_##num:	;\
_ivor_##label:	;


/** IVOR exception entry code, used by non-critical exception IVORs
 *
 * - checks if interrupt/exception happened in user / kernel
 *   - in user: switch to kernel stack, use user register save area
 *   - in kernel: allocate new reg save area on existing kernel stack
 * - r0 is the exception number
 * - r1 points to the right kernel stack
 * - r2 contains CR
 * - r3 is set to the register save area
 * - registers saved to save area: r0 + r1
 * - registers saved in SPRGs: r2 + r3
 */
.macro IVOR_EXCEPTION_HANDLER ivor handler
	/* save r2 + r3 and cr in r2 */
	mtspr	SPR_SPRG0, r2
	mtspr	SPR_SPRG1, r3
	mfcr	r2
	/* we can now use r3 as working register */

	/* test if MSR_PR bit is zero */
	mfspr	r3, SPR_SRR1
	e_rlwinm r3, r3, 0, 17, 17
    se_cmpi  r3, 0
    e_beq    1f

	/* exception in user mode: use task save area */
	mfspr	r3, SPR_SPRG3	/* get register save area */
	e_lwz	r3, SCHED_STATE_REGS(r3)
	e_stw	r0, REG(0, r3)
	e_stw	r1, REG(1, r3)
	mfspr	r1, SPR_SPRG3	/* get kernel stack */
	e_lwz	r1, ARCH_STATE_KERN_STACK(r1)
	se_li	r0, \ivor
	e_b		\handler

1:	/* exception in kernel mode: save area on kernel stack */
	e_add16i	r3, r1, -FRAMESIZE
	e_stw	r0, REG(0, r3)
	e_stw	r1, REG(1, r3)
	e_add16i	r1, r3, -8		/* new frame on kernel stack */
	se_li	r0, \ivor
	e_b		\handler
.endm


/** IVOR interrupt entry code, used by non-critical interrupt IVORs
 *
 * - always saves registers in user register save area
 * - always switches to the kernel stack
 * - r0 is the exception number
 * - r1 points to the right kernel stack
 * - r2 contains CR
 * - r3 is set to the register save area
 * - registers saved to save area: r0 + r1
 * - registers saved in SPRGs: r2 + r3
 */
.macro IVOR_INTERRUPT_HANDLER ivor handler
	/* save r2 + r3 and cr in r2 */
	mtspr	SPR_SPRG0, r2
	mtspr	SPR_SPRG1, r3
	mfcr	r2
	/* we can now use r3 as working register */

	/* save registers in current task save area */
	mfspr	r3, SPR_SPRG3
	se_lwz	r3, SCHED_STATE_REGS(r3)
	e_stw	r0, REG(0, r3)
	e_stw	r1, REG(1, r3)

	/* switch to kernel stack */
	mfspr	r1, SPR_SPRG3
	se_lwz	r1, ARCH_STATE_KERN_STACK(r1)

	/* interrupt in user or kernel mode */
	se_li	r0, \ivor
	e_b		\handler
.endm

/** IVOR critical interrupt entry code
 *
 * - maskable interrupt, can interrupt everywhere, except itself
 * - uses a dedicated stack
 * - stack pointer points to register save area
 * - r2 contains sched_state
 * - r3 is set to the register save area
 * - r4 is the exception number
 * - registers saved to save area: r4 + r5
 * - registers saved in SPRGs: r2 + r3
 */
.macro IVOR_CRITICAL_HANDLER ivor handler
	/* free r2+r3 as working registers */
	mtspr	SPR_SPRG4, r2
	mtspr	SPR_SPRG5, r3

	/* get register save area in r2, pointer to sched_state in r3 */
	mfspr	r2, SPR_SPRG3
	se_lwz	r3, ARCH_STATE_CRIT_STACK(r2)

	/* save r4 and r5 */
	e_stw	r4, REG(4, r3)
	e_stw	r5, REG(5, r3)
	se_li	r4, \ivor
	e_b		\handler
.endm


/**************************************************************************/


/* critical input (== critical interrupt) */
/* critical */
IVOR_ENTRY(0, crit)
	IVOR_CRITICAL_HANDLER 0 _critical_interrupt


/* machine check */
/* critical or machine critical */
IVOR_ENTRY(1, mce)
	IVOR_EXCEPTION_HANDLER 1 _machinecheck_exception


/* data storage */
IVOR_ENTRY(2, dsi)
	IVOR_EXCEPTION_HANDLER 2 _common_exception


/* instruction storage */
IVOR_ENTRY(3, isi)
	IVOR_EXCEPTION_HANDLER 3, _common_exception


/* external input (== normal interrupt) */
IVOR_ENTRY(4, ext)
	IVOR_INTERRUPT_HANDLER 4 _common_interrupt


/* alignment */
IVOR_ENTRY(5, align)
	IVOR_EXCEPTION_HANDLER 5 _common_exception


/* program */
IVOR_ENTRY(6, program)
	IVOR_EXCEPTION_HANDLER 6 _common_exception


/* floating-point unavailable (not on the e500) */
IVOR_ENTRY(7, fp_unavail)
	IVOR_EXCEPTION_HANDLER 7 _common_exception


/* system call */
IVOR_ENTRY(8, syscall)
	/* syscalls always come from user space -- free r1+r2 as working registers */
	mtspr	SPR_SPRG0, r1
	mtspr	SPR_SPRG1, r2

	/* get register save area in r1, pointer to sched_state in r2 */
	mfspr	r2, SPR_SPRG3
	se_lwz	r1, SCHED_STATE_REGS(r2)

	/* arguments in r3..r8, syscall number in r0 */
	/* use volatile registers r9..12 as scratch registers */

	/* save SRR0, SRR1, LR, CR (partly non-volatile) */
	mfspr	r9,  SPR_SRR0
	mfspr	r10, SPR_SRR1
	e_addi	r11, r3, 0
	se_mflr	r3
	mfcr	r12
	e_stw	r9,  REG(R_SRR0, r1)
	e_stw	r10, REG(R_SRR1, r1)
	e_stw	r3, REG(R_LR, r1)
	e_addi	r3, r11, 0
	e_stw	r12, REG(R_CR, r1)

	/* save original r1 and r2 */
	mfspr	r9,  SPR_SPRG0
	mfspr	r10, SPR_SPRG1
	e_stw	r9,  REG(1, r1)
	e_stw	r10, REG(2, r1)

	/* the remaining regs are non-volatile */

	/* switch to kernel stack */
	se_lwz	r1, ARCH_STATE_KERN_STACK(r2)

	/* do syscall[r0] within NUM_SYSCALLS limits */
	e_cmpl16i r0, NUM_SYSCALLS
	e_ble	1f
	e_li	r0, NUM_SYSCALLS
1:
	lwi		r12, _syscalls
	e_rlwinm	r0, r0, 2, 0, 29
	lwzx	r0, r12, r0
	se_mtlr	r0
	se_li	r0, 0

	/* terminate stack back chain and call */
	se_stw	r0, 0(r1)
	se_blrl

	/* r2 still contains sched_state */

	/* check for rescheduling */
	se_lwz	r0, SCHED_STATE_RESCHEDULE(r2)
	se_cmpi	r0, 0
	e_bne	_syscall_exit_slow

_syscall_exit_fast:
	/* let r1 contain the register save area */
	se_lwz	r1, SCHED_STATE_REGS(r2)

	/* restore mandatory user mode registers, clear rest (info leakage) */
	e_lwz	r2, REG(2, r1)

	/* restore r3..r6 (return code plus additional output registers) */
	e_lwz	r3, REG(3, r1)
	e_lwz	r4, REG(4, r1)
	e_lwz	r5, REG(5, r1)
	e_lwz	r6, REG(6, r1)

	e_lwz	r9, REG(R_SRR0, r1)
	e_lwz	r10, REG(R_SRR1, r1)
	e_lwz	r8, REG(R_CR, r1)
	e_lwz	r7, REG(R_LR, r1)
	mtspr	SPR_SRR0, r9
	mtspr	SPR_SRR1, r10
	mtcr	r8
	se_mtlr	r7

	/* switch back to user stack */
	e_lwz	r1, REG(1, r1)

	/* hide kernel register state */
	/* r0 must be zero due to the "cmpwi r0, #0" from above */
	e_addi	r11, r0, 0
	e_addi	r12, r0, 0
	se_mtctr	r0
	mtxer	r12

	/* and return */
	se_rfi

	/* syscall exit handling */
_syscall_exit_slow:

	/* let r4 contain the register save area */
	se_lwz	r4, SCHED_STATE_REGS(r2)

	/* save remaining non-volatile registers */
	e_stmw	r13, REG(13, r4)

	/* CR, LR, SRR0, SRR1 are already saved -- the rest is volatile */
	e_b		_ppc_return_resched


/* auxiliary processor unavailable (not on the e200/e500) */
IVOR_ENTRY(9, aux_unavail)
	IVOR_EXCEPTION_HANDLER 9 _common_exception


/* decrementer (== normal interrupt) */
IVOR_ENTRY(10, dec)
	IVOR_INTERRUPT_HANDLER 10 _common_interrupt


/* fixed-interval timer interrupt */
IVOR_ENTRY(11, fit)
	IVOR_INTERRUPT_HANDLER 11 _common_interrupt


/* watchdog timer interrupt */
/* critical */
IVOR_ENTRY(12, wdog)
	IVOR_CRITICAL_HANDLER 0 _critical_interrupt


/* Data TLB error */
IVOR_ENTRY(13, dtlb)
	IVOR_EXCEPTION_HANDLER 13 _common_exception


/* Instruction TLB error */
IVOR_ENTRY(14, itlb)
	IVOR_EXCEPTION_HANDLER 14 _common_exception


/* debug */
IVOR_ENTRY(15, debug)
	/* FIXME: IMPLEMENTME */
1:	se_b	1b


/* SPE APU unavailable (e500 specific) */
IVOR_ENTRY(32, spe_unavail)
	IVOR_EXCEPTION_HANDLER 32 _common_exception


/* embedded floating-point data exception (e500 specific) */
IVOR_ENTRY(33, spe_data)
	IVOR_EXCEPTION_HANDLER 33 _common_exception


/* embedded floating-point round exception (e500 specific) */
IVOR_ENTRY(34, spe_round)
	IVOR_EXCEPTION_HANDLER 34 _common_exception



/**************************************************************************/


/** Machine check exception handling code:
 * see IVOR_EXCEPTION_HANDLER() for the expected register state
 */
_machinecheck_exception:
	/* save original r4..r31 */
	e_stmw	r4, REG(4, r3)

	/* save original r2 and r3 */
	mfspr	r30, SPR_SPRG0
	mfspr	r31, SPR_SPRG1
	e_stmw	r30, REG(2, r3)

	/* save SRR0, SRR1, LR, CTR, CR (still in r2), XER */
	mfspr	r26, SPR_MCSRR0
	mfspr	r27, SPR_MCSRR1
	se_mflr	r28
	se_mfctr	r29
	se_mr	r30, r2
	mfxer	r31
	e_add16i	r4, r3, 4*R_SRR0
	e_stmw	r26, 0(r4)

	/* save exception type */
	e_stw	r0, REG(R_EXC, r3)

	/* C code expects a pointer to sched_state in r2 */
	mfspr	r2, SPR_SPRG3

	se_mr	r4, r0
	se_li	r0, 0

	/* call with terminated stack back chain */
	se_stw	r0, 0(r1)
	/* r3: registers, r4: vector */
	e_bl	ppc_handler_mce
	e_b		_ppc_return_resched

/** Common exception handling code:
 * see IVOR_EXCEPTION_HANDLER() for the expected register state
 */
_common_exception:
	/* save original r4..r31 */
	e_stmw	r4, REG(4, r3)

	/* save original r2 and r3 */
	mfspr	r30, SPR_SPRG0
	mfspr	r31, SPR_SPRG1
	e_stmw	r30, REG(2, r3)

	/* save SRR0, SRR1, LR, CTR, CR (still in r2), XER */
	mfspr	r26, SPR_SRR0
	mfspr	r27, SPR_SRR1
	se_mflr	r28
	se_mfctr	r29
	se_mr	r30, r2
	mfxer	r31
	e_add16i	r4, r3, 4*R_SRR0
	e_stmw	r26, 0(r4)

	/* save exception type */
	e_stw	r0, REG(R_EXC, r3)

	/* C code expects a pointer to sched_state in r2 */
	mfspr	r2, SPR_SPRG3

	/* get second level handler address */
	lwi		r4, _ppc_exception_table
	e_rlwinm	r5, r0, 2, 0, 29
	lwzx	r5, r4, r5
	se_mr	r4, r0
	se_li	r0, 0
	se_mtlr	r5

	/* call with terminated stack back chain */
	se_stw	r0, 0(r1)
	/* r3: registers, r4: vector */
	se_blrl
	/* FALL-THROUGH */

/*
 * common return path for all exceptions in user mode
 * unconditionally reschedule (to get a valid register context for return)
 */
_ppc_return_resched:
	/* still on the kernel stack, invoke the scheduler */
	e_bl	sched_schedule
	/* returns a new register context to return to in r3 */
	se_mr	r1, r3

	/* restore all registers */
_ppc_restore_all:
	/* restore special regs first */
	e_add16i	r0, r1, 4*R_SRR0
    se_mtar     r11,r0
	
    e_lmw	r26, 0(r11)
    
	mtsrr0	r26
	mtsrr1	r27
	se_mtlr	r28
	se_mtctr	r29
	mtcr	r30
	mtxer	r31

	/* restore user regs (r1 must come last) */
	e_lmw	r2, REG(2, r1)
	e_lwz	r0, REG(0, r1)
	e_lwz	r1, REG(1, r1)
	se_rfi


/**************************************************************************/


/** Common interrupt handling code:
 * invoked by IVOR_INTERRUPT_HANDLER()
 */
_common_interrupt:
	/* save remaining non-volatile registers up to r12 */
	e_stw	r4, REG(4, r3)
	e_stw	r5, REG(5, r3)
	e_stw	r6, REG(6, r3)
	e_stw	r7, REG(7, r3)
	e_stw	r8, REG(8, r3)
	e_stw	r9, REG(9, r3)
	e_stw	r10, REG(10, r3)
	e_stw	r11, REG(11, r3)
	e_stw	r12, REG(12, r3)

	/* save original r2 and r3 */
	mfspr	r4, SPR_SPRG0
	mfspr	r5, SPR_SPRG1
	e_stw	r4, REG(2, r3)
	e_stw	r5, REG(3, r3)

	/* save SRR0, SRR1, LR, CTR, CR (still in r2), XER */
	mfspr	r7, SPR_SRR0
	mfspr	r8, SPR_SRR1
	se_mflr	r5
	se_mfctr	r6
	mfxer	r10
	e_stw	r7, REG(R_SRR0, r3)
	e_stw	r8, REG(R_SRR1, r3)
	e_stw	r5, REG(R_LR, r3)
	e_stw	r6, REG(R_CTR, r3)
	e_stw	r2, REG(R_CR, r3)
	e_stw	r10, REG(R_XER, r3)

	/* C code expects a pointer to sched_state in r2 */
	mfspr	r2, SPR_SPRG3

	/* get second level handler address */
	lwi		r4, _ppc_exception_table
	e_rlwinm	r5, r0, 2, 0, 29
	lwzx	r5, r4, r5
	se_mr	r4, r0
	se_li	r0, 0
	se_mtlr	r5

	/* call with terminated stack back chain */
	se_stw	r0, 0(r1)
	/* r3: registers, r4: vector */
	se_blrl
	/* FALL-THROUGH */

	/* r2 still contains sched_state */

	/* check for rescheduling */
	se_lwz	r0, SCHED_STATE_RESCHEDULE(r2)
	se_cmpi	r0, 0
	e_bne	_interrupt_exit_slow

	/* no rescheduling required */
_interrupt_exit_fast:
	/* let r1 contain the register save area */
	se_lwz	r1, SCHED_STATE_REGS(r2)

	/* restore  SRR0, SRR1, LR, CTR, CR, XER */
	e_lwz	r3, REG(R_SRR0, r1)
	e_lwz	r4, REG(R_SRR1, r1)
	e_lwz	r5, REG(R_LR, r1)
	e_lwz	r6, REG(R_CTR, r1)
	e_lwz	r7, REG(R_CR, r1)
	e_lwz	r8, REG(R_XER, r1)
	mtspr	SPR_SRR0, r3
	mtspr	SPR_SRR1, r4
	se_mtlr	r5
	se_mtctr	r6
	mtcr	r7
	mtxer	r8

	/* restore volatile registers */
	e_lwz	r2, REG(2, r1)
	e_lwz	r3, REG(3, r1)
	e_lwz	r4, REG(4, r1)
	e_lwz	r5, REG(5, r1)
	e_lwz	r6, REG(6, r1)
	e_lwz	r7, REG(7, r1)
	e_lwz	r8, REG(8, r1)
	e_lwz	r9, REG(9, r1)
	e_lwz	r10, REG(10, r1)
	e_lwz	r11, REG(11, r1)
	e_lwz	r12, REG(12, r1)

	e_lwz	r0, REG(0, r1)

	/* switch back to previous stack and return */
	e_lwz	r1, REG(1, r1)
	se_rfi


	/* rescheduling required! */
_interrupt_exit_slow:
	/* let r4 contain the register save area */
	se_lwz	r4, SCHED_STATE_REGS(r2)

	/* save remaining non-volatile registers */
	e_stmw	r13, REG(13, r4)

	/* now all registers are saved */
	e_b		_ppc_return_resched


/**************************************************************************/


/** Critical interrupt handling code:
 * invoked by IVOR_CRITICAL_HANDLER()
 */
_critical_interrupt:
	/* save remaining non-volatile registers up to r12 */
	e_stw	r0, REG(0, r3)
	e_stw	r1, REG(1, r3)
	mfspr	r0, SPR_SPRG4
	mfspr	r1, SPR_SPRG5
	e_stw	r0, REG(2, r3)
	e_stw	r1, REG(3, r3)
	e_stw	r6, REG(6, r3)
	e_stw	r7, REG(7, r3)
	e_stw	r8, REG(8, r3)
	e_stw	r9, REG(9, r3)
	e_stw	r10, REG(10, r3)
	e_stw	r11, REG(11, r3)
	e_stw	r12, REG(12, r3)

	/* save SRR0, SRR1, LR, CTR, CR, XER */
	mfspr	r7, SPR_CSRR0
	mfspr	r8, SPR_CSRR1
	se_mflr	r5
	se_mfctr	r6
	mfcr	r9
	mfxer	r10
	e_stw	r7, REG(R_SRR0, r3)
	e_stw	r8, REG(R_SRR1, r3)
	e_stw	r5, REG(R_LR, r3)
	e_stw	r6, REG(R_CTR, r3)
	e_stw	r9, REG(R_CR, r3)
	e_stw	r10, REG(R_XER, r3)

	/* prepare stack: space for a stack frame */
	e_add16i	r1, r3, -8

	/* get second level handler address */
	lwi		r5, _ppc_exception_table
	e_rlwinm	r6, r4, 2, 0, 29
	lwzx	r6, r5, r6
	se_li	r0, 0
	se_mtlr	r6

	/* call with terminated stack back chain */
	se_stw	r0, 0(r1)
	/* r3: registers, r4: vector */
	se_blrl
	/* FALL-THROUGH */

	/* critical interrupts never trigger rescheduling */

	/* undo stack frame */
	e_add16i	r1, r1, 8

	/* restore  SRR0, SRR1, LR, CTR, CR, XER */
	e_lwz	r3, REG(R_SRR0, r1)
	e_lwz	r4, REG(R_SRR1, r1)
	e_lwz	r5, REG(R_LR, r1)
	e_lwz	r6, REG(R_CTR, r1)
	e_lwz	r7, REG(R_CR, r1)
	e_lwz	r8, REG(R_XER, r1)
	mtspr	SPR_SRR0, r3
	mtspr	SPR_SRR1, r4
	se_mtlr	r5
	se_mtctr	r6
	mtcr	r7
	mtxer	r8

	/* restore volatile registers */
	e_lwz	r2, REG(2, r1)
	e_lwz	r3, REG(3, r1)
	e_lwz	r4, REG(4, r1)
	e_lwz	r5, REG(5, r1)
	e_lwz	r6, REG(6, r1)
	e_lwz	r7, REG(7, r1)
	e_lwz	r8, REG(8, r1)
	e_lwz	r9, REG(9, r1)
	e_lwz	r10, REG(10, r1)
	e_lwz	r11, REG(11, r1)
	e_lwz	r12, REG(12, r1)

	e_lwz	r0, REG(0, r1)

	/* switch back to previous stack and return */
	e_lwz	r1, REG(1, r1)
	se_rfci

/**************************************************************************/

/*
 * The exception table has pointers to C functions
 * Even indices refer to _kernel handlers,
 * and odd indices to _user handlers.
 * Odd handlers are sometimes omitted.
 */
	.balign 4
_ppc_exception_table:
	/* MMU exceptions */
/* IVOR 0 -- critical interrupt */
	.long	ppc_handler_irq_crit
/* IVOR 1 -- MCE */
	.long	ppc_handler_panic	/* never called via table */
/* IVOR 2 -- DSI */
	.long	ppc_handler_data
/* IVOR 3 -- ISI */
	.long	ppc_handler_inst
/* IVOR 4 -- external */
	.long	ppc_handler_irq
/* IVOR 5 -- alignment */
	.long	ppc_handler_align
/* IVOR 6 -- program */
	.long	ppc_handler_program
/* IVOR 7 */
	.long	ppc_handler_panic /* not raised, we keep MSR.FP disabled */
/* IVOR 8 -- system call */
	.long	ppc_handler_panic	/* never called via table */
/* IVOR 9 -- aux unavail -- not implemented on e200/e500 */
	.long	ppc_handler_panic
/* IVOR 10 -- decrementer */
	.long	ppc_handler_irq
/* IVOR 11 -- fixed interval timer */
	.long	ppc_handler_irq
/* IVOR 12 -- watchdog */
	.long	ppc_handler_irq_crit
/* IVOR 13 -- DTLB miss */
	.long	ppc_handler_data
/* IVOR 14 -- ITLB miss */
	.long	ppc_handler_inst
/* IVOR 15 -- debug exception */
	.long	ppc_handler_panic /* FIXME: not implemented */

/* IVOR 16 .. 31 */
	.rept 16
		.long	0	/* unused */
	.endr

/* IVOR 32 */
	.long	ppc_handler_spe
/* IVOR 33 */
	.long	ppc_handler_spe
/* IVOR 34 */
	.long	ppc_handler_spe
#if 0
/* IVOR 35 -- performance monitor (FIXME: only for user code ?) */
	.long	ppc_handler_perf
#endif


/**************************************************************************/


/*
 * switch to the kernel stack and invoke the kernel function
 * void ppc_switch_to_kernel_stack(unsigned long stack, void (*func)(void *)) __noreturn;
 *
 */
	.pushsection .text.init, "ax"

ppc_switch_to_kernel_stack:
	/* switch stack and terminate back chain */
	se_mr	r1, r3
	se_li	r0, 0
	e_stw	r0, 0(r1)
	/* call func() with new stack as argument */
	se_mtlr	r4
	se_blrl
	e_b		_ppc_return_resched

	.popsection

/**************************************************************************/


/*
 * IVOR setup (kept in .text.init)
 */
	.pushsection .text.init, "ax"

ppc_set_ivors:
#if (defined MPC5748G)
	/* Calypso uses fix offsets for exception handlers instead of the IVOR
	 * registers. */
	lwi        r3, _hardware_exception_table
	mtspr      SPR_IVPR, r3
#else
	/* set exception handling vectors */
	e_li	r3, _ivor_0@l
	mtspr	SPR_IVOR0, r3
	e_li	r3, _ivor_1@l
	mtspr	SPR_IVOR1, r3
	e_li	r3, _ivor_2@l
	mtspr	SPR_IVOR2, r3
	e_li	r3, _ivor_3@l
	mtspr	SPR_IVOR3, r3
	e_li	r3, _ivor_4@l
	mtspr	SPR_IVOR4, r3
	e_li	r3, _ivor_5@l
	mtspr	SPR_IVOR5, r3
	e_li	r3, _ivor_6@l
	mtspr	SPR_IVOR6, r3
	e_li	r3, _ivor_7@l
	mtspr	SPR_IVOR7, r3
	e_li	r3, _ivor_8@l
	mtspr	SPR_IVOR8, r3
	e_li	r3, _ivor_9@l
	mtspr	SPR_IVOR9, r3
	e_li	r3, _ivor_10@l
	mtspr	SPR_IVOR10, r3
	e_li	r3, _ivor_11@l
	mtspr	SPR_IVOR11, r3
	e_li	r3, _ivor_12@l
	mtspr	SPR_IVOR12, r3
	e_li	r3, _ivor_13@l
	mtspr	SPR_IVOR13, r3
	e_li	r3, _ivor_14@l
	mtspr	SPR_IVOR14, r3
	e_li	r3, _ivor_15@l
	mtspr	SPR_IVOR15, r3

	/* e200 / e500 SPE */
	e_li	r3, _ivor_32@l
	mtspr	SPR_IVOR32, r3
	e_li	r3, _ivor_33@l
	mtspr	SPR_IVOR33, r3
	e_li	r3, _ivor_34@l
	mtspr	SPR_IVOR34, r3


	e_lis	r3, _ivor_0@h
	mtspr	SPR_IVPR, r3
#endif
	se_blr

	.popsection


/**************************************************************************/


/*
 * save SPE context
 *
 * void ppc_spe_save(struct arch_fpu_frame *fpu);
 */
	.balign 16
ppc_spe_save:
	/* save high parts of r0..31 */
	i = 0
	.rept 32
		evmergehi	r0, r0, i
		e_stw			r0, i*8(r3)
		i = i + 1
	.endr
	/* save accumulator */
	evxor		evr0, evr0, evr0
	evmwumiaa	evr0, evr0, evr0
	evstdd		evr0, 128(r3)
	se_blr


/*
 * restore SPE context
 *
 * void ppc_spe_restore(struct arch_fpu_frame *fpu);
 */
	.balign 16
ppc_spe_restore:
	/* load accumulator */
	evldd		evr0, 128(r3)
	evmra		evr0, evr0
	/* load high parts of r0..31 */
	i = 0
	.rept 32
		e_lwz			r0, i*8(r3)
		evmergelo	i, r0, i
		i = i + 1
	.endr
	se_blr

/*==================[Jump Tables]=============================================*/

#if (defined MPC5748G)
/*------------------[Hardware exception table]--------------------------------*/

/* Base location of the vector table, aligned to a 256-byte boundary
   The address of this table goes into the IVPR register.
   See "MPC5748G Reference Manual, Rev 4, 07/2015",
   chapter "60.8.5 Interrupt definitions".
 */

.balign 256
_hardware_exception_table:

/* Offset 0x00 */
_Critical_Input_Interrupt:
    e_b _ivor_0

/* Offset 0x10 */
//.skip 16
.balign 16
_Machine_Check_interrupt:
    e_b _ivor_1

/* Offset 0x20 */
.balign 16
_Data_Storage_interrupt:
    e_b _ivor_2

/* Offset 0x30 */
.balign 16
_Instruction_Storage_interrupt:
    e_b _ivor_3

/* Offset 0x40 */
/* An External Input exception is signaled to the processor by the assertion
   of an interrupt from the interrupt controller. */
.balign 16
_External_Input_interrupt:
    e_b _ivor_4

/* Offset 0x50 */
.balign 16
_Alignment_interrupt:
    e_b _ivor_5

/* Offset 0x60 */
.balign 16
_Program_interrupt:
    e_b _ivor_6

/* Offset 0x70 */
.balign 16
_Performance_Monitor_interrupt:
    e_b _Performance_Monitor_interrupt

/* Offset 0x80 */
.balign 16
_System_Call_interrupt:
    e_b _ivor_8

/* Offset 0x90 */
.balign 16
_Debug_interrupt:
    e_b _ivor_15

/* Offset 0xA0 */
.balign 16
_EFPU_Data_interrupt:
    e_b _ivor_33

/* Offset 0xB0 */
.balign 16
_EFPU_Round_interrupt:
    e_b _ivor_34
/*------------------[End of hardware exception table]-------------------------*/
#endif /* if (defined MPC5748G) */
	
/**************************************************************************/


/*
 * Import system call table
 *
 * NOTE: the last entry in the syscall table is sys_ni_syscall
 */
	.balign 16
_syscalls:
#define __SYSCALL(x) .long x ;
#include "../../../src/syscall_table.S"
