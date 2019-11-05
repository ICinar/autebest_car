/*
 * entry.S
 *
 * ARM exception handling kernel entry/exit code
 *
 * azuepke, 2013-09-12: initial
 * azuepke, 2013-11-24: reworked for MPU and single kernel stack
 * azuepke, 2014-09-02: support for IRQ stack
 * azuepke, 2015-01-05: optimized register layout
 */

#include <syscalls.h>
#include <arm_cr.h>
#include <sched_state.h>
#include <arch_state.h>
#include <assembler.h>

	.global arm_vector_undef
	.global arm_vector_pabt
	.global arm_vector_dabt
	.global arm_vector_irq
	.global arm_vector_fiq
	.global arm_vector_svc
	.global arm_switch_to_kernel_stack

	.text
	.arm

	/* helper macros */

/**
 * switch to kernel stacks
 * - load sched_state.schedule into r11
 * - load new kernel / IRQ stack pointer
 */
.macro load_r11_and_switch_to_kernel_stack
	mrc		p15, 0, r11, c13, c0, 4		/* read tls2 */
	ldr		sp, [r11, #ARCH_STATE_KERN_STACK]
.endm


/******************************************************************************/

	/* ARM low-level exception handlers, called from the vector page */

/**
 * UNDEFINED INSTRUCTION dispatcher
 *
 * - for exceptions in user mode:
 *   - saves registers
 *   - switches to the kernel stack
 *   - calls arm_undef_handler_user
 *   - continues at _arm_return_resched
 *
 * - for exceptions in kernel mode:
 *   - saves registers
 *   - calls arm_undef_handler_kern
 *   - continues at _arm_return_kern
 */
FUNC(arm_vector_undef)
	/* called in UNDEF mode */
	/* fixup caller PC */
	mrs		sp, spsr
	tst		sp, #CPSR_T
	subne	lr, #2
	subeq	lr, #4
	srsdb	sp!, #CPSR_MODE_SVC

	/* PC + CPSR saved */
	tst		sp, #0xf
	cps		#CPSR_MODE_SVC
	sub		sp, #4*15
	/* now in SVC mode, with stack pointer at beginning of stack frame */
	bne		2f

1:	/* undefined instruction in user mode */
	/* save user mode registers */
	stm		sp, {r0 - r14}^

	/* switch stack, call handler */
	mov		r0, sp
	load_r11_and_switch_to_kernel_stack
	blx		arm_undef_handler_user
	b		_arm_return_resched

2:	/* undefined instruction in kernel mode -> try recovery */
	/* save kernel mode registers */
	stm		sp, {r0 - r12}
	/* save original lr */
	str		lr, [sp, #4*14]
	/* NOTE: sp is not updated and still points to user mode! */

	/* call handler, keep stack */
	mov		r0, sp
	blx		arm_undef_handler_kern
	/* FALL-THROUGH to _arm_return_kern */

/*
 * common return path for all exceptions in kernel mode:
 * restore all registers and return to kernel mode
 * the code below assumes that the exception registers are saved on stack
 */
_arm_return_kern:
	ldm		sp, {r0 - r12}
	/* we don't restore SP, as SP is the kernel SP already */
	ldr		lr, [sp, #4*14]
	add		sp, #4*15
	rfeia	sp!


/******************************************************************************/


/**
 * PREFETCH ABORT dispatcher
 *
 * - for exceptions in user mode:
 *   - saves registers
 *   - switches to the kernel stack
 *   - calls arm_pabt_handler_user
 *   - continues at _arm_return_resched
 *
 * - for exceptions in kernel mode:
 *   - saves registers
 *   - calls arm_pabt_handler_kern
 *   - continues at _arm_return_kern
 */
FUNC(arm_vector_pabt)
	/* called in ABORT mode */
	sub		lr, #4
	srsdb	sp!, #CPSR_MODE_SVC
	mrs		sp, spsr

	/* PC + CPSR saved */
	tst		sp, #0xf
	cps		#CPSR_MODE_SVC
	sub		sp, #4*15
	/* now in SVC mode, with stack pointer at beginning of stack frame */
	bne		4f

3:	/* prefetch abort in user mode */
	stm		sp, {r0 - r14}^

	/* switch stack, call handler */
	mov		r0, sp
	load_r11_and_switch_to_kernel_stack
	/* get IFAR and IFSR */
	mrc		p15, 0, r1, c6, c0, 2
	mrc		p15, 0, r2, c5, c0, 1
	cpsie	a
	blx		arm_pabt_handler_user
	b		_arm_return_resched

4:	/* prefetch abort in kernel mode -> panic */
	/* save kernel mode registers */
	stm		sp, {r0 - r12}
	/* save original lr */
	str		lr, [sp, #4*14]
	/* NOTE: sp is not updated and still points to user mode! */

	/* call handler, keep stack */
	mov		r0, sp
	/* get IFAR and IFSR */
	mrc		p15, 0, r1, c6, c0, 2
	mrc		p15, 0, r2, c5, c0, 1
	blx		arm_pabt_handler_kern
	b		_arm_return_kern


/******************************************************************************/

/**
 * DATA ABORT dispatcher
 *
 * - for exceptions in user mode:
 *   - saves registers
 *   - switches to the kernel stack
 *   - calls arm_dabt_handler_user
 *   - continues at _arm_return_resched
 *
 * - for exceptions in kernel mode:
 *   - saves registers
 *   - calls arm_dabt_handler_kern
 *   - continues at _arm_return_kern
 */
FUNC(arm_vector_dabt)
	/* called in ABORT mode */
	sub		lr, #8
	srsdb	sp!, #CPSR_MODE_SVC
	mrs		sp, spsr

	/* PC + CPSR saved */
	tst		sp, #0xf
	cps		#CPSR_MODE_SVC
	sub		sp, #4*15
	/* now in SVC mode, with stack pointer at beginning of stack frame */
	bne		6f

5:	/* data abort in user mode */
	stm		sp, {r0 - r14}^

	/* switch stack, call handler */
	mov		r0, sp
	load_r11_and_switch_to_kernel_stack
	/* get DFAR and DFSR */
	mrc		p15, 0, r1, c6, c0, 0
	mrc		p15, 0, r2, c5, c0, 0
	cpsie	a
	blx		arm_dabt_handler_user
	b		_arm_return_resched

6:	/* data abort in kernel mode -> try exception recovery */
	/* save kernel mode registers */
	stm		sp, {r0 - r12}
	/* save original lr */
	str		lr, [sp, #4*14]
	/* NOTE: sp is not updated and still points to user mode! */

	/* call handler, keep stack */
	mov		r0, sp
	/* get DFAR and DFSR */
	mrc		p15, 0, r1, c6, c0, 0
	mrc		p15, 0, r2, c5, c0, 0
	blx		arm_dabt_handler_kern
	b		_arm_return_kern


/******************************************************************************/

/**
 * IRQ dispatcher
 *
 * - for exceptions in user mode:
 *   - saves registers (on SVC stack == register save area)
 *   - always switches to kernel stack
 *   - calls arm_irq_handler_user
 *   - continues at _arm_return_resched
 *
 * - for exceptions in kernel mode:
 *   - gets current register save area from sched.regs
 *   - saves registers
 *   - always switches to kernel stack
 *   - calls arm_irq_handler_kern
 *   - continues at _arm_return_resched
 */
FUNC(arm_vector_irq)
	/* called in IRQ mode */
	sub		lr, #4
	mrs		sp, spsr
	tst		sp, #0xf
	bne 	8f

7:	/* IRQ in user mode */
	/* save registers in register save area (== SVC stack) */
	srsdb	sp!, #CPSR_MODE_SVC
	/* PC + CPSR saved */
	cpsie	a, #CPSR_MODE_SVC
	sub		sp, #4*15

	/* now in SVC mode, with stack pointer at beginning of stack frame */
	stm		sp, {r0 - r14}^

	/* switch to kernel stack, call handler */
	mov		r0, sp
	load_r11_and_switch_to_kernel_stack
	blx		arm_irq_handler
	b		_arm_return_resched

8:	/* IRQ in kernel mode */
	/* get pointer to register save area */
	mrc		p15, 0, sp, c13, c0, 4		/* read tls2 */
	ldr		sp, [sp, #SCHED_STATE_REGS]

	/* save kernel mode registers */
	stm		sp, {r0 - r12}
	mov		r0, sp

	/* save original pc + cpsr */
	mrs		r1, spsr
	str		lr, [r0, #4*15]
	str		r1, [r0, #4*16]

	/* switch to SVC mode */
	cpsie	a, #CPSR_MODE_SVC

	/* save original sp + lr */
	str		sp, [r0, #4*13]
	str		lr, [r0, #4*14]

	/* switch to kernel stack, call handler */
	load_r11_and_switch_to_kernel_stack
	blx		arm_irq_handler
	b		_arm_return_resched


/******************************************************************************/

/**
 * FIQ dispatcher
 */
FUNC(arm_vector_fiq)
	/* FIQs have a dedicated stack */
	sub		lr, #4
	srsdb	sp!, #CPSR_MODE_FIQ
	sub		sp, #4*15
	stm		sp, {r0 - r7}
	mov		r0, sp
	blx		arm_fiq_handler
	ldm		sp, {r0 - r7}
	add		sp, #4*15
	rfeia	sp!


/******************************************************************************/

/**
 * system call dispatcher
 *
 * NOTE: Assumed to be called in user mode!
 */
FUNC(arm_vector_svc)
	/* no fixup of caller PC required */

	/* save user SP, LR, PC, SPSR only, R12 is scratch */
	srsdb	sp!, #CPSR_MODE_SVC
	sub		sp, #4*2
	stm		sp, {r13, lr}^

	/* switch to kernel stack */
	mrc		p15, 0, r12, c13, c0, 4		/* read tls2 */
	ldr		sp, [r12, #ARCH_STATE_KERN_STACK]

	/* do syscall[r7] within NUM_SYSCALLS limits */
	cmp		r7, #NUM_SYSCALLS
	movcs	r7, #NUM_SYSCALLS

	/* call handler with a stack frame for r4 + r5 */
	adr		r12, _syscalls
	ldr		r12, [r12, r7, lsl #2]
	push	{r4, r5}
	blx		r12

	/* check for rescheduling */
	mrc		p15, 0, r12, c13, c0, 4		/* read tls2 */
	ldr		r1, [r12, #SCHED_STATE_RESCHEDULE]
	ldr		r2, [r12, #SCHED_STATE_REGS]
	cmp		r1, #0
	bne		_arm_call_scheduler_svc

	/* restore r0..r3 (return code plus additional output registers) */
	mov		sp, r2
	ldrd	r0, r1, [sp, #4*0]
	ldrd	r2, r3, [sp, #4*2]

	/* r12 pointer to sched_state (const per task) */

	/* restore user SP, LR, PC, and SPSR from the save area */
	add		sp, #4*13	/* pop over r0..r12 */
	ldm		sp, {r13, lr}^
	add		sp, #4*2
	rfeia	sp!

_arm_call_scheduler_svc:
	/* undo frame for r4+r5 on stack */
	add		sp, #8

	/* save remaining registers */
	add		r1, r2, #4*4
	stm		r1, {r4 - r11}
	/* r12 is scratch */
	/* FALL-THROUGH */

/*
 * common return path for all exceptions in user mode (and interrupts in kernel mode)
 * unconditionally reschedule (to get a valid register context for return)
 */
_arm_return_resched:
	/* still on the kernel stack, invoke the scheduler */
	blx		sched_schedule
	/* returns a new register context to return to in r0 */
	mov		sp, r0

	/* return from all kind of exceptions (both user and kernel mode) */
	ldm		sp, {r0 - r14}^
	ldr		lr, [sp, #4*14]
	add		sp, #4*15
	rfeia	sp!

/*
 * switch to the kernel stack and invoke the kernel function
 * void arm_switch_to_kernel_stack(unsigned long stack, void (*func)(void *)) __noreturn;
 *
 */
	.pushsection .text.init, "ax"
FUNC(arm_switch_to_kernel_stack)
	/* switch stack */
	mov		sp, r0

	/* call func() with new stack as argument */
	blx		r1
	b		_arm_return_resched

	.popsection

/******************************************************************************/


/*
 * Import system call table
 *
 * NOTE: the last entry in the syscall table is sys_ni_syscall
 */
	.balign 16
_syscalls:
#define __SYSCALL(x) .word x ;
#include "../../../src/syscall_table.S"
