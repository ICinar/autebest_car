/*
 * arch.h
 *
 * PowerPC architecture abstraction layer.
 *
 * azuepke, 2014-06-03: initial PPC version
 * azuepke, 2014-09-02: more kernel stacks
 * azuepke, 2015-01-06: move state to arch_state.h and inliner from arch_task.h
 */

#ifndef __ARCH_H__
#define __ARCH_H__

#include <hv_compiler.h>
#include <stddef.h>
#include <assert.h>
#include <arch_regs.h>
#include <ppc_private.h>
#include <ppc_msr.h>
#include <ppc_insn.h>
#include <sched_state.h>


/** banner for kernel boot message */
#define ARCH_BANNER_NAME "PowerPC"

/** turn FPU on */
static inline void ppc_fpu_enable(void)
{
	unsigned long msr;
	msr = ppc_get_msr();
	msr |= MSR_SPE;
	ppc_set_msr(msr);
}

/** turn FPU off */
static inline void ppc_fpu_disable(void)
{
	unsigned long msr;
	msr = ppc_get_msr();
	msr &= ~MSR_SPE;
	ppc_set_msr(msr);
}

/** save FPU state */
static inline void ppc_fpu_save(struct arch_fpu_frame *fpu)
{
	assert(fpu != NULL);

	ppc_spe_save(fpu);
}

/** restore FPU state */
static inline void ppc_fpu_restore(struct arch_fpu_frame *fpu)
{
	assert(fpu != NULL);

	ppc_spe_restore(fpu);
}

/** internal context switch, part 1: save state of current task */
static inline void arch_task_save(struct arch_reg_frame *regs, struct arch_fpu_frame *fpu)
{
	assert(regs != NULL);

	/* save extra registers */
	regs->usprg0 = ppc_get_usprg0();
	regs->spefscr = ppc_get_spefscr();

	/* switch FPU state */
	if (fpu != NULL) {
		ppc_fpu_save(fpu);
		ppc_fpu_disable();
	}
}

/** internal context switch, part 2: restore state of current task */
static inline void arch_task_restore(struct arch_reg_frame *regs, struct arch_fpu_frame *fpu)
{
	assert(regs != NULL);

	/* restore extra registers */
	ppc_set_usprg0(regs->usprg0);
	ppc_set_spefscr(regs->spefscr);

	/* clear exclusive state */
	ppc_clear_reservation();

	/* switch FPU state */
	if (fpu != NULL) {
		ppc_fpu_enable();
		ppc_fpu_restore(fpu);
	}
}

/** set kernel stack pointer */
static inline void arch_set_kern_stack(
	struct sched_state *sched,
	unsigned char *stack,
	size_t size)
{
	assert(stack != NULL);
	assert(size > 16);

	/* partition's kernel stack:
	 * initialized to the top of the stack,
	 * leave space for a stack frame (16 bytes)
	 */
	sched->arch.kern_stack = (unsigned long)&stack[size];
	sched->arch.kern_stack -= 16;
	assert((sched->arch.kern_stack & 15) == 0);
}

/** set kernel's NMI stack pointer */
static inline void arch_set_nmi_stack(
	struct sched_state *sched,
	unsigned char *stack,
	size_t size)
{
	assert(stack != NULL);
	assert(size > 16);

	/* critical IRQ stack:
	 * initialized to the register save area, space for 40 registers
	 */
	sched->arch.crit_stack = (unsigned long)&stack[size];
	sched->arch.crit_stack -= 40*4;
	assert((sched->arch.crit_stack & 15) == 0);
}

/** set kernel's register contexts */
static inline void arch_set_kern_ctxts(
	struct sched_state *sched __unused,
	struct arch_ctxt_frame *ctxts __unused,
	unsigned int num_ctxts __unused)
{
}

/** set kernel's NMI register contexts */
static inline void arch_set_nmi_ctxts(
	struct sched_state *sched __unused,
	struct arch_ctxt_frame *ctxts __unused,
	unsigned int num_ctxts __unused)
{
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
static inline void arch_ctxt_frame_init(struct arch_reg_frame *regs __unused, struct arch_ctxt_frame *ctxts __unused, unsigned int num_ctxts __unused)
{
	assert(regs != NULL);
	assert(ctxts != NULL);
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

	assert(regs != NULL);

	/* On PowerPC, the stack is aligned to 8 bytes, with 8 bytes overhead */
	stack = stack_base + stack_size;
	stack &= -8;
	stack -= 8;

	regs->srr0 = entry;
	regs->regs[1] = stack;
	regs->regs[3] = arg0;
	regs->srr1 = MSR_USER_BITS;
}

/** set small data area registers in register frame */
static inline void arch_reg_frame_init_sda(struct arch_reg_frame *regs, addr_t sda1_base, addr_t sda2_base)
{
	assert(regs != NULL);

	regs->regs[13] = sda1_base;	/* initialized data, symbol _SDA_BASE_ */
	regs->regs[2] = sda2_base;	/* initialized data, symbol _SDA2_BASE_ */
}

/** save argument 0 in a register frame (if not already saved by entry asm) */
static inline void arch_reg_frame_save_arg0(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[3] = val;
}

/** save argument 1 in a register frame (if not already saved by entry asm) */
static inline void arch_reg_frame_save_arg1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[4] = val;
}

/** set argument 0 in a register frame */
static inline void arch_reg_frame_set_arg0(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[3] = val;
}

/** set argument 1 in a register frame */
static inline void arch_reg_frame_set_arg1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[4] = val;
}

/** set argument 2 in a register frame */
static inline void arch_reg_frame_set_arg2(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[5] = val;
}

/** get argument 0 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg0(struct arch_reg_frame *regs)
{
	assert(regs != NULL);

	return regs->regs[3];
}

/** get argument 1 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg1(struct arch_reg_frame *regs)
{
	assert(regs != NULL);

	return regs->regs[4];
}

/** get argument 2 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg2(struct arch_reg_frame *regs)
{
	assert(regs != NULL);

	return regs->regs[5];
}

/** set return code in a register frame */
static inline void arch_reg_frame_set_return(struct arch_reg_frame *regs, unsigned long ret)
{
	assert(regs != NULL);

	regs->regs[3] = ret;
}

/** set 64-bit return code in a register frame */
static inline void arch_reg_frame_set_return64(struct arch_reg_frame *regs, uint64_t ret64)
{
	assert(regs != NULL);

	/* NOTE: assumes PowerPC big endian order */
	regs->regs[3] = ret64 >> 32;
	regs->regs[4] = ret64 & 0xffffffffUL;
}

/** set additional return code "out1" in a register frame */
static inline void arch_reg_frame_set_out1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[4] = val;
}

/** set additional return code "out2" in a register frame */
static inline void arch_reg_frame_set_out2(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[5] = val;
}

/** set additional return code "out3" in a register frame */
static inline void arch_reg_frame_set_out3(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[6] = val;
}

/** assign kernel functions to a register frame */
static inline void arch_reg_frame_assign_idle(struct arch_reg_frame *regs, unsigned long entry, unsigned char *stack_base, size_t stack_size, unsigned long sched_state)
{
	unsigned long stack;

	assert(regs != NULL);
	assert(stack_base != NULL);
	assert(stack_size > 16);

	/* On PowerPC, the stack is aligned to 8 bytes, with 8 bytes overhead */
	stack = (unsigned long)&stack_base[stack_size];
	stack &= -8;
	stack -= 8;

	regs->srr0 = entry;
	regs->regs[1] = stack;
	regs->srr1 = MSR_IDLE_BITS;
	regs->regs[2] = sched_state;
}

/** scheduler switches to idle thread if non-zero */
static inline void arch_set_idle(int idle_state __unused)
{
}

/** set small data area registers for partition */
static inline void arch_set_partition_sda(addr_t sda1_base __unused, addr_t sda2_base __unused)
{
	/* small data sections initialized by arch_reg_frame_init_sda() */
}

/** set pointer to current per_cpu state (called during init) */
static inline void arch_set_sched_state(struct sched_state *sched)
{
	/* keep pointer to scheduling state in SPRG3 */
	ppc_set_spr(SPR_SPRG3, (unsigned long)sched);
	ppc_set_r2((unsigned long)sched);
}

/** return pointer to current per_cpu state (kept in r2 on PowerPC) */
static inline struct sched_state *arch_get_sched_state(void) __pure;
static inline struct sched_state *arch_get_sched_state(void)
{
	return (struct sched_state *) ppc_get_r2();
}

/** return the current CPU ID */
static inline unsigned int arch_cpu_id(void) __pure;
static inline unsigned int arch_cpu_id(void)
{
#ifdef SMP
	return ppc_get_smp_id() & 0xff;
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
	unsigned long esr);
#endif
void arch_init_exceptions(void);
void arch_switch_to_kernel_stack(void (*next)(void *)) __noreturn;

static inline void arch_yield(void)
{
	barrier();
}

#endif
