/*
 * exception.c
 *
 * Architecture specific exception handling
 *
 * azuepke, 2015-06-26: initial
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
	unsigned long xpsr = 0;
	assert(regs != NULL);

	if (regs->psp != NULL) {
		printf(" r0 =%lx  r1 =%lx  r2 =%lx  r3 =%lx\n",
			   regs->psp->r0, regs->psp->r1, regs->psp->r2, regs->psp->r3);
	} else {
		printf(" r0 =????????  r1 =????????  r2 =????????  r3 =????????\n");
	}
	printf(" r4 =%lx  r5 =%lx  r6 =%lx  r7 =%lx\n",
	       regs->r4[0], regs->r4[1], regs->r4[2], regs->r4[3]);
	printf(" r8 =%lx  r9 =%lx  r10=%lx  r11=%lx\n",
	       regs->r4[4], regs->r4[5], regs->r4[6], regs->r4[7]);

	if (regs->psp != NULL) {
		stack = (unsigned long)regs->psp;
		stack += 32;
		xpsr = regs->psp->xpsr;
		if (xpsr & CPSR_STK) {
			stack += 4;
		}
		printf(" r12=%lx  sp =%lx  lr =%lx  pc =%lx\n",
		       regs->psp->r12, stack, regs->psp->lr, regs->psp->pc);
	} else {
		printf(" r12=????????  sp =????????  lr =????????  pc =????????\n");
	}

	printf("xpsr=%lx addr=%lx  fsr=%lx  psp=%lx  xlr=%lx  vec=%s\n",
	       xpsr, fault, aux, (unsigned long)regs->psp, regs->xlr,
	       vector==2 ? "NMI" : vector==3 ? "HARD" : vector==4 ? "MEM" :
	       vector==5 ? "BUS" : vector==6 ? "USAGE" : "?");
}
#endif

void arm_memory_handler_user(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long cfsr;
	unsigned long mmfar;

	mmfar = MMFAR;
	cfsr = CFSR;
	barrier();
	/* clear state */
	CFSR = 0x000000ff;

	//printf("* arm_memory_handler_user() 0x%lx 0x%lx\n", cfsr, mmfar);

	if (cfsr & CFSR_MUNSTKERR) {
		/* treat UNSTK errors as read faults */
		error_code = HM_ERROR_MPU_ERROR_READ;
		mmfar = (unsigned long)regs->psp;
		regs->psp = NULL;	/* psp is no longer valid */
	} else if (cfsr & CFSR_MSTKERR) {
		/* while STK errors are probably stack overflows */
		error_code = HM_ERROR_STACK_OVERFLOW;
		mmfar = (unsigned long)regs->psp;
		regs->psp = NULL;	/* psp is no longer valid */
	} else if (cfsr & CFSR_IACCVIOL) {
		error_code = HM_ERROR_MPU_ERROR_CODE;
		mmfar = regs->psp->pc;
	} else if (cfsr & CFSR_DACCVIOL) {
		/* read or write, can't decide */
		error_code = HM_ERROR_MPU_ERROR_WRITE;
		/* valid mmfar */
	} else /* if (cfsr & CFSR_MLSPERR) */ {
		error_code = HM_ERROR_CONTEXT_ERROR;
	}
	hm_exception(regs, 0, error_code, 4, mmfar, cfsr);
}

void arm_memory_handler_kern(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long cfsr;
	unsigned long mmfar;

	mmfar = MMFAR;
	cfsr = CFSR;
	barrier();
	/* clear state */
	CFSR = 0x000000ff;

	//printf("* arm_memory_handler_kern() 0x%lx 0x%lx\n", cfsr, mmfar);

	if (cfsr & CFSR_MUNSTKERR) {
		/* treat UNSTK errors as read faults */
		error_code = HM_ERROR_MPU_ERROR_READ;
		mmfar = (unsigned long)regs->psp;
		regs->psp = NULL;	/* psp is no longer valid */
	} else if (cfsr & CFSR_MSTKERR) {
		/* while STK errors are probably stack overflows */
		error_code = HM_ERROR_STACK_OVERFLOW;
		mmfar = (unsigned long)regs->psp;
		regs->psp = NULL;	/* psp is no longer valid */
	} else if (cfsr & CFSR_IACCVIOL) {
		error_code = HM_ERROR_MPU_ERROR_CODE;
		mmfar = regs->psp->pc;
	} else if (cfsr & CFSR_DACCVIOL) {
		/* read or write, can't decide */
		error_code = HM_ERROR_MPU_ERROR_WRITE;
		/* valid mmfar */
	} else /* if (cfsr & CFSR_MLSPERR) */ {
		error_code = HM_ERROR_CONTEXT_ERROR;
	}
	hm_exception(regs, 1, error_code, 4, mmfar, cfsr);
}

void arm_usage_handler_user(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long cfsr;

	cfsr = CFSR;
	barrier();
	/* clear state */
	CFSR = 0xffff0000;

	//printf("* arm_usage_handler_user() 0x%lx\n", cfsr);

	if (cfsr & CFSR_UNDEFINSTR) {
		error_code = HM_ERROR_ILLEGAL_INSTRUCTION;
	} else if (cfsr & (CFSR_INVSTATE|CFSR_INVPC)) {
		/* invalid context on exception return */
		error_code = HM_ERROR_CONTEXT_ERROR;
	} else if (cfsr & CFSR_NOCP) {
		error_code = HM_ERROR_FPU_ACCESS;
	} else if (cfsr & CFSR_UNALIGNED) {
		error_code = HM_ERROR_UNALIGNED_DATA;
	} else /* if (cfsr & CFSR_DIVBYZERO) */ {
		error_code = HM_ERROR_ARITHMETIC_OVERFLOW;
	}
	hm_exception(regs, 0, error_code, 6, 0, cfsr);
}

void arm_usage_handler_kern(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long cfsr;

	cfsr = CFSR;
	barrier();
	/* clear state */
	CFSR = 0xffff0000;

	//printf("* arm_usage_handler_kern() 0x%lx\n", cfsr);

	if (cfsr & CFSR_UNDEFINSTR) {
		error_code = HM_ERROR_ILLEGAL_INSTRUCTION;
	} else if (cfsr & (CFSR_INVSTATE|CFSR_INVPC)) {
		/* invalid context on exception return */
		error_code = HM_ERROR_CONTEXT_ERROR;
	} else if (cfsr & CFSR_NOCP) {
		error_code = HM_ERROR_FPU_ACCESS;
	} else if (cfsr & CFSR_UNALIGNED) {
		error_code = HM_ERROR_UNALIGNED_DATA;
	} else /* if (cfsr & CFSR_DIVBYZERO) */ {
		error_code = HM_ERROR_ARITHMETIC_OVERFLOW;
	}
	hm_exception(regs, 1, error_code, 6, 0, cfsr);
}

void arm_bus_handler_user(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long cfsr;
	unsigned long bfar;
	int fatal;

	bfar = BFAR;
	cfsr = CFSR;
	barrier();
	/* clear state */
	CFSR = 0x0000ff00;

	//printf("* arm_bus_handler_user() 0x%lx 0x%lx\n", cfsr, bfar);

	fatal = 0;
	if (cfsr & (CFSR_STKERR | CFSR_UNSTKERR)) {
		error_code = HM_ERROR_SYNC_BUS_ERROR;
		bfar = (unsigned long)regs->psp;
		regs->psp = NULL;	/* psp is no longer valid */
	} else if (cfsr & CFSR_IBUSERR) {
		error_code = HM_ERROR_CODE_MEMORY_ERROR;
		bfar = regs->psp->pc;
	} else if (cfsr & CFSR_PRECISERR) {
		error_code = HM_ERROR_SYNC_BUS_ERROR;
	} else if (cfsr & CFSR_IMPRECISERR) {
		error_code = HM_ERROR_ASYNC_BUS_ERROR;
		fatal = 1;
	} else /* if (cfsr & CFSR_LSPERR) */ {
		error_code = HM_ERROR_CONTEXT_ERROR;
	}
	hm_exception(regs, fatal, error_code, 4, bfar, cfsr);
}

void arm_bus_handler_kern(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long cfsr;
	unsigned long bfar;

	bfar = BFAR;
	cfsr = CFSR;
	barrier();
	/* clear state */
	CFSR = 0x0000ff00;

	//printf("* arm_bus_handler_kern() 0x%lx 0x%lx\n", cfsr, bfar);

	if (cfsr & (CFSR_STKERR | CFSR_UNSTKERR)) {
		error_code = HM_ERROR_SYNC_BUS_ERROR;
		bfar = (unsigned long)regs->psp;
		regs->psp = NULL;	/* psp is no longer valid */
	} else if (cfsr & CFSR_IBUSERR) {
		error_code = HM_ERROR_CODE_MEMORY_ERROR;
		bfar = regs->psp->pc;
	} else if (cfsr & CFSR_PRECISERR) {
		error_code = HM_ERROR_SYNC_BUS_ERROR;
	} else if (cfsr & CFSR_IMPRECISERR) {
		error_code = HM_ERROR_ASYNC_BUS_ERROR;
	} else /* if (cfsr & CFSR_LSPERR) */ {
		error_code = HM_ERROR_CONTEXT_ERROR;
	}
	hm_exception(regs, 1, error_code, 4, bfar, cfsr);
}

void arm_debug_handler_user(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long dfsr;
	int fatal;

	dfsr = DFSR;
	barrier();
	/* clear state */
	DFSR = 0x0000001f;

	//printf("* arm_debug_handler_user() 0x%lx\n", dfsr);

	error_code = HM_ERROR_TRAP;
	if (dfsr & DFSR_BKPT) {
		fatal = 0;
	} else {
		fatal = 1;
	}
	hm_exception(regs, fatal, error_code, 4, 0, dfsr);
}

void arm_debug_handler_kern(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long dfsr;

	dfsr = DFSR;
	barrier();
	/* clear state */
	DFSR = 0x0000001f;

	//printf("* arm_debug_handler_kern() 0x%lx\n", dfsr);

	error_code = HM_ERROR_TRAP;
	hm_exception(regs, 1, error_code, 4, 0, dfsr);
}

void arm_hard_handler_user(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long hfsr;
	unsigned long cfsr;
	unsigned long far;

	hfsr = HFSR;
	cfsr = CFSR;
	if (cfsr & CFSR_BFARVALID) {
		far = BFAR;
	} else if (cfsr & CFSR_MMARVALID) {
		far = MMFAR;
	} else {
		far = 0;
	}

	//printf("* arm_hard_handler_user() 0x%lx 0x%lx 0x%lx\n", hfsr, cfsr, far);

	if (hfsr & HFSR_VECTTBL) {
		error_code = HM_ERROR_SYNC_BUS_ERROR;
	} else {
		error_code = HM_ERROR_HARDWARE_ERROR;
	}
	hm_exception(regs, 1, error_code, 3, far, hfsr);
}

void arm_hard_handler_kern(struct arch_reg_frame *regs)
{
	unsigned int error_code;
	unsigned long hfsr;
	unsigned long cfsr;
	unsigned long far;

	hfsr = HFSR;
	cfsr = CFSR;
	if (cfsr & CFSR_BFARVALID) {
		far = BFAR;
	} else if (cfsr & CFSR_MMARVALID) {
		far = MMFAR;
	} else {
		far = 0;
	}

	//printf("* arm_hard_handler_kern() 0x%lx 0x%lx 0x%lx\n", hfsr, cfsr, far);

	if (hfsr & HFSR_VECTTBL) {
		error_code = HM_ERROR_SYNC_BUS_ERROR;
	} else {
		error_code = HM_ERROR_HARDWARE_ERROR;
	}
	hm_exception(regs, 1, error_code, 3, far, hfsr);
}

void arm_irq_handler(unsigned int vector)
{
	board_irq_dispatch(vector);
}

void arm_nmi_handler(struct arch_reg_frame *regs __unused)
{
	//printf("* arm_nmi_handler()\n");

	board_nmi_dispatch((unsigned long)regs);
}

__init void arch_init_exceptions(void)
{
	/* enforce save default exception behaviour */
	CCR = CCR_STKALIGN | CCR_DIV_0_TRP | CCR_UNALIGN_TRP;

	/* enable all exceptions */
	SHCSR |= SHCSR_USGFAULTENA | SHCSR_BUSFAULTENA | SHCSR_MEMFAULTENA;
}
