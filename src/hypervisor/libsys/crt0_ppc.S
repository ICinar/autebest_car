/*
 * crt0_ppc.S
 *
 * Minimal runtime for PowerPC
 *
 * azuepke, 2014-06-03: initial
 */

#include <syscalls.h>
#include <ppc_asm.h>
#include <assembler.h>


/*
 * void __sys_startup(void (*func)(void)) __noreturn;
 *
 * setup data and bss, invoke func (startup hook)
 * and signale "initialization complete" at the end
 *
 * unfortunately, we need to do this in assembler,
 * otherwise we'd overwrite our own stack in a non-debug configuration
 *
 * NOTE: the routine expects that all labels are 64-bit aligned!
 */
FUNC_PROLOG(__sys_startup)
	/* remember func for later */
	mtlr	r3

	/* copy .data
	 *
	 * src = &__rom_data_start;
	 * dst = &__data_start;
	 * while (dst < &__data_end)
	 *     *dst++ = *src++;
	 */
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

	/* zero .bss
	 *
	 * dst = &__bss_start;
	 * while (dst < &__bss_end)
	 *     *dst++ = 0;
	 */
	lwi		r4, __bss_start
	lwi		r5, __bss_end
	li		r0, 0
	b		4f

	/* word clear */
3:	stw		r0, 0(r4)
	addi	r4, r4, 4
4:	cmplw	r4, r5
	blt+	3b

	/* call startup hook */
	blrl

	li		r0, SYSCALL_TASK_TERMINATE
	sc
FUNC_EPILOG(__sys_startup)


/*
 * void __sys_task_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func and terminate calling task
 */
FUNC_PROLOG(__sys_task_invoke)
	mtlr	r3
	blrl
	li		r0, SYSCALL_TASK_TERMINATE
	sc
FUNC_EPILOG(__sys_task_invoke)
