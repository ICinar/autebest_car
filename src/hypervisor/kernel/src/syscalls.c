/*
 * syscalls.c
 *
 * System call implementations
 *
 * azuepke, 2013-04-07: initial
 * azuepke, 2013-05-17: more system call dummies
 * azuepke, 2013-08-15: added task and mapping syscalls
 * azuepke, 2013-08-26: split into multiple files
 * azuepke, 2013-11-24: initial MPU version
 */

#include <kernel.h>
#include <arch.h>
#include <hv_compiler.h>
#include <hv_error.h>
#include <board.h>


__tc_fastcall void sys_putchar(const char c);
__tc_fastcall void sys_ni_syscall(void);
__tc_fastcall void sys_cpu_id(void);
__tc_fastcall void sys_null(void);


/** putchar syscall */
void sys_putchar(const char c)
{
	unsigned int err;

	err = board_putc(c);

	SET_RET(err);
}

/** "not implemented" syscall, just returns an error */
void sys_ni_syscall(void)
{
	SET_RET(E_OS_NOFUNC);
}

/** get current CPU ID */
void sys_cpu_id(void)
{
	unsigned int cpu_id;

	cpu_id = arch_cpu_id();

	SET_RET(cpu_id);
}

/** NULL system call */
// FIXME: remove!
void sys_null(void)
{
	/* a real NULL-syscall */
}
