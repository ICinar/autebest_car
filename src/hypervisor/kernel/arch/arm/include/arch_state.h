/*
 * arch_state.h
 *
 * Architecture specific state kept in struct sched_state
 *
 * azuepke, 2013-11-24: initial MPU version
 * azuepke, 2014-09-02: more kernel stacks
 * azuepke, 2015-01-06: added data structures from arch.h
 */

#ifndef __ARCH_STATE_H__
#define __ARCH_STATE_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>

/** architecture specific state that needs to accessed from asm code
 */
struct arch_state {
	/** current partition's kernel stack pointer */
	unsigned long kern_stack;
	/** FIQ stack pointer */
	unsigned long fiq_stack;
};

/** alignment of struct sched_state */
#define SCHED_STATE_ALIGN		8

/** alignment of the kernel stacks */
#define ARCH_KERN_STACK_ALIGN	16

#endif

/** offsets in struct arch_state */
#define ARCH_STATE_KERN_STACK	0
#define ARCH_STATE_FIQ_STACK	4

#define ARCH_STATE_SIZE			8

#endif
