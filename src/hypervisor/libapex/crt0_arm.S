/*
 * crt0_arm.S
 *
 * OSEK runtime for ARM
 *
 * azuepke, 2014-03-01: initial
 * azuepke, 2014-03-02: merged into crt0.S
 * azuepke, 2014-06-03: renamed to crt0_arm.S
 * azuepke, 2014-09-08: adapted to APEX
 */

#include <syscalls.h>
#include <assembler.h>


	.text

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
	mov32	r1, __rom_data_start
	mov32	r2, __data_start
	mov32	r3, __data_end

	/* uses 64-bit load / stores, clobbers r4 and r5 */
	b		2f
1:	ldrd	r4, [r1], #8
	strd	r4, [r2], #8
2:	cmp		r2, r3
	bcc		1b

	/* zero .bss
	 *
	 * dst = &__bss_start;
	 * while (dst < &__bss_end)
	 *     *dst++ = 0;
	 */
	mov32	r2, __bss_start
	mov32	r3, __bss_end

	mov		r4, #0
	mov		r5, #0
	b		4f
3:	strd	r4, [r2], #8
4:	cmp		r2, r3
	bcc		3b

	/* continue in C code */
	blx		__apex_init

	/* ABORT on return */
	mov		r7, #SYSCALL_ABORT
	svc		#0
FUNC_EPILOG(__apex_startup)

/*
 * void __apex_proc_invoke(void (*func)(void)) __noreturn;
 *
 * invoke func and terminate calling task
 */
FUNC_PROLOG(__apex_proc_invoke)
	blx		r0

	/* ABORT on return */
	mov		r7, #SYSCALL_ABORT
	svc		#0
FUNC_EPILOG(__apex_proc_invoke)
