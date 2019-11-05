/*
 * crt0osek_tricore.S
 *
 * OSEK runtime for Tricore
 *
 * azuepke, 2014-12-16: initial
 */

#include <syscalls.h>
#include <assembler.h>


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
	movh.a	%a2, hi:__rom_data_start
	lea		%a2, [%a2] lo:__rom_data_start
	movh.a	%a3, hi:__data_start
	lea		%a3, [%a3] lo:__data_start
	movh.a	%a15, hi:__data_end
	lea		%a15, [%a15] lo:__data_end
	j		2f

	/* 64-bit copy loop */
1:	ld.d	%e0, [%a2+] 8
	st.d	[%a3+] 8, %e0
2:	lt.a	%d15, %a3, %a15
	jnz		%d15, 1b

	/* zero .bss
	 *
	 * dst = &__bss_start;
	 * while (dst < &__bss_end)
	 *     *dst++ = 0;
	 */
	movh.a	%a3, hi:__bss_start
	lea		%a3, [%a3] lo:__bss_start
	movh.a	%a15, hi:__bss_end
	lea		%a15, [%a15] lo:__bss_end
	mov		%e0, 0
	j		4f

	/* 64-bit clear loop */
3:	st.d	[%a3+] 8, %e0
4:	lt.a	%d15, %a3, %a15
	jnz		%d15, 3b

	/* initialize .stack */
	movh.a	%a3, hi:__stack_start
	lea		%a3, [%a3] lo:__stack_start
	movh.a	%a15, hi:__stack_end
	lea		%a15, [%a15] lo:__stack_end
	movh	%d0, hi:0xA070BE57
	addi	%d0, %d0, lo:0xA070BE57
	j		6f

	/* word write */
5:	st.w	[%a3+] 4, %d0
6:	lt.a	%d15, %a3, %a15
	jnz		%d15, 5b

	/* call startup hook */
	calli	%a4

	syscall	SYSCALL_TASK_TERMINATE
FUNC_EPILOG(__os_startup)

/*
 * void __os_task_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling task if it returns
 */
FUNC_PROLOG(__os_task_invoke)
	calli	%a4
	call	_OsTaskCleanup
	/* SWS_Os_00052: implicitly terminate task */
	syscall	SYSCALL_TASK_TERMINATE
FUNC_EPILOG(__os_task_invoke)

/*
 * void __os_isr_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling ISR if it returns
 */
FUNC_PROLOG(__os_isr_invoke)
	calli	%a4
	call	_OsIsrCleanup
	syscall	SYSCALL_TASK_TERMINATE
FUNC_EPILOG(__os_isr_invoke)

/*
 * void __os_hook_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling hook if it returns
 */
FUNC_PROLOG(__os_hook_invoke)
	calli	%a4
	syscall	SYSCALL_TASK_TERMINATE
FUNC_EPILOG(__os_hook_invoke)
