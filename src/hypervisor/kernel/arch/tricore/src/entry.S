/*
 * entry.S
 *
 * Architecture specific exception kernel entry/exit code
 *
 * azuepke, 2014-12-16: initial
 * azuepke, 2014-12-24: inline syscall jumptable and exception recovery
 */

#include <syscalls.h>
#include <tc_regs.h>
#include <sched_state.h>
#include <arch_state.h>
#include <arch_regs.h>
#include <tc_layout.h>

	.global tc_trap_table_start
	.global tc_trap_table_end
	.global tc_switch_to_kernel_stack

	.section .text.init, "ax"

/**************************************************************************/

	.balign 32
	/* the trap table is copied to CRAM at offset 0 */
	/* NOTE: all branches in the trap table use relative addressing,
	 * except when branching into flash (which uses absolute addressing).
	 */
tc_trap_table_start:

	/* at offset +0, we keep the TRAP vectors */
	.balign 32
_trap_class0:
	/* Class 0 -- MMU */
	mov		%d14, 0
	j		_trap_common_kernel

	.balign 32
_trap_class1:
	/* Class 1 -- Internal Protection */
	/* get PCXI and check if PIE is set. We test against UL which is 1 */
	mfcr	%d8, CSFR_PCXI
	and.t	%d9, %d8, 20, %d8, 21
	mov		%d14, 1
	jz		%d9, _trap_common_kernel
	j		_trap_common_user


	.balign 32
_trap_class2:
	/* Class 2 -- Instruction Errors */
	/* get PCXI and check if PIE is set. We test against UL which is 1 */
	mfcr	%d8, CSFR_PCXI
	and.t	%d9, %d8, 20, %d8, 21
	mov		%d14, 2
	jz		%d9, _trap_common_kernel
	j		_trap_common_user

	.balign 32
_trap_class3:
	/* Class 3 -- Context Management */
	jeq		%d15, TIN_FCD, _trap_fcd
	jeq		%d15, TIN_FCU, _trap_fcu
.L_trap_class3_cont:
	/* get PCXI and check if PIE is set. We test against UL which is 1 */
	mfcr	%d8, CSFR_PCXI
	and.t	%d9, %d8, 20, %d8, 21
	mov		%d14, 3
	jz		%d9, _trap_common_kernel
	j		_trap_common_user

	.balign 32
_trap_class4:
	/* Class 4 -- System Bus and Peripheral Errors */
	/* get PCXI and check if PIE is set. We test against UL which is 1 */
	mfcr	%d8, CSFR_PCXI
	and.t	%d9, %d8, 20, %d8, 21
	mov		%d14, 4
	jz		%d9, _trap_common_kernel
	j		_trap_common_user

	.balign 32
_trap_class5:
	/* Class 5 -- Assertion Traps */
	/* get PCXI and check if PIE is set. We test against UL which is 1 */
	mfcr	%d8, CSFR_PCXI
	and.t	%d9, %d8, 20, %d8, 21
	mov		%d14, 5
	jz		%d9, _trap_common_kernel
	j		_trap_common_user

	.balign 32
_trap_class6:
	/* Class 6 -- System Calls */
	j		_trap_sys

	.balign 32
_trap_class7:
	/* Class 7 -- NMI */
	/* the NMI handler starts immediately */
_trap_nmi:
	/* switch to NMI stack and save FCX + LCX in a8 */
	ld.a	%a10, [%a8] ARCH_STATE_NMI_STACK
	mfcr	%d8, CSFR_FCX
	mfcr	%d9, CSFR_LCX
	st.d	[%a8] ARCH_STATE_NMI_SAVED_FCX_LCX, %e8

	/* switch to NMI FCX + LCX */
	ld.d	%e12, [%a8] ARCH_STATE_NMI_FCX_LCX
	mtcr	CSFR_FCX, %d12
	mtcr	CSFR_LCX, %d13
	isync

	/* save LOWER */
	ld.a	%a13, [%a8] ARCH_STATE_NMI_CSA
	stlcx	[%a13] CTXT_CSA

	/* call tc_handler_nmi(PTR_LOWER, CX_UPPER) */
	mov.aa	%a4, %a13		/* LOWER */
	mfcr	%d4, CSFR_PCXI	/* HIGHER (PCXI after entry) */
	jla		tc_handler_nmi

	/* restore LOWER+PC and saved FCX+LCX and return (PCXI unaltered) */
	ld.a	%a13, [%a8] ARCH_STATE_NMI_CSA
	ld.a	%a11, [%a13] CTXT_PC
	ldlcx	[%a13] CTXT_CSA
	ld.d	%e12, [%a8] ARCH_STATE_NMI_SAVED_FCX_LCX
	mtcr	CSFR_FCX, %d12
	mtcr	CSFR_LCX, %d13
	isync
	rfe

/**************************************************************************/

	.balign 32
_trap_sys:
	/* SYSCALL is only used by user code! */

	/* switch to kernel stack (no stack frame needed here) */
	ld.a	%a10, [%a8] ARCH_STATE_KERN_STACK

	/* get per-task register-save-area */
	ld.a	%a12, [%a8] SCHED_STATE_REGS
	ld.a	%a13, [%a12] REGS_LOWER

	/* switch to kernel FCX + LCX */
	ld.d	%e12, [%a8] ARCH_STATE_KERN_FCX_LCX
	mtcr	CSFR_FCX, %d12
	mtcr	CSFR_LCX, %d13
	isync

	/* save LOWER (incl PCXI) */
	stlcx	[%a13] CTXT_CSA

	/* call _syscalls[d15], limit syscall number */
	min.u	%d15, %d15, NUM_SYSCALLS
	lea		%a14, (CRAM_VECTOR_BASE + _syscalls - tc_trap_table_start)
	addsc.a	%a15, %a14, %d15, 2
	jli		%a15

	/* check for rescheduling */
	ld.w	%d15, [%a8] SCHED_STATE_RESCHEDULE
	jnz		%d15, _trap_return_user_resched
	/* FALL-THROUGH */

_trap_sys_exit_fast:
	/* restore LOWER+PC and restore PCXI, FCX, and LCX */
	ld.a	%a12, [%a8] SCHED_STATE_REGS
	ld.a	%a13, [%a12] REGS_LOWER
	ld.w	%d14, [%a12] REGS_LCX

	ld.w	%d12, [%a13] CTXT_PCXI
	add		%d13, %d12, 1
	ld.a	%a11, [%a13] CTXT_PC
	ldlcx	[%a13] CTXT_CSA
	/* reload %d0..%d3 -- return code plus additional output registers */
	ld.d	%e0, [%a13] CTXT_D0
	ld.d	%e2, [%a13] CTXT_D2

	mtcr	CSFR_PCXI, %d12
	mtcr	CSFR_FCX, %d13
	mtcr	CSFR_LCX, %d14
	isync
	rfe
	/* NOT REACHED */

_trap_return_user_resched:
	/* invoke scheduler */
	jla		sched_schedule
	/* returns a new register context to return to in a2 */
	mov.aa	%a12, %a2
	/* FALL-THROUGH */

_trap_return_user:
	/* NOTE: a12 must point to struct arch_reg_frame *regs! */
	/* restore LOWER+PC and restore PCXI, FCX, and LCX */
	ld.a	%a13, [%a12] REGS_LOWER
	ld.w	%d14, [%a12] REGS_LCX

	ld.w	%d12, [%a13] CTXT_PCXI
	add		%d13, %d12, 1
	ld.a	%a11, [%a13] CTXT_PC
	ldlcx	[%a13] CTXT_CSA

	mtcr	CSFR_PCXI, %d12
	mtcr	CSFR_FCX, %d13
	mtcr	CSFR_LCX, %d14
	isync
	rfe
	/* NOT REACHED */

/**************************************************************************/

/*
 * Import system call table
 *
 * NOTE: the last entry in the syscall table is sys_ni_syscall
 *
 * NOTE: uses absolute branches (32-bit instructions) instead of pointers!
 */
	.balign 16
_syscalls:
#define __SYSCALL(x) ja x ;
#include "../../../src/syscall_table.S"

/**************************************************************************/

	.balign 32
_trap_common_user:
	/* TRAP in user mode! */

	/* switch to kernel stack (no stack frame needed here) */
	ld.a	%a10, [%a8] ARCH_STATE_KERN_STACK

	/* get per-task register-save-area */
	ld.a	%a12, [%a8] SCHED_STATE_REGS
	ld.a	%a13, [%a12] REGS_LOWER

	/* switch to kernel FCX + LCX */
	ld.d	%e12, [%a8] ARCH_STATE_KERN_FCX_LCX
	mtcr	CSFR_FCX, %d12
	mtcr	CSFR_LCX, %d13
	isync

	/* save LOWER (incl PCXI) */
	stlcx	[%a13] CTXT_CSA

	/* call tc_handler_trap_user(PTR_LOWER, CX_UPPER, class, tin) */
	mov.aa	%a4, %a13
	mfcr	%d4, CSFR_PCXI
	mov		%d5, %d14
	mov		%d6, %d15
	jla		tc_handler_trap_user

	/* check for rescheduling */
	ld.w	%d15, [%a8] SCHED_STATE_RESCHEDULE
	jnz		%d15, _trap_return_user_resched

	/* leave kernel */
	ld.a	%a12, [%a8] SCHED_STATE_REGS
	j		_trap_return_user
	/* NOT REACHED */

/**************************************************************************/

	.balign 32
_trap_irq_disp:
	/* IRQ in user mode! */

	/* switch to kernel stack (no stack frame needed here) */
	ld.a	%a10, [%a8] ARCH_STATE_KERN_STACK

	/* get per-task register-save-area */
	ld.a	%a12, [%a8] SCHED_STATE_REGS
	ld.a	%a13, [%a12] REGS_LOWER

	/* switch to kernel FCX + LCX */
	ld.d	%e12, [%a8] ARCH_STATE_KERN_FCX_LCX
	mtcr	CSFR_FCX, %d12
	mtcr	CSFR_LCX, %d13
	isync

	/* save LOWER (incl PCXI) */
	stlcx	[%a13] CTXT_CSA

	/* get interrupt number from ICR.CCPN */
	mfcr	%d15, CSFR_ICR
	extr.u	%d4, %d15, 0, 8

	/* get pointers in isr_cfg[%d4] and call handler(arg) */
	movh.a	%a15, hi:isr_cfg
	lea		%a15, [%a15], lo:isr_cfg
	addsc.a	%a15, %a15, %d4, 3
	ld.da	%a12, [%a15] 0
	mov.aa	%a4, %a13
	calli	%a12

	/* check for rescheduling */
	ld.w	%d15, [%a8] SCHED_STATE_RESCHEDULE
	jnz		%d15, _trap_return_user_resched

	/* leave kernel */
	ld.a	%a12, [%a8] SCHED_STATE_REGS
	j		_trap_return_user
	/* NOT REACHED */

/**************************************************************************/

	.balign 32
_trap_common_kernel:
	/* call kernel trap handler to raise a panic ... */

	/* switch to NMI stack and NMI FCX + LCX */
	ld.a	%a10, [%a8] ARCH_STATE_NMI_STACK
	ld.d	%e8, [%a8] ARCH_STATE_NMI_FCX_LCX
	mtcr	CSFR_FCX, %d8
	mtcr	CSFR_LCX, %d9
	isync

	/* save LOWER */
	ld.a	%a13, [%a8] ARCH_STATE_NMI_CSA
	stlcx	[%a13] CTXT_CSA

	/* call tc_handler_trap_kern(PTR_LOWER, CX_UPPER, class, tin) */
	mov.aa	%a4, %a13
	mfcr	%d4, CSFR_PCXI
	mov		%d5, %d14
	mov		%d6, %d15
	jla		tc_handler_trap_kern
	/* DOES NOT RETURN */

/**************************************************************************/

	.balign 32
_trap_fcd:
	/* CSA depleted -- quick check if trap occured in entry vectors,
	 * e.g. CRAM < %a11 < CRAM+8000.
	 * a11 points to the interrupted instruction
	 */
	lea		%a14, CRAM_VECTOR_BASE
	lt.a	%d8, %a14, %a11
	jz		%d8, .L_trap_class3_cont

	/* check upper bound */
	lea		%a14, [%a14] CRAM_VECTOR_SIZE
	lt.a	%d8, %a11, %a14
	jz		%d8, .L_trap_class3_cont

	/* continue */
	rfe

/**************************************************************************/

	.balign 32
_trap_fcu:
	/* CSA underflow -- really bad */

	/* switch to NMI stack and panic */
	ld.a	%a10, [%a8] ARCH_STATE_NMI_STACK

	/* switch to NMI FCX + LCX */
	ld.d	%e12, [%a8] ARCH_STATE_NMI_FCX_LCX
	mtcr	CSFR_FCX, %d12
	mtcr	CSFR_LCX, %d13
	isync

	/* call trap_handler_fcu(PCXI, pc) */
	mfcr	%d4, CSFR_PCXI
	mov.d	%d5, %a11
	ja		tc_handler_fcu
	/* NOT REACHED */


/**************************************************************************/

	.org tc_trap_table_start + CRAM_VECTOR_IRQ_OFFSET
_trap_irq:
	/* interrupt vector placed at offset 0x7f8 in CRAM -- 8 byte slot */
	j		_trap_irq_disp

/**************************************************************************/

	/* end of trap table copied to CRAM */
	.org tc_trap_table_start + CRAM_VECTOR_SIZE
tc_trap_table_end:

/**************************************************************************/

/*
 * switch to the kernel stack and kernel CSA
 * void tc_switch_to_kernel_stack(...) __noreturn;
 * NOTE: this function is invoked in the original place in .text,
 *       not in the copy in CRAM!
 */
tc_switch_to_kernel_stack:
	/* set PCXI, FCX and LCX to the same values as on kernel entry */
	mtcr	CSFR_PCXI, %d4
	mtcr	CSFR_FCX, %d5
	mtcr	CSFR_LCX, %d6
	mtcr	CSFR_PSW, %d7
	isync

	/* set new stack and jump */
	mov.aa	%a10, %a4
	jli		%a5

	/* leave kernel with scheduling */
	j		_trap_return_user_resched
	/* NOT REACHED */
