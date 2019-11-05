/*
 * syscall.h
 *
 * PowerPC specific syscall definitions
 * NOTE: this file is included from the assembler syscall stubs
 *
 * azuepke, 2014-06-03: initial
 * azuepke, 2015-05-08: new IN/OUT syscall scheme
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <assembler.h>
#include <ppc_asm.h>

/*
 * syscall args:
 *   uint32_t r3, r4, r5, r6, r7, r8
 *
 * NOTE: all arguments are passed in registers following the standard ABI!
 *
 * syscall number:
 *   uint32_t r0;
 *
 * on return, the error code is kept in r3 respective r3/r4 for 64-bit values.
 * additionally, we map OUT1 to r4, OUT2 to r5, and OUT3 to r6.
 */

.macro _SYSCALL_IN6, name
#ifdef VLE_ON
	lwi		r0, \name
	se_sc
	se_blr
#else
	li		r0, \name
	sc
	blr
#endif
.endm

#define _SYSCALL_IN0		_SYSCALL_IN6
#define _SYSCALL_IN0_RET64	_SYSCALL_IN6
#define _SYSCALL_IN1		_SYSCALL_IN6
#define _SYSCALL_IN2		_SYSCALL_IN6
#define _SYSCALL_IN3		_SYSCALL_IN6
#define _SYSCALL_IN4		_SYSCALL_IN6
#define _SYSCALL_IN5		_SYSCALL_IN6

.macro _SYSCALL_IN1_OUT1, name
#ifdef VLE_ON
	e_stwu	r1, -16(r1)
	e_stw	r4, 8(r1)
	lwi		r0, \name
	se_sc
	/* save OUT1 */
	e_lwz		r11, 8(r1)
	e_stw		r4, 0(r11)
	e_add16i    r1, r1, 16
	se_blr
#else
	stwu	r1, -16(r1)
	stw		r4, 8(r1)
	li		r0, \name
	sc
	/* save OUT1 */
	lwz		r11, 8(r1)
	stw		r4, 0(r11)
	addi    r1, r1, 16
	blr
#endif
.endm

.macro _SYSCALL_IN1_OUT2, name
#ifdef VLE_ON
	e_stwu	r1, -16(r1)
	e_stw		r4, 8(r1)
	e_stw		r5, 12(r1)
	lwi		r0, \name
	se_sc
	/* save OUT1 */
	e_lwz		r11, 8(r1)
	e_stw		r4, 0(r11)
	/* save OUT2 */
	e_lwz		r11, 12(r1)
	e_stw		r5, 0(r11)
	e_add16i    r1, r1, 16
    se_blr
#else
	stwu	r1, -16(r1)
	stw		r4, 8(r1)
	stw		r5, 12(r1)
	li		r0, \name
	sc
	/* save OUT1 */
	lwz		r11, 8(r1)
	stw		r4, 0(r11)
	/* save OUT2 */
	lwz		r11, 12(r1)
	stw		r5, 0(r11)
	addi    r1, r1, 16
	blr
#endif
.endm

.macro _SYSCALL_IN1_OUT3, name
#ifdef VLE_ON
	e_stwu	r1, -24(r1)
	e_stw	r4, 8(r1)
	e_stw	r5, 12(r1)
	e_stw	r6, 16(r1)
	lwi		r0, \name
	se_sc
	/* save OUT1 */
	e_lwz		r11, 8(r1)
	e_stw		r4, 0(r11)
	/* save OUT2 */
	e_lwz		r11, 12(r1)
	e_stw		r5, 0(r11)
	/* save OUT3 */
	e_lwz		r11, 16(r1)
	e_stw		r6, 0(r11)
	e_add16i    r1, r1, 24
	se_blr
#else
	stwu	r1, -24(r1)
	stw		r4, 8(r1)
	stw		r5, 12(r1)
	stw		r6, 16(r1)
	li		r0, \name
	sc
	/* save OUT1 */
	lwz		r11, 8(r1)
	stw		r4, 0(r11)
	/* save OUT2 */
	lwz		r11, 12(r1)
	stw		r5, 0(r11)
	/* save OUT3 */
	lwz		r11, 16(r1)
	stw		r6, 0(r11)
	addi    r1, r1, 24
	blr
#endif
.endm

.macro _SYSCALL_IN2_OUT1, name
#ifdef VLE_ON
	e_stwu	r1, -16(r1)
	e_stw	r5, 8(r1)
	lwi		r0, \name
	se_sc
	/* save OUT1 */
	e_lwz	r11, 8(r1)
	e_stw	r4, 0(r11)
	e_add16i	r1, r1, 16
	se_blr
#else
	stwu	r1, -16(r1)
	stw		r5, 8(r1)
	li		r0, \name
	sc
	/* save OUT1 */
	lwz		r11, 8(r1)
	stw		r4, 0(r11)
	addi    r1, r1, 16
	blr
#endif
.endm

.macro _SYSCALL_IN2_OUT2, name
#ifdef VLE_ON
	e_stwu	r1, -16(r1)
	e_stw	r5, 8(r1)
	e_stw	r6, 12(r1)
	lwi		r0, \name
	se_sc
	/* save OUT1 */
	e_lwz		r11, 8(r1)
	e_stw		r4, 0(r11)
	/* save OUT2 */
	e_lwz		r11, 12(r1)
	e_stw		r5, 0(r11)
	e_add16i    r1, r1, 16
	se_blr
#else
	stwu	r1, -16(r1)
	stw		r5, 8(r1)
	stw		r6, 12(r1)
	li		r0, \name
	sc
	/* save OUT1 */
	lwz		r11, 8(r1)
	stw		r4, 0(r11)
	/* save OUT2 */
	lwz		r11, 12(r1)
	stw		r5, 0(r11)
	addi    r1, r1, 16
	blr
#endif
.endm

.macro _SYSCALL_IN4_OUT1, name
#ifdef VLE_ON
	e_stwu	r1, -16(r1)
	e_stw	r7, 8(r1)
	lwi		r0, \name
	se_sc
	/* save OUT1 */
	e_lwz	r11, 8(r1)
	e_stw	r4, 0(r11)
	e_add16i	r1, r1, 16
	se_blr
#else
	stwu	r1, -16(r1)
	stw		r7, 8(r1)
	li		r0, \name
	sc
	/* save OUT1 */
	lwz		r11, 8(r1)
	stw		r4, 0(r11)
	addi    r1, r1, 16
	blr
#endif
.endm

#define _SYSCALL_PROLOG(x) FUNC_PROLOG(x)
#define _SYSCALL_EPILOG(x) FUNC_EPILOG(x)

#endif
