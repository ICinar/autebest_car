/*
 * syscall.h
 *
 * ARM specific syscall definitions
 * NOTE: this file is included from the assembler syscall stubs
 *
 * azuepke, 2013-09-11: initial
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <assembler.h>

/*
 * syscall args:
 *   uint32_t r0, r1, r2, r3, r4, r5
 *
 * NOTE: the ABI defines up to 4 arguments passed in registers, we add another
 * two in r4/r5 to get rid of stack accesses in system calls.
 *
 * syscall number:
 *   uint32_t r7;
 *
 * on return, the error code is kept in r0 respective r0/r1 for 64-bit values.
 * additionally, we map OUT1 to r1, OUT2 to r2, and OUT3 to r3.
 */

/* ARM macros -- NOTE: due to the use of "#" in ARM assembler,
 * we cannot use the C preprocessor for these macros!
 */

/* NOTE: we always keep the stack 8 byte aligned */

.macro _SYSCALL_IN4, name
	push	{r7, lr}
	mov 	r7, #\name
	svc		#0
	pop		{r7, pc}
.endm

#define _SYSCALL_IN0		_SYSCALL_IN4
#define _SYSCALL_IN0_RET64	_SYSCALL_IN4
#define _SYSCALL_IN1		_SYSCALL_IN4
#define _SYSCALL_IN2		_SYSCALL_IN4
#define _SYSCALL_IN3		_SYSCALL_IN4

.macro _SYSCALL_IN6, name
	push	{r4, r5, r7, lr}
	ldrd	r4, r5, [sp, #16]
	mov 	r7, #\name
	svc		#0
	pop		{r4, r5, r7, pc}
.endm
#define _SYSCALL_IN5		_SYSCALL_IN6


.macro _SYSCALL_IN1_OUT1, name
	push	{r1, r2, r7, lr}
	mov 	r7, #\name
	svc		#0
	/* save OUT1 */
	ldr		r7, [sp, #0]
	str		r1, [r7]
	pop		{r1, r2, r7, pc}
.endm

.macro _SYSCALL_IN1_OUT2, name
	push	{r1, r2, r7, lr}
	mov 	r7, #\name
	svc		#0
	/* save OUT1 */
	ldr		r7, [sp, #0]
	str		r1, [r7]
	/* save OUT2 */
	ldr		r7, [sp, #4]
	str		r2, [r7]
	pop		{r1, r2, r7, pc}
.endm

.macro _SYSCALL_IN1_OUT3, name
	push	{r1, r2, r3, r4, r7, lr}
	mov 	r7, #\name
	svc		#0
	/* save OUT1 */
	ldr		r7, [sp, #0]
	str		r1, [r7]
	/* save OUT2 */
	ldr		r7, [sp, #4]
	str		r2, [r7]
	/* save OUT3 */
	ldr		r7, [sp, #8]
	str		r3, [r7]
	pop		{r1, r2, r3, r4, r7, pc}
.endm

.macro _SYSCALL_IN2_OUT1, name
	push	{r2, r3, r7, lr}
	mov 	r7, #\name
	svc		#0
	/* save OUT1 */
	ldr		r7, [sp, #0]
	str		r1, [r7]
	pop		{r2, r3, r7, pc}
.endm

.macro _SYSCALL_IN2_OUT2, name
	push	{r2, r3, r7, lr}
	mov 	r7, #\name
	svc		#0
	/* save OUT1 */
	ldr		r7, [sp, #0]
	str		r1, [r7]
	/* save OUT2 */
	ldr		r7, [sp, #4]
	str		r2, [r7]
	pop		{r2, r3, r7, pc}
.endm

.macro _SYSCALL_IN4_OUT1, name
	push	{r7, lr}
	mov 	r7, #\name
	svc		#0
	/* save OUT1 */
	ldr		r7, [sp, #8]
	str		r1, [r7]
	pop		{r7, pc}
.endm

#define _SYSCALL_PROLOG(x) FUNC_PROLOG(x)
#define _SYSCALL_EPILOG(x) FUNC_EPILOG(x)

#endif
