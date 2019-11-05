/*
 * arch_state.h
 *
 * Architecture specific state kept in struct sched_state
 *
 * azuepke, 2014-10-25: initial
 * azuepke, 2015-01-06: added data structures from arch.h
 */

#ifndef __ARCH_STATE_H__
#define __ARCH_STATE_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>

/** architecture specific state that needs to accessed from asm code
 */
struct arch_state {
	/** current partition's kernel stack pointer, FCX and LCX */
	unsigned long kern_fcx, kern_lcx;
	void *kern_stack;
	unsigned long padding;

	/** kernel ISR stack pointer, FCX and LCX */
	unsigned long nmi_fcx, nmi_lcx;
	unsigned long nmi_saved_fcx, nmi_saved_lcx;
	void *nmi_stack;
	void *nmi_csa;	/* for LOWER */
};

/** alignment of struct sched_state */
#define SCHED_STATE_ALIGN		32

/** alignment of the kernel stacks */
#define ARCH_KERN_STACK_ALIGN	8

#endif

/** offsets in struct arch_state (accessible through a8) */
#define ARCH_STATE_KERN_FCX_LCX			0
#define ARCH_STATE_KERN_STACK			8
#define ARCH_STATE_NMI_FCX_LCX			16
#define ARCH_STATE_NMI_SAVED_FCX_LCX	24
#define ARCH_STATE_NMI_STACK			32
#define ARCH_STATE_NMI_CSA				36

#define ARCH_STATE_SIZE					40

#endif
