/*
 * crt0osek_ppc_vle.S
 *
 * OSEK runtime for PowerPC
 *
 * lberaru, 2015-08-11: cloned from original
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
	e_b		2f

	/* word copy loop */
1:	e_lwz		r0, 0(r3)
	e_add16i	r3, r3, 4
	se_stw		r0, 0(r4)
	e_add16i	r4, r4, 4
2:	cmplw	r4, r5
	e_blt	1b

	/* zero .bss */
	lwi		r4, __bss_start
	lwi		r5, __bss_end
	e_li	r0, 0
	e_b		4f

	/* word clear */
3:	se_stw		r0, 0(r4)
	e_add16i	r4, r4, 4
4:	cmplw	r4, r5
	e_blt	3b

	/* initialize .stack */
	lwi		r4, __stack_start
	lwi		r5, __stack_end
	lwi		r3, 0xA070BE57
	e_b		6f

	/* word write */
5:	se_stw		r3, 0(r4)
	e_add16i	r4, r4, 4
6:	cmplw	r4, r5
	e_blt	5b

	/* call startup hook */
	se_blrl

	lwi		r0, SYSCALL_TASK_TERMINATE
	se_sc
FUNC_EPILOG(__os_startup)

/*
 * void __os_task_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling task if it returns
 */
FUNC_PROLOG(__os_task_invoke)
	mtlr	r3
	se_blrl
	e_bl		_OsTaskCleanup
	/* SWS_Os_00052: implicitly terminate task */
	lwi		r0, SYSCALL_TASK_TERMINATE
	se_sc
FUNC_EPILOG(__os_task_invoke)

/*
 * void __os_isr_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling ISR if it returns
 */
FUNC_PROLOG(__os_isr_invoke)
	mtlr	r3
	se_blrl
	e_bl		_OsIsrCleanup
	lwi		r0, SYSCALL_TASK_TERMINATE
	se_sc
FUNC_EPILOG(__os_isr_invoke)

/*
 * void __os_hook_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func, clean up and terminate calling hook if it returns
 */
FUNC_PROLOG(__os_hook_invoke)
	mtlr	r3
	se_blrl
	lwi		r0, SYSCALL_TASK_TERMINATE
	se_sc
FUNC_EPILOG(__os_hook_invoke)
