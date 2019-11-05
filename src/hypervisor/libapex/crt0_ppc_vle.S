/*
 * crt0_ppc_vle.S
 *
 * OSEK runtime for PowerPC
 *
 * azuepke, 2014-06-03: initial
 * azuepke, 2014-09-08: adapted to APEX
 * lberaru, 2015-08-2015: cloned and adapted to VLE syntax
 */

#include <syscalls.h>
#include <ppc_asm.h>
#include <assembler.h>


/*
 * void __apex_startup(void) __noreturn;
 *
 * setup data and bss, invoke func (startup hook)
 * and signale "initialization complete" at the end
 *
 * unfortunately, we need to do this in assembler,
 * otherwise we'd overwrite our own stack in a non-debug configuration
 *
 * NOTE: the routine expects that all labels are 64-bit aligned!
 */
FUNC_PROLOG(__apex_startup)
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
	e_b		2f

	/* word copy loop */
1:	e_lwz	r0, 0(r3)
	e_add16i	r3, r3, 4
	e_stw	r0, 0(r4)
	e_add16i	r4, r4, 4
2:	cmplw	r4, r5
	e_blt	1b

	/* zero .bss
	 *
	 * dst = &__bss_start;
	 * while (dst < &__bss_end)
	 *     *dst++ = 0;
	 */
	lwi	r4, __bss_start
	lwi	r5, __bss_end
	lwi	r0, 0
	e_b	4f

	/* word clear */
3:	e_stw		r0, 0(r4)
	e_add16i	r4, r4, 4
4:	cmplw	r4, r5
	e_blt	3b

	/* continue in C code */
	e_bl		__apex_init

	/* ABORT on return */
	lwi		r0, SYSCALL_ABORT
	se_sc
FUNC_EPILOG(__apex_startup)

/*
 * void __apex_proc_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func and terminate calling task
 */
FUNC_PROLOG(__apex_proc_invoke)
	mtctr	r3
	se_bctrl

	/* ABORT on return */
	lwi		r0, SYSCALL_ABORT
	se_sc
FUNC_EPILOG(__apex_proc_invoke)
