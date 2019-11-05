#include <syscalls.h>
#include <assembler.h>


/*
 * void __posix_startup(void) __noreturn;
 *
 * setup data and bss, invoke func (startup hook)
 * and signale "initialization complete" at the end
 *
 * unfortunately, we need to do this in assembler,
 * otherwise we'd overwrite our own stack in a non-debug configuration
 *
 * NOTE: the routine expects that all labels are 64-bit aligned!
 */
FUNC_PROLOG(__posix_startup)
	/* copy .data
	 *
	 * src = &__rom_data_start;
	 * dst = &__data_start;
	 * while (dst < &__data_end)
	 *     *dst++ = *src++;
	 */
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

	/* continue in C code */
	jl		__posix_init
	/* DOES NOT RETURN */
FUNC_EPILOG(__posix_startup)
