/*
 * arch.h
 *
 * TriCore architecture abstraction layer.
 *
 * azuepke, 2014-10-25: initial
 * azuepke, 2015-01-06: move state to arch_state.h and inliner from arch_task.h
 */

#ifndef __ARCH_H__
#define __ARCH_H__

#include <hv_compiler.h>
#include <stddef.h>
#include <assert.h>
#include <arch_regs.h>
#include <tc_regs.h>
#include <sched_state.h>


/** banner for kernel boot message */
#define ARCH_BANNER_NAME "TriCore"

/** internal context switch, part 1: save state of current task */
static inline void arch_task_save(struct arch_reg_frame *regs, struct arch_fpu_frame *fpu __unused)
{
	assert(regs != NULL);
	assert(regs->csa != NULL);

	(void)regs;
}

/** internal context switch, part 2: restore state of current task */
static inline void arch_task_restore(struct arch_reg_frame *regs, struct arch_fpu_frame *fpu __unused)
{
	assert(regs != NULL);
	assert(regs->csa != NULL);
}

/** set kernel stack pointer */
static inline void arch_set_kern_stack(
	struct sched_state *sched,
	unsigned char *stack,
	size_t size)
{
	assert(stack != NULL);
	assert(size > 0);

	/* partition's kernel stack:
	 * initialized to the top of the stack
	 */
	sched->arch.kern_stack = &stack[size];
	assert(((unsigned long)sched->arch.kern_stack & 7) == 0);
}

/** set kernel's NMI stack pointer */
static inline void arch_set_nmi_stack(
	struct sched_state *sched,
	unsigned char *stack,
	size_t size)
{
	assert(stack != NULL);
	assert(size > 0);

	sched->arch.nmi_stack = &stack[size];
	assert(((unsigned long)sched->arch.nmi_stack & 7) == 0);
}

/** set kernel's register contexts */
static inline void arch_set_kern_ctxts(
	struct sched_state *sched,
	struct arch_ctxt_frame *csa,
	unsigned int num_ctxts)
{
	unsigned int i;

	assert(num_ctxts > 4);

	/* Kern CSA: entry #0 starts the free chain */
	for (i = 0; i < num_ctxts - 1; i++) {
		csa[i].pcxi = PTR_TO_CX(&csa[i+1]);
	}
	assert(i == num_ctxts - 1);
	csa[i].pcxi = 0;

	sched->arch.kern_fcx = PTR_TO_CX(&csa[0]);
	sched->arch.kern_lcx = PTR_TO_CX(&csa[num_ctxts-1]);
}

/** set kernel's NMI register contexts */
static inline void arch_set_nmi_ctxts(
	struct sched_state *sched,
	struct arch_ctxt_frame *csa,
	unsigned int num_ctxts)
{
	unsigned int i;

	assert(num_ctxts > 4);

	/* NMI CSA: entry #1 starts the free chain, entry #0 holds LOWER */
	csa[0].pcxi = 0;
	for (i = 1; i < num_ctxts - 1; i++) {
		csa[i].pcxi = PTR_TO_CX(&csa[i+1]);
	}
	assert(i == num_ctxts - 1);
	csa[i].pcxi = 0;

	sched->arch.nmi_fcx = PTR_TO_CX(&csa[1]);
	sched->arch.nmi_lcx = PTR_TO_CX(&csa[num_ctxts-1]);
	sched->arch.nmi_csa = csa;
}

/** initialize a user register frame */
static inline void arch_reg_frame_init(struct arch_reg_frame *regs __unused)
{
	assert(regs != NULL);
}

/** initialize an FPU frame */
static inline void arch_fpu_frame_init(struct arch_fpu_frame *fpu __unused)
{
	assert(fpu != NULL);
}

/** initialize a register context frame */
static inline void arch_ctxt_frame_init(struct arch_reg_frame *regs, struct arch_ctxt_frame *ctxts, unsigned int num_ctxts)
{
	unsigned int i;

	assert(regs != NULL);
	assert(ctxts != NULL);
	assert(num_ctxts >= 4);

	/* called twice for partition==idle task contexts */
	assert((regs->csa == NULL) ||
	       (regs->csa == ctxts && regs->csa_num == num_ctxts));
	regs->csa = ctxts;
	regs->lcx = PTR_TO_CX(&regs->csa[num_ctxts - 2]);
	regs->csa_num = num_ctxts;

	/* csa[0] is always used as LOWER, with FCX pointing to csa[1] after store */
	regs->csa[0].pcxi = 0;

	/* csa[1] starts the free chain */
	for (i = 1; i < regs->csa_num - 1; i++) {
		regs->csa[i].pcxi = PTR_TO_CX(&regs->csa[i+1]);
	}
	assert(i == regs->csa_num - 1);
	regs->csa[i].pcxi = 0;
}

/** check if user stack is in partition assigned memory */
static inline int arch_check_user_stack(const struct part_cfg *part_cfg __unused, addr_t stack __unused)
{
	return 1;	/* always OK -> an invalid stack faults in user space */
}

/** assign user functions to a register frame */
static inline void arch_reg_frame_assign(struct arch_reg_frame *regs, unsigned long entry, unsigned long stack_base, size_t stack_size, unsigned long arg0)
{
	unsigned long stack;
	unsigned int i;

	assert(regs != NULL);
	assert(regs->csa != NULL);

	/* the stack is aligned to 8 bytes on TriCore */
	stack = stack_base + stack_size;
	stack &= -8;

	/* csa[0] is LOWER */
	regs->csa[0].pcxi = PTR_TO_CX(&regs->csa[1]) | PCXI_UL | PCXI_PIE;
	regs->csa[0].cx.l.pc = entry;
	regs->csa[0].cx.l.a4 = arg0;

	/* csa[1] is UPPER */
	regs->csa[1].pcxi = 0;
	regs->csa[1].cx.u.psw = PSW_USER_BITS;
	regs->csa[1].cx.u.a10 = stack;

	/* reset free chain */
	for (i = 2; i < regs->csa_num - 1; i++) {
		regs->csa[i].pcxi = PTR_TO_CX(&regs->csa[i+1]);
	}
	assert(i == regs->csa_num - 1);
	regs->csa[i].pcxi = 0;
}

/** set small data area registers in register frame */
static inline void arch_reg_frame_init_sda(struct arch_reg_frame *regs __unused, addr_t sda1_base __unused, addr_t sda2_base __unused)
{
	assert(regs != NULL);

	/* a0/a1 must be set globally, see arch_set_partition_sda() */
}

/** save argument 0 in a register frame (if not already saved by entry asm) */
static inline void arch_reg_frame_save_arg0(struct arch_reg_frame *regs __unused, unsigned long val __unused)
{
	assert(regs != NULL);

	/* arguments kept in LOWER, and LOWER is always saved */
	/* NOTE: d4..7 are preserved and not overwritten by the return code */
}

/** save argument 1 in a register frame (if not already saved by entry asm) */
static inline void arch_reg_frame_save_arg1(struct arch_reg_frame *regs __unused, unsigned long val __unused)
{
	assert(regs != NULL);

	/* arguments kept in LOWER, and LOWER is always saved */
}

/** set argument 0 in a register frame */
static inline void arch_reg_frame_set_arg0(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	/* csa[0] is LOWER */
	regs->csa[0].cx.l.d4 = val;
}

/** set argument 1 in a register frame */
static inline void arch_reg_frame_set_arg1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	/* csa[0] is LOWER */
	regs->csa[0].cx.l.d5 = val;
}

/** set argument 2 in a register frame */
static inline void arch_reg_frame_set_arg2(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	/* csa[0] is LOWER */
	regs->csa[0].cx.l.d6 = val;
}

/** get argument 0 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg0(struct arch_reg_frame *regs)
{
	assert(regs != NULL);
	assert(regs->csa != NULL);

	/* csa[0] is LOWER */
	return regs->csa[0].cx.l.d4;
}

/** get argument 1 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg1(struct arch_reg_frame *regs)
{
	assert(regs != NULL);
	assert(regs->csa != NULL);

	/* csa[0] is LOWER */
	return regs->csa[0].cx.l.d5;
}

/** get argument 2 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg2(struct arch_reg_frame *regs)
{
	assert(regs != NULL);
	assert(regs->csa != NULL);

	/* csa[0] is LOWER */
	return regs->csa[0].cx.l.d6;
}

/** set return code in a register frame */
static inline void arch_reg_frame_set_return(struct arch_reg_frame *regs, unsigned long ret)
{
	assert(regs != NULL);
	assert(regs->csa != NULL);

	/* csa[0] is LOWER */
	regs->csa[0].cx.l.d2 = ret;
}

/** set 64-bit return code in a register frame */
static inline void arch_reg_frame_set_return64(struct arch_reg_frame *regs, uint64_t ret64)
{
	assert(regs != NULL);

	/* NOTE: TriCore uses always little endian */
	regs->csa[0].cx.l.d2 = ret64 & 0xffffffffUL;
	regs->csa[0].cx.l.d3 = ret64 >> 32;
}

/** set additional return code "out1" in a register frame */
static inline void arch_reg_frame_set_out1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->csa[0].cx.l.d3 = val;
}

/** set additional return code "out2" in a register frame */
static inline void arch_reg_frame_set_out2(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->csa[0].cx.l.d0 = val;
}

/** set additional return code "out3" in a register frame */
static inline void arch_reg_frame_set_out3(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->csa[0].cx.l.d1 = val;
}

/** assign kernel functions to a register frame */
static inline void arch_reg_frame_assign_idle(struct arch_reg_frame *regs, unsigned long entry, unsigned char *stack_base, size_t stack_size, unsigned long sched_state)
{
	unsigned long stack;

	assert(regs != NULL);
	assert(regs->csa != NULL);
	assert(stack_base != NULL);
	assert(stack_size > 16);

	/* the stack is aligned to 8 bytes on TriCore */
	stack = (unsigned long)&stack_base[stack_size];
	stack &= -8;

	/* csa[0] is LOWER */
	regs->csa[0].pcxi = PTR_TO_CX(&regs->csa[1]) | PCXI_UL | PCXI_PIE;
	regs->csa[0].cx.l.pc = entry;
	regs->csa[0].cx.l.a4 = sched_state;

	/* csa[1] is UPPER */
	regs->csa[1].pcxi = 0;
	regs->csa[1].cx.u.psw = PSW_IDLE_BITS;
	regs->csa[1].cx.u.a10 = stack;
	assert((regs->csa[1].cx.u.a10 & 7) == 0);
}

/** scheduler switches to idle thread if non-zero */
static inline void arch_set_idle(int idle_state __unused)
{
}

/** set small data area registers for partition */
static inline void arch_set_partition_sda(addr_t sda1_base, addr_t sda2_base)
{
	tc_set_a0_a1((void*)sda1_base, (void*)sda2_base);
}

/** set pointer to current per_cpu state (called during init) */
static inline void arch_set_sched_state(struct sched_state *sched)
{
	tc_set_a8(sched);
}

/** return pointer to current per_cpu state (kept in a8 on Tricore) */
static inline struct sched_state *arch_get_sched_state(void) __pure;
static inline struct sched_state *arch_get_sched_state(void)
{
	return (struct sched_state *) tc_get_a8();
}

/** return the current CPU ID */
static inline unsigned int arch_cpu_id(void) __pure;
static inline unsigned int arch_cpu_id(void)
{
#ifdef SMP
	return MFCR(CSFR_CORE_ID) & 0x7;
#else
	return 0;
#endif
}

/* exception.c */
#ifndef NDEBUG
void arch_dump_registers(
	struct arch_reg_frame *regs,
	unsigned long vector,
	unsigned long fault,
	unsigned long higher_cx);
#endif
void arch_init_exceptions(void);
void arch_switch_to_kernel_stack(void (*next)(void *)) __noreturn;

static inline void arch_yield(void)
{
	barrier();
}

#endif
