/*
 * arch_v7m.h
 *
 * ARM architecture abstraction layer.
 *
 * azuepke, 2015-06-26: clone from v7
 */

#ifndef __ARCH_V7M_H__
#define __ARCH_V7M_H__

#include <hv_compiler.h>
#include <stddef.h>
#include <assert.h>
#include <arch_regs.h>
#include <arm_private.h>
#include <arm_cr.h>
#include <arm_insn.h>
#include <sched_state.h>
#include <part_state.h>


/** banner for kernel boot message */
#define ARCH_BANNER_NAME "ARMv7-M"

/** turn FPU on */
static inline void arm_fpu_enable(void)
{
	CPACR = CPACR_CP10 | CPACR_CP11;
}

/** turn FPU off */
static inline void arm_fpu_disable(void)
{
	CPACR = 0;
}

/** save FPU state */
static inline void arm_fpu_save(struct arch_reg_frame *regs __unused, struct arch_fpu_frame *fpu)
{
	assert(regs != NULL);
	assert(fpu != NULL);

	fpu->fpscr = arm_get_fpscr();
	arm_vstm_d0_d31(fpu->fpregs);
}

/** restore FPU state */
static inline void arm_fpu_restore(struct arch_reg_frame *regs __unused, struct arch_fpu_frame *fpu)
{
	assert(regs != NULL);
	assert(fpu != NULL);

	arm_set_fpscr(fpu->fpscr);
	arm_vldm_d0_d31(fpu->fpregs);
}

/** internal context switch, part 1: save state of current task */
static inline void arch_task_save(struct arch_reg_frame *regs __unused, struct arch_fpu_frame *fpu __unused)
{
	assert(regs != NULL);

#ifdef ARM_VFP16
	/* switch FPU state */
	if (fpu != NULL) {
		arm_fpu_save(regs, fpu);
		arm_fpu_disable();
	}
#endif
}

/** internal context switch, part 2: restore state of current task */
static inline void arch_task_restore(struct arch_reg_frame *regs __unused, struct arch_fpu_frame *fpu __unused)
{
	assert(regs != NULL);

	/* clear exclusive state */
	arm_clrex();

#ifdef ARM_VFP16
	/* switch FPU state */
	if (fpu != NULL) {
		arm_fpu_enable();
		arm_fpu_restore(regs, fpu);
	}
#endif
}

/** set kernel stack pointer */
static inline void arch_set_kern_stack(
	struct sched_state *sched __unused,
	unsigned char *stack __unused,
	size_t size __unused)
{
#ifndef NDEBUG
	extern unsigned char __kern_stack;
	assert(&__kern_stack == &stack[size]);
#endif
}

/** set kernel's NMI stack pointer */
static inline void arch_set_nmi_stack(
	struct sched_state *sched __unused,
	unsigned char *stack __unused,
	size_t size __unused)
{
#ifndef NDEBUG
	extern unsigned char __nmi_stack;
	assert((addr_t)&__nmi_stack == (addr_t)&stack[size]);
#endif
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
static inline int arch_check_user_stack(const struct part_cfg *part_cfg, addr_t stack)
{
	const struct mem_range *range;
	unsigned int i;

	assert(part_cfg != NULL);

	stack &= -8;
	stack -= 32;
	for (i = 0; i < NUM_MEM_RANGES; i++) {
		range = &part_cfg->mem_ranges[i];
		if ((range->start <= stack) && (stack + 32 < range->end)) {
			/* OK, match */
			return 1;
		}
	}
	return 0;
}

/** assign user functions to a register frame */
static inline void arch_reg_frame_assign(struct arch_reg_frame *regs, unsigned long entry, unsigned long stack_base, size_t stack_size, unsigned long arg0)
{
	unsigned long stack;

	assert(regs != NULL);

	/* the stack is aligned to 8 bytes on ARM */
	stack = stack_base + stack_size;
	stack &= -8;
	/* additionally, the CPU expects 8 saved registers on stack */
	stack -= 32;

	regs->psp = (struct arch_exception_frame *)stack;
	regs->psp->r0 = arg0;
	regs->psp->pc = entry & ~1;
	regs->psp->xpsr = CPSR_USER_BITS;
	regs->xlr = EXC_RETURN_TP;
}

/** set small data area registers in register frame */
static inline void arch_reg_frame_init_sda(struct arch_reg_frame *regs __unused, addr_t sda1_base __unused, addr_t sda2_base __unused)
{
	assert(regs != NULL);

	/* small data sections not used on ARM */
}

/** save argument 0 in a register frame (if not already saved by entry asm) */
static inline void arch_reg_frame_save_arg0(struct arch_reg_frame *regs __unused, unsigned long val __unused)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	/* already saved on PSP stack */
}

/** save argument 1 in a register frame (if not already saved by entry asm) */
static inline void arch_reg_frame_save_arg1(struct arch_reg_frame *regs __unused, unsigned long val __unused)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	/* already saved on PSP stack */
}

/** set argument 0 in a register frame */
static inline void arch_reg_frame_set_arg0(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	regs->psp->r0 = val;
}

/** set argument 1 in a register frame */
static inline void arch_reg_frame_set_arg1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	regs->psp->r1 = val;
}

/** set argument 2 in a register frame */
static inline void arch_reg_frame_set_arg2(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	regs->psp->r2 = val;
}

/** get argument 0 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg0(struct arch_reg_frame *regs)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	return regs->psp->r0;
}

/** get argument 1 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg1(struct arch_reg_frame *regs)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	return regs->psp->r1;
}

/** get argument 2 from register frame (previously saved) */
static inline unsigned long arch_reg_frame_get_arg2(struct arch_reg_frame *regs)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	return regs->psp->r2;
}

/** set return code in a register frame */
static inline void arch_reg_frame_set_return(struct arch_reg_frame *regs, unsigned long ret)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	regs->psp->r0 = ret;
}

/** set 64-bit return code in a register frame */
static inline void arch_reg_frame_set_return64(struct arch_reg_frame *regs, uint64_t ret64)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	/* NOTE: ugly, but the compiler implicitly does the right thing to shuffle
	 * the 64-bit argument into R0 and R1 for both little and endian systems
	 */
	*(uint64_t*)&regs->psp->r0 = ret64;
}

/** set additional return code "out1" in a register frame */
static inline void arch_reg_frame_set_out1(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	regs->psp->r1 = val;
}

/** set additional return code "out2" in a register frame */
static inline void arch_reg_frame_set_out2(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	regs->psp->r2 = val;
}

/** set additional return code "out3" in a register frame */
static inline void arch_reg_frame_set_out3(struct arch_reg_frame *regs, unsigned long val)
{
	assert(regs != NULL);
	assert(regs->psp != NULL);

	regs->psp->r3 = val;
}

/** assign kernel functions to a register frame */
static inline void arch_reg_frame_assign_idle(struct arch_reg_frame *regs, unsigned long entry __unused, unsigned char *stack_base, size_t stack_size, unsigned long sched_state __unused)
{
	unsigned long stack;

	assert(regs != NULL);
	assert(stack_base != NULL);
	assert(stack_size > 16);

	/* the stack is aligned to 8 bytes on ARM */
	stack = (unsigned long)&stack_base[stack_size];
	stack &= -8;

	regs->psp = NULL;
	regs->xlr = EXC_RETURN_TP;

	/* all registers on stack are ignored. We store the idle task's
	 * stack in PSP. arch_switch_to_kernel_stack() will switch to this PSP
	 * after the initialization of the kernel is done.
	 */
	arm_set_psp(stack);
}

/** scheduler switches to idle thread if non-zero */
static inline void arch_set_idle(int idle_state)
{
	arm_set_control(idle_state ? 0 : CONTROL_NPRIV);
}

/** set small data area registers for partition */
static inline void arch_set_partition_sda(addr_t sda1_base __unused, addr_t sda2_base __unused)
{
	/* small data sections not used on ARM */
}

/** set pointer to current per_cpu state (called during init) */
static inline void arch_set_sched_state(struct sched_state *sched __unused)
{
#ifndef NDEBUG
	extern struct sched_state __sched_state;
	assert(sched == &__sched_state);
#endif
}

/** return pointer to current per_cpu state (kept in TLS2 on ARM) */
static inline struct sched_state *arch_get_sched_state(void) __pure;
static inline struct sched_state *arch_get_sched_state(void)
{
	extern struct sched_state __sched_state;
	return &__sched_state;
}

/** return the current CPU ID */
static inline unsigned int arch_cpu_id(void) __pure;
static inline unsigned int arch_cpu_id(void)
{
	return 0;
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
