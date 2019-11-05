/*
 * arch_v7.h
 *
 * ARM architecture abstraction layer.
 *
 * azuepke, 2013-09-11: initial
 * azuepke, 2013-11-24: initial MPU version
 * azuepke, 2014-09-02: more kernel stacks
 * azuepke, 2015-01-06: move state to arch_state.h and inliner from arch_task.h
 */

#ifndef __ARCH_V7_H__
#define __ARCH_V7_H__

#include <hv_compiler.h>
#include <stddef.h>
#include <assert.h>
#include <arch_regs.h>
#include <arm_private.h>
#include <arm_cr.h>
#include <arm_insn.h>
#include <sched_state.h>


/** banner for kernel boot message */
#define ARCH_BANNER_NAME "ARMv7"

/** turn FPU on */
static inline void arm_fpu_enable(void)
{
	arm_set_fpexc(FPEXC_EN);
}

/** turn FPU off */
static inline void arm_fpu_disable(void)
{
	arm_set_fpexc(0);
}

/** save FPU state */
static inline void arm_fpu_save(struct arch_reg_frame *regs, struct arch_fpu_frame *fpu)
{
	assert(regs != NULL);
	assert(fpu != NULL);

	regs->fpscr = arm_get_fpscr();
	arm_vstm_d0_d31(fpu->fpregs);
}

/** restore FPU state */
static inline void arm_fpu_restore(struct arch_reg_frame *regs, struct arch_fpu_frame *fpu)
{
	assert(regs != NULL);
	assert(fpu != NULL);

	arm_set_fpscr(regs->fpscr);
	arm_vldm_d0_d31(fpu->fpregs);
}

/** internal context switch, part 1: save state of current task */
static inline void arch_task_save(struct arch_reg_frame *regs, struct arch_fpu_frame *fpu)
{
	assert(regs != NULL);

	/* save TLS registers (only TLS0 can be modified by the user) */
	regs->tls0 = arm_get_tls0();

	/* switch FPU state */
	if (fpu != NULL) {
		arm_fpu_save(regs, fpu);
		arm_fpu_disable();
	}
}

/** internal context switch, part 2: restore state of current task */
static inline void arch_task_restore(struct arch_reg_frame *regs, struct arch_fpu_frame *fpu)
{
	assert(regs != NULL);

	/* restore TLS registers (only TLS0 can be modified by the user) */
	arm_set_tls0(regs->tls0);
	arm_set_tls1(regs->tls1);

	/* clear exclusive state */
	arm_clrex();

	/* switch FPU state */
	if (fpu != NULL) {
		arm_fpu_enable();
		arm_fpu_restore(regs, fpu);
	}
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
	sched->arch.kern_stack = (unsigned long)&stack[size];
	assert((sched->arch.kern_stack & 7) == 0);
}

/** set kernel's NMI stack pointer */
static inline void arch_set_nmi_stack(
	struct sched_state *sched,
	unsigned char *stack,
	size_t size)
{
	assert(stack != NULL);
	assert(size > 0);

	sched->arch.fiq_stack = (unsigned long)&stack[size];
	assert((sched->arch.fiq_stack & 7) == 0);
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

/** initialize a user register frame (called once at boot time) */
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

	/* the stack is aligned to 8 bytes on ARM */
	stack = stack_base + stack_size;
	stack &= -8;

	regs->regs[15] = entry & ~1;
	regs->regs[13] = stack;
	regs->regs[0] = arg0;
	regs->cpsr = CPSR_USER_BITS;
	if (entry & 1) {
		regs->cpsr |= CPSR_T;
	}
}

/** set small data area registers in register frame */
static inline void arch_reg_frame_init_sda(struct arch_reg_frame *regs __unused, addr_t sda1_base __unused, addr_t sda2_base __unused)
{
	assert(regs != NULL);

	/* small data sections not used on ARM */
}

/** save argument 0 in a register frame (if not already saved by entry asm) */
static inline void arch_reg_frame_save_arg0(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[0] = val;
}

/** save argument 1 in a register frame (if not already saved by entry asm) */
static inline void arch_reg_frame_save_arg1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[1] = val;
}

/** set argument 0 in a register frame */
static inline void arch_reg_frame_set_arg0(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[0] = val;
}

/** set argument 1 in a register frame */
static inline void arch_reg_frame_set_arg1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[1] = val;
}

/** set argument 2 in a register frame */
static inline void arch_reg_frame_set_arg2(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[2] = val;
}

/** get argument 0 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg0(struct arch_reg_frame *regs)
{
	assert(regs != NULL);

	return regs->regs[0];
}

/** get argument 1 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg1(struct arch_reg_frame *regs)
{
	assert(regs != NULL);

	return regs->regs[1];
}

/** get argument 2 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg2(struct arch_reg_frame *regs)
{
	assert(regs != NULL);

	return regs->regs[2];
}

/** set return code in a register frame */
static inline void arch_reg_frame_set_return(struct arch_reg_frame *regs, unsigned long ret)
{
	assert(regs != NULL);

	regs->regs[0] = ret;
}

/** set 64-bit return code in a register frame */
static inline void arch_reg_frame_set_return64(struct arch_reg_frame *regs, uint64_t ret64)
{
	assert(regs != NULL);

	/* NOTE: ugly, but the compiler implicitly does the right thing to shuffle
	 * the 64-bit argument into R0 and R1 for both little and endian systems
	 */
	*(uint64_t*)&regs->regs[0] = ret64;
}

/** set additional return code "out1" in a register frame */
static inline void arch_reg_frame_set_out1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[1] = val;
}

/** set additional return code "out2" in a register frame */
static inline void arch_reg_frame_set_out2(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[2] = val;
}

/** set additional return code "out3" in a register frame */
static inline void arch_reg_frame_set_out3(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);

	regs->regs[3] = val;
}

/** assign kernel functions to a register frame */
static inline void arch_reg_frame_assign_idle(struct arch_reg_frame *regs, unsigned long entry, unsigned char *stack_base, size_t stack_size, unsigned long sched_state __unused)
{
	unsigned long stack;

	assert(regs != NULL);
	assert(stack_base != NULL);
	assert(stack_size > 16);

	/* the stack is aligned to 8 bytes on ARM */
	stack = (unsigned long)&stack_base[stack_size];
	stack &= -8;

	regs->regs[15] = entry & ~1;
	regs->regs[13] = stack;
	regs->cpsr = CPSR_IDLE_BITS;
	if (entry & 1) {
		regs->cpsr |= CPSR_T;
	}
}

/** scheduler switches to idle thread if non-zero */
static inline void arch_set_idle(int idle_state __unused)
{
}

/** set small data area registers for partition */
static inline void arch_set_partition_sda(addr_t sda1_base __unused, addr_t sda2_base __unused)
{
	/* small data sections not used on ARM */
}

/** set pointer to current per_cpu state (called during init) */
static inline void arch_set_sched_state(struct sched_state *sched)
{
	arm_set_tls2((unsigned long)sched);
	arm_set_fiq_stack(sched->arch.fiq_stack);
}

/** return pointer to current per_cpu state (kept in TLS2 on ARM) */
static inline struct sched_state *arch_get_sched_state(void) __pure;
static inline struct sched_state *arch_get_sched_state(void)
{
	return (struct sched_state *)arm_get_tls2();
}

/** return the current CPU ID */
static inline unsigned int arch_cpu_id(void) __pure;
static inline unsigned int arch_cpu_id(void)
{
#ifdef SMP
	return arm_get_mpidr() & 0xf;
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
	unsigned long fsr);
#endif
void arch_init_exceptions(void);
void arch_switch_to_kernel_stack(void (*next)(void *)) __noreturn;

static inline void arch_yield(void)
{
	arm_yield();
}

#endif
