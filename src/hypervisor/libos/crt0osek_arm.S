/*
 * crt0osek_arm.S
 *
 * OSEK runtime for ARM
 *
 * tjordan, 2014-09-19: initial
 */

#include <syscalls.h>
#include <assembler.h>


	.text

/*
 * void __os_startup(void (*func)(void)) __noreturn;
 *
 * setup data and bss, invoke func (startup hook)
 * and signale "initialization complete" at the end
 *
 * unfortunately, we need to do this in assembler,
 * otherwise we'd overwrite our own stack in a non-debug configuration
 *
 * NOTE: the routine expects that all labels are 64-bit aligned!
 */
FUNC_PROLOG(__os_startup)
	/* copy .data */
	mov32	r1, __rom_data_start
	mov32	r2, __data_start
	mov32	r3, __data_end

	/* uses 64-bit load / stores, clobbers r4 and r5 */
	b		2f
1:	ldrd	r4, [r1], #8
	strd	r4, [r2], #8
2:	cmp		r2, r3
	bcc		1b

	/* zero .bss */
	mov32	r2, __bss_start
	mov32	r3, __bss_end

	mov		r4, #0
	mov		r5, #0
	b		4f
3:	strd	r4, [r2], #8
4:	cmp		r2, r3
	bcc		3b

	/* initialize .stack */
	mov32	r2, __stack_start
	mov32	r3, __stack_end

	mov32	r4, 0xA070BE57
	mov32	r5, 0xA070BE57
	b		6f
5:	strd	r4, [r2], #8
6:	cmp		r2, r3
	bcc		5b

	/* call startup hook */
	blx		r0

	mov		r7, #SYSCALL_TASK_TERMINATE
	svc		#0
FUNC_EPILOG(__os_startup)

/*
 * void __os_task_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling task if it returns
 */
FUNC_PROLOG(__os_task_invoke)
	blx		r0
	blx		_OsTaskCleanup
	/* SWS_Os_00052: implicitly terminate task */
	mov		r7, #SYSCALL_TASK_TERMINATE
	svc		#0
FUNC_EPILOG(__os_task_invoke)

/*
 * void __os_isr_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling ISR if it returns
 */
FUNC_PROLOG(__os_isr_invoke)
	blx		r0
	blx		_OsIsrCleanup
	mov		r7, #SYSCALL_TASK_TERMINATE
	svc		#0
FUNC_EPILOG(__os_isr_invoke)

/*
 * void __os_hook_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling hook if it returns
 */
FUNC_PROLOG(__os_hook_invoke)
	blx		r0
	mov		r7, #SYSCALL_TASK_TERMINATE
	svc		#0
FUNC_EPILOG(__os_hook_invoke)
