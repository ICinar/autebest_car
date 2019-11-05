/*
 * entry_m.S
 *
 * ARMv7-M exception handling kernel entry/exit code for Cortex-M3/M4
 *
 * azuepke, 2015-06-24: cloned from ARMv7-A
 */

#include <syscalls.h>
#include <arm_cr.h>
#include <sched_state.h>
#include <arch_state.h>
#include <assembler.h>


	.global arm_vector_nmi
	.global arm_vector_hard
	.global arm_vector_memory
	.global arm_vector_bus
	.global arm_vector_usage
	.global arm_vector_svc
	.global arm_vector_debug
	.global arm_vector_irq
	.global arch_switch_to_kernel_stack

	.text
	.syntax unified

	/* ARM low-level exception handlers, called from the vector page */

/**
 * Syscall entry
 */
FUNC(arm_vector_svc)
	/* save PSP in task registers */
	movw	r12, #:lower16:__sched_state
	movt	r12, #:upper16:__sched_state
	ldr		r0, [r12, #SCHED_STATE_REGS]

	mrs		r12, psp
	str		r12, [r0, #4*8]

	/* restore original r0..r3 from PSP in case a late-arriving IRQ interrupted us */
	ldm		r12, {r0-r3}

	/* do syscall[r7] within NUM_SYSCALLS limits */
	cmp		r7, #NUM_SYSCALLS
	movcs	r7, #NUM_SYSCALLS

	/* call handler with a stack frame for r4 + r5 */
	adr		r12, _syscalls
	ldr		r12, [r12, r7, lsl #2]
	push	{r4, r5}
	blx		r12
	pop		{r4, r5}

	/* check for rescheduling */
	/* reload PSP in any case (task may have re-activated self) */
	movw	r12, #:lower16:__sched_state
	movt	r12, #:upper16:__sched_state
	ldr		r0, [r12, #SCHED_STATE_REGS]
	ldr		r1, [r12, #SCHED_STATE_RESCHEDULE]
	cmp		r1, #0
	bne		2f

1:	/* return */
	ldr		r12, [r0, #4*8]
	msr		psp, r12
	ldr		pc, [r0, #4*9]
	/* NOT REACHED */

2:	/* save all registers for rescheduling */
	stm		r0, {r4-r11}
	/* all user registers now saved */

_arm_return_resched:
	cpsid	i
	bl		sched_schedule
	/* returns a new register context to return to in r0 */

_arm_return:
	/* restore all registers incl PSP and exception lr and return */
	ldm		r0, {r4-r11, r12, lr}
	msr		psp, r12
	cpsie	i
	bx		lr


/**
 * Interrupt entry
 */
FUNC(arm_vector_irq)
	/* NOTE: Interrupts can be interrupted by higher priority interrupts,
	 * so we disable interrupts globally while in interrupt handling.
	 * Interrupts are re-enabled again on return from an exception.
	 */
	cpsid	i

	/* r0 contains interrupt */
	mrs		r0, ipsr
	bl		arm_irq_handler

	/* check for rescheduling */
	movw	r12, #:lower16:__sched_state
	movt	r12, #:upper16:__sched_state
	ldr		r0, [r12, #SCHED_STATE_REGS]
	ldr		r1, [r12, #SCHED_STATE_RESCHEDULE]
	cmp		r1, #0
	bne		2f

1:	/* direct return (PSP unchanged) */
	cpsie	i
	ldr		pc, [r0, #4*9]
	/* NOT REACHED */

2:	/* save all registers for rescheduling */
	mrs		r12, psp
	stm		r0, {r4-r11, r12}
	b		_arm_return_resched


/**
 * Memory fault entry
 */
FUNC(arm_vector_memory)
	tst		lr, #0x8
	beq		2f

1:	/* memory fault in thread mode */
	/* use task register frame */
	movw	r12, #:lower16:__sched_state
	movt	r12, #:upper16:__sched_state
	ldr		r0, [r12, #SCHED_STATE_REGS]
	mrs		r12, psp
	stm		r0, {r4-r11, r12, lr}
	bl		arm_memory_handler_user
	b		_arm_return_resched

2:	/* memory fault in handler mode */
	/* create a register frame on stack */
	mov		r12, sp
	push	{r4-r11, r12, lr}
	mov		r0, sp
	bl		arm_memory_handler_kern

	/* PSP unchanged */
	pop		{r4-r11, r12, lr}
	mov		sp, r12
	bx		lr


/**
 * Usage fault entry
 */
FUNC(arm_vector_usage)
	tst		lr, #0x8
	beq		2f

1:	/* usage fault in thread mode */
	/* use task register frame */
	movw	r12, #:lower16:__sched_state
	movt	r12, #:upper16:__sched_state
	ldr		r0, [r12, #SCHED_STATE_REGS]
	mrs		r12, psp
	stm		r0, {r4-r11, r12, lr}
	bl		arm_usage_handler_user
	b		_arm_return_resched

2:	/* usage fault in handler mode */
	/* create a register frame on stack */
	mov		r12, sp
	push	{r4-r11, r12, lr}
	mov		r0, sp
	bl		arm_usage_handler_kern

	/* PSP unchanged */
	pop		{r4-r11, r12, lr}
	mov		sp, r12
	bx		lr


/**
 * Bus fault entry
 */
FUNC(arm_vector_bus)
	tst		lr, #0x8
	beq		2f

1:	/* bus fault in thread mode */
	/* use task register frame */
	movw	r12, #:lower16:__sched_state
	movt	r12, #:upper16:__sched_state
	ldr		r0, [r12, #SCHED_STATE_REGS]
	mrs		r12, psp
	stm		r0, {r4-r11, r12, lr}
	bl		arm_bus_handler_user
	b		_arm_return_resched

2:	/* bus fault in handler mode */
	/* create a register frame on stack */
	mov		r12, sp
	push	{r4-r11, r12, lr}
	mov		r0, sp
	bl		arm_bus_handler_kern

	/* PSP unchanged */
	pop		{r4-r11, r12, lr}
	mov		sp, r12
	bx		lr


/**
 * Debug Monitor entry
 */
FUNC(arm_vector_debug)
	tst		lr, #0x8
	beq		2f

1:	/* debug monitor fault in thread mode */
	/* use task register frame */
	movw	r12, #:lower16:__sched_state
	movt	r12, #:upper16:__sched_state
	ldr		r0, [r12, #SCHED_STATE_REGS]
	mrs		r12, psp
	stm		r0, {r4-r11, r12, lr}
	bl		arm_debug_handler_user
	b		_arm_return_resched

2:	/* debug monitor fault in handler mode */
	/* create a register frame on stack */
	mov		r12, sp
	push	{r4-r11, r12, lr}
	mov		r0, sp
	bl		arm_debug_handler_kern

	/* PSP unchanged */
	pop		{r4-r11, r12, lr}
	mov		sp, r12
	bx		lr


/**
 * NMI entry
 */
FUNC(arm_vector_nmi)
	/* we can be anywere on the main or process stack. switch to NMI stack */
	mov		r12, sp
	movw	r0, #:lower16:__nmi_stack
	movt	r0, #:upper16:__nmi_stack
	mov		sp, r0

	/* create a register frame on stack */
	push	{r4-r11, r12, lr}
	mov		r0, sp
	bl		arm_nmi_handler

	/* PSP unchanged */
	pop		{r4-r11, r12, lr}
	mov		sp, r12
	bx		lr


/**
 * Hard fault entry
 */
FUNC(arm_vector_hard)
	tst		lr, #0x8
	beq		2f

1:	/* bus fault in thread mode */
	/* use task register frame */
	movw	r12, #:lower16:__sched_state
	movt	r12, #:upper16:__sched_state
	ldr		r0, [r12, #SCHED_STATE_REGS]
	mrs		r12, psp
	stm		r0, {r4-r11, r12, lr}
	bl		arm_hard_handler_user
	b		_arm_return_resched

2:	/* bus fault in handler mode */
	/* create a register frame on stack */
	mov		r12, sp
	push	{r4-r11, r12, lr}
	mov		r0, sp
	bl		arm_hard_handler_kern

	/* PSP unchanged */
	pop		{r4-r11, r12, lr}
	mov		sp, r12
	bx		lr


/*
 * switch to the kernel stack and invoke the kernel function
 * void arch_switch_to_kernel_stack(void (*func)(void *)) __noreturn;
 *
 */
	.pushsection .text.init, "ax"
FUNC(arch_switch_to_kernel_stack)
	mov		lr, r0

	/* we currently are in privileged thread mode on main stack */
	/* prepare main stack (MSP) for entry to handler mode */
	movw	r0, #:lower16:__kern_stack
	movt	r0, #:upper16:__kern_stack
	mov		sp, r0

	/* call func() with new stack as argument */
	blx		lr

	/* Kernel initialization is done.
	 * We now need to trick the CPU into the scheduler.
	 * But first switch to the idle stack set by arch_reg_frame_assign_idle()
	 */
	mov		r0, #CONTROL_SPSEL
	msr		control, r0
	isb

	/* We are still in privileged thread mode, but now use
	 * the thread stack (PSP). The stack pointer was implicitly switched.
	 */

	/* enable interrupts and exceptions */
	cpsie	if

	/* serve pending interrupts first: if we issue the SVC too early,
	 * the first user task will see a sys_abort().
	 * The first interrupt is served at the third nop.
	 */
	nop
	nop
	nop

	/* enforce scheduling (in handler mode) via a NULL-syscall */
	mov 	r7, #NUM_SYSCALLS
	svc		#0

	/* continue at board_idle() */
	b		board_idle

	.popsection


/*
 * Import system call table
 *
 * NOTE: the last entry in the syscall table is sys_ni_syscall
 */
	.balign 4
_syscalls:
#define __SYSCALL(x) .word x ;
#include "../../../src/syscall_table.S"
