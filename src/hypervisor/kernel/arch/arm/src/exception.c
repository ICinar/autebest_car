/*
 * exception.c
 *
 * Architecture specific exception handling
 *
 * azuepke, 2013-09-15: initial
 */

#include <kernel.h>
#include <assert.h>
#include <arm_private.h>
#include <task.h>
#include <arm_cr.h>
#include <board.h>
#include <arm_perf.h>
#include <sched.h>
#include <hm.h>


#ifndef NDEBUG
void __cold arch_dump_registers(
	struct arch_reg_frame *regs,
	unsigned long vector,
	unsigned long fault,
	unsigned long aux)
{
	unsigned long stack;

	assert(regs != NULL);

	printf(" r0 =%lx  r1 =%lx  r2 =%lx  r3 =%lx\n"
	       " r4 =%lx  r5 =%lx  r6 =%lx  r7 =%lx\n",
	       regs->regs[0], regs->regs[1], regs->regs[2], regs->regs[3],
	       regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);

	/* exceptions in kernel mode do not update stack pointer */
	if ((regs->cpsr & 0xf) == 0) {
		stack = regs->regs[13];
	} else {
		stack = (addr_t)&regs->cpsr + 4;
	}

	printf(" r8 =%lx  r9 =%lx  r10=%lx  r11=%lx\n"
	       " r12=%lx  sp =%lx  lr =%lx  pc =%lx\n",
	       regs->regs[8], regs->regs[9], regs->regs[10], regs->regs[11],
	       regs->regs[12], stack, regs->regs[14], regs->regs[15]);

	printf("cpsr=%lx addr=%lx  fsr=%lx  vec=%s\n",
	       regs->cpsr, fault, aux,
	       vector==1 ? "UNDEF" : vector==3 ? "PABT" : "DABT");
}
#endif

void arm_undef_handler_user(struct arch_reg_frame *regs)
{
	// NOTE: further dispatching possible by analyzing the instruction
	hm_exception(regs, 0, HM_ERROR_ILLEGAL_INSTRUCTION, 1, regs->regs[15], 0);
}

void arm_undef_handler_kern(struct arch_reg_frame *regs)
{
	// NOTE: further dispatching possible by analyzing the instruction
	hm_exception(regs, 1, HM_ERROR_ILLEGAL_INSTRUCTION, 1, regs->regs[15], 0);
}

/*
 * FSR status bits encoding:
 *
 *  Status	DI	Ext	Description
 *  0 0000  DI	-	background fault
 *  0 0001	D-	-	alignment fault
 *  0 0010	DI	-	debug event
 *  0 0100	D-	-	Icache maintenance fault
 *  0 01x1	DI	-	translation fault (no mapping)
 *  0 1000	DI	E	synchronous external abort (error on device access)
 *  0 11x1	DI	-	permission fault (due to APx or nX bits)
 *  1 1001	DI	E	synchronous parity error (error on device access)
 *  1 1000	DI	E	asynchronous parity error (error on device access)
 */
static void arm_dabt_handler(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr, int fatal)
{
	unsigned int status;
	unsigned int err;

	//printf("* arm_dabt_handler: addr=%lx fsr=%lx fatal=%d\n", addr, fsr, fatal);

	/* construct contiguous abort state */
	status = fsr & FSR_FSMASK;
	if (fsr & FSR_FSBIT4)
		status |= 0x10;

	if (status == 0x01) {
		/* alignment fault */
		err = HM_ERROR_UNALIGNED_DATA;
	} else if (status == 0x02) {
		/* debug event */
		err = HM_ERROR_TRAP;
		assert(0);
	} else if (status == 0x00) {
		/* no MPU window matched -> no mapping */
		if (fsr & FSR_WRITE) {
			/* write access */
			err = HM_ERROR_MPU_ERROR_WRITE;
		} else {
			/* read access */
			err = HM_ERROR_MPU_ERROR_READ;
		}
	} else if (status == 0x04) {
		/* icache maintenance fault */
		err = HM_ERROR_ICACHE_ERROR;
	} else if (status == 0x08) {
		/* synchronous external abort */
		err = HM_ERROR_SYNC_BUS_ERROR;
	} else if (status == 0x19) {
		/* synchronous parity/ECC abort */
		err = HM_ERROR_SYNC_BUS_ERROR;
	} else if (status & 0x4) {
		if (fsr & 0x8) {
			/* permission fault */
		} else {
			/* no mapping */
		}

		/* stack fault? */
		if (addr == regs->regs[13]) {
			err = HM_ERROR_STACK_OVERFLOW;
		} else if (fsr & FSR_WRITE) {
			/* write access */
			err = HM_ERROR_MPU_ERROR_WRITE;
		} else {
			/* read access */
			err = HM_ERROR_MPU_ERROR_READ;
		}
	} else {
		/* asynchronous external abort or parity error */
		err = HM_ERROR_ASYNC_BUS_ERROR;
		fatal = 1;
	}

	hm_exception(regs, fatal, err, 4, addr, fsr);
}

void arm_dabt_handler_user(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr)
{
	assert((regs->cpsr & CPSR_MODE_MASK) == CPSR_MODE_USER);

	arm_dabt_handler(regs, addr, fsr, 0);
}

void arm_dabt_handler_kern(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr)
{
	assert((regs->cpsr & CPSR_MODE_MASK) != CPSR_MODE_USER);

	arm_dabt_handler(regs, addr, fsr, 1);
}

static void arm_pabt_handler(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr, int fatal)
{
	unsigned int status;
	unsigned int err;

	//printf("* arm_pabt_handler: addr=%lx fsr=%lx fatal=%d\n", addr, fsr, fatal);

	/* construct contiguous abort state */
	status = fsr & FSR_FSMASK;
	if (fsr & FSR_FSBIT4)
		status |= 0x10;

	if (status == 0x02) {
		/* debug event */
		err = HM_ERROR_TRAP;
		assert(0);
	} else if (status == 0x00) {
		/* no MPU window matched -> no mapping */
		err = HM_ERROR_MPU_ERROR_CODE;
	} else if (status == 0x08) {
		/* synchronous external abort */
		err = HM_ERROR_SYNC_BUS_ERROR;
	} else if (status == 0x19) {
		/* synchronous parity/ECC abort */
		err = HM_ERROR_SYNC_BUS_ERROR;
	} else if (status & 0x4) {
		if (fsr & 0x8) {
			/* permission fault */
		} else {
			/* no mapping */
		}

		err = HM_ERROR_MPU_ERROR_CODE;
	} else {
		/* asynchronous external abort or parity error */
		err = HM_ERROR_ASYNC_BUS_ERROR;
		fatal = 1;
	}

	hm_exception(regs, fatal, err, 3, addr, fsr);
}

void arm_pabt_handler_user(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr)
{
	assert((regs->cpsr & CPSR_MODE_MASK) == CPSR_MODE_USER);

	arm_pabt_handler(regs, addr, fsr, 0);
}

void arm_pabt_handler_kern(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr)
{
	assert((regs->cpsr & CPSR_MODE_MASK) != CPSR_MODE_USER);

	arm_pabt_handler(regs, addr, fsr, 1);
}

void arm_irq_handler(struct arch_reg_frame *regs __unused)
{
	board_irq_dispatch(0);
}

void __cold arm_fiq_handler(struct arch_reg_frame *regs __unused)
{
	board_nmi_dispatch(0);
}

static __init void arm_enable_performance_monitor(void)
{
	unsigned long ctrl;

	/*
	 * ARMv7 defines a cycle counter and up to 32 individual counters
	 * we keep the individual counters disabled
	 * but enable the cycle counter by default
	 */

	/* disable all counters, IRQs, overflows, except CCTR */
	arm_perf_disable_counter(ARM_PERF_MASK_ALL);
	arm_perf_int_mask(ARM_PERF_MASK_ALL);
	arm_perf_int_ack(ARM_PERF_MASK_ALL);

	arm_perf_enable_counter(ARM_PERF_MASK_CCNT);

	/* enable only the cycle counter */
	ctrl = arm_perf_get_ctrl();
	ctrl &= ~ARM_PERF_PMCR_D;
	ctrl |= ARM_PERF_PMCR_C | ARM_PERF_PMCR_E;
	arm_perf_set_ctrl(ctrl);

	/* enable access to performance monitor registers from user space */
	arm_perf_set_useren(ARM_PERF_USEREN);
}

__init void arch_init_exceptions(void)
{
	arm_enable_performance_monitor();
}

/** switch over to the kernel stack and invoke next() */
__init void arch_switch_to_kernel_stack(void (*next)(void *))
{
	struct sched_state *sched = current_sched_state();

	arm_switch_to_kernel_stack(sched->arch.kern_stack, next);
}
