/*
 * syscall.h
 *
 * Tricore specific syscall definitions
 * NOTE: this file is included from the assembler syscall stubs
 *
 * azuepke, 2014-10-28: initial
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <assembler.h>

/*
 * syscall args:
 *   uint32_t d[4..7] and a[4..7]
 *
 * NOTE: up to 4 data words and 4 address words can be passed in registers
 * at the same time. When necessary, we cast data to address registers or vv
 * to get rid of stack usage in the kernel ABI.
 *
 * syscall number:
 *   encoded in the instruction and passed in TIN
 *
 * on return, the error code is kept in d2 respective d2/d3 for 64-bit values.
 * additionally, we map OUT1 to d3, OUT2 to d0, and OUT3 to d1.
 */

/*
 * NOTE: sys_wq_wait() and sys_task_create() are the only calls with more
 * than 4 arguments, but they use both pointers and integral arguments
 * that fit into both d4..7 and a4..7 *and* have no OUT arguments.
 * Also, the kernel does not clobber d4..7 and a4..7 during syscalls.
 */

.macro _SYSCALL_IN6, name
	syscall	\name
	ret
.endm
#define _SYSCALL_IN0		_SYSCALL_IN6
#define _SYSCALL_IN0_RET64	_SYSCALL_IN6
#define _SYSCALL_IN1		_SYSCALL_IN6
#define _SYSCALL_IN2		_SYSCALL_IN6
#define _SYSCALL_IN3		_SYSCALL_IN6
#define _SYSCALL_IN4		_SYSCALL_IN6
#define _SYSCALL_IN5		_SYSCALL_IN6

.macro _SYSCALL_IN4_OUT1, name
	syscall	\name
	/* save OUT1 */
	st.w	[%a4], %d3
	ret
.endm
#define _SYSCALL_IN1_OUT1	_SYSCALL_IN4_OUT1
#define _SYSCALL_IN2_OUT1	_SYSCALL_IN4_OUT1
#define _SYSCALL_IN3_OUT1	_SYSCALL_IN4_OUT1

.macro _SYSCALL_IN2_OUT2, name
	syscall	\name
	/* save OUT1 */
	st.w	[%a4], %d3
	/* save OUT2 */
	st.w	[%a5], %d0
	ret
.endm
#define _SYSCALL_IN1_OUT2	_SYSCALL_IN2_OUT2

.macro _SYSCALL_IN2_OUT3, name
	syscall	\name
	/* save OUT1 */
	st.w	[%a4], %d3
	/* save OUT2 */
	st.w	[%a5], %d0
	/* save OUT3 */
	st.w	[%a6], %d1
	ret
.endm
#define _SYSCALL_IN1_OUT3	_SYSCALL_IN2_OUT3

#define _SYSCALL_PROLOG(x) FUNC_PROLOG(x)
#define _SYSCALL_EPILOG(x) FUNC_EPILOG(x)

#endif
