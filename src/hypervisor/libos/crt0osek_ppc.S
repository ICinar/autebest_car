/*
 * crt0osek_ppc.S
 *
 * OSEK runtime for PowerPC
 *
 * tjordan, 2014-09-19: initial
 */

#include <syscalls.h>
#include <ppc_asm.h>
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
	/* remember func for later */
	mtlr	r3

	/* copy .data */
	lwi		r3, __rom_data_start
	lwi		r4, __data_start
	lwi		r5, __data_end
	b		2f

	/* word copy loop */
1:	lwz		r0, 0(r3)
	addi	r3, r3, 4
	stw		r0, 0(r4)
	addi	r4, r4, 4
2:	cmplw	r4, r5
	blt+	1b

	/* zero .bss */
	lwi		r4, __bss_start
	lwi		r5, __bss_end
	li		r0, 0
	b		4f

	/* word clear */
3:	stw		r0, 0(r4)
	addi	r4, r4, 4
4:	cmplw	r4, r5
	blt+	3b

	/* initialize .stack */
	lwi		r4, __stack_start
	lwi		r5, __stack_end
	lwi		r3, 0xA070BE57
	b		6f

	/* word write */
5:	stw		r3, 0(r4)
	addi	r4, r4, 4
6:	cmplw	r4, r5
	blt+	5b

	/* call startup hook */
	blrl

	li		r0, SYSCALL_TASK_TERMINATE
	sc
FUNC_EPILOG(__os_startup)

/*
 * void __os_task_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling task if it returns
 */
FUNC_PROLOG(__os_task_invoke)
	mtlr	r3
	blrl
	bl		_OsTaskCleanup
	/* SWS_Os_00052: implicitly terminate task */
	li		r0, SYSCALL_TASK_TERMINATE
	sc
FUNC_EPILOG(__os_task_invoke)

/*
 * void __os_isr_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling ISR if it returns
 */
FUNC_PROLOG(__os_isr_invoke)
	mtlr	r3
	blrl
	bl		_OsIsrCleanup
	li		r0, SYSCALL_TASK_TERMINATE
	sc
FUNC_EPILOG(__os_isr_invoke)

/*
 * void __os_hook_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling hook if it returns
 */
FUNC_PROLOG(__os_hook_invoke)
	mtlr	r3
	blrl
	li		r0, SYSCALL_TASK_TERMINATE
	sc
FUNC_EPILOG(__os_hook_invoke)
