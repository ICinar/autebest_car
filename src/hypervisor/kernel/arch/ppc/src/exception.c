/**
 * \file     exception.c
 * \brief    Architecture specific exception handling.
 *
 * \details
 *
 * \date     2013-11-22
 * \author   azuepke
 * \author   easycore GmbH, 91058 Erlangen, Germany
 * \version
 *
 * \par          License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * \par      File history
 * - azuepke, 2013-11-22: initial PPC port
 * - azuepke, 2014-06-03: ported to MPU version
 * - azuepke, 2015-05-27: simplified to single kernel stack
 *
 * 
 * \copyright Copyright 2013 easycore GmbH.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include <kernel.h>
#include <assert.h>
#include <ppc_private.h>
#include <task.h>
#include <ppc_msr.h>
#include <board.h>
#include <ppc_spr.h>
#include <ppc_tlb.h>
#include <sched.h>
#include <hm.h>

/*==================[macros]==================================================*/

/**
 * \brief Bits of the MCSR register indicating an exception caused by the MPU.
 */
#define MCSR_MPU_BITS (MCSR_BUS_WRERR|MCSR_BUS_DRERR|MCSR_BUS_IRERR|MCSR_IF|MCSR_LD|MCSR_G|MCSR_ST)

/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/
/*==================[external function definitions]===========================*/

#ifndef NDEBUG
void __cold arch_dump_registers(
	struct arch_reg_frame *regs,
	unsigned long vector,
	unsigned long fault,
	unsigned long esr)
{
	const char *s;

	assert(regs != NULL);

	printf(" r0 =%x  r1 =%x  r2 =%x  r3 =%x | pc  =%x %s\n",
	       regs->regs[0], regs->regs[1], regs->regs[2], regs->regs[3],
	       regs->srr0, regs->srr1 & MSR_PR ? "user" : "kernel");
	printf(" r4 =%x  r5 =%x  r6 =%x  r7 =%x | msr =%x\n",
	       regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7],
	       regs->srr1);
	printf(" r8 =%x  r9 =%x  r10=%x  r11=%x | lr  =%x\n",
	       regs->regs[8], regs->regs[9], regs->regs[10], regs->regs[11],
	       regs->lr);
	printf(" r12=%x  r13=%x  r14=%x  r15=%x | ctr =%x\n",
	       regs->regs[12], regs->regs[13], regs->regs[14], regs->regs[15],
	       regs->ctr);
	printf(" r16=%x  r17=%x  r18=%x  r19=%x | cr  =%x\n",
	       regs->regs[16], regs->regs[17], regs->regs[18], regs->regs[19],
	       regs->cr);
	printf(" r20=%x  r21=%x  r22=%x  r23=%x | xer =%x\n",
	       regs->regs[20], regs->regs[21], regs->regs[22], regs->regs[23],
	       regs->xer);
	printf(" r24=%x  r25=%x  r26=%x  r27=%x | addr=%lx\n",
	       regs->regs[24], regs->regs[25], regs->regs[26], regs->regs[27],
	       fault);
	printf(" r28=%x  r29=%x  r30=%x  r31=%x | %s=%lx ",
	       regs->regs[28], regs->regs[29], regs->regs[30], regs->regs[31],
	       vector == 1 ? "mcsr" : "esr ", esr);

	switch (vector) {
	default:
		printf("IVOR%ld\n", vector);
		return;
	case 1:
		s = "MCE";
		break;
	case 2:
		s = "DSI";
		break;
	case 3:
		s = "ISI";
		break;
	case 5:
		s = "ALIGN";
		break;
	case 6:
		s = "PROG";
		break;
	case 32:
	case 33:
	case 34:
		s = "SPE";
		break;
	}
	printf("%s\n", s);
}
#endif

/* fatal exception */
void ppc_handler_panic(struct arch_reg_frame *regs, unsigned int vector)
{
	unsigned long fault;
	unsigned long esr;

	fault = ppc_get_spr(SPR_DEAR);
	esr = ppc_get_spr(SPR_ESR);

	/* FIXME: only IVOR 15 points to here */
	assert(vector == 15);

	hm_exception(regs, 1, HM_ERROR_TRAP, vector, fault, esr);
}

/* IRQ */
void ppc_handler_irq(struct arch_reg_frame *regs __unused, unsigned int vector)
{
	board_irq_dispatch(vector);
}

/* critical IRQ */
void ppc_handler_irq_crit(struct arch_reg_frame *regs __unused, unsigned int vector)
{
	board_nmi_dispatch(vector);
}

/*------------------[Machine Check Interrupt]---------------------------------*/
/**
 * \brief   Machine Check Exception handler (MCE).
 *
 * \details This function handles the "Machine Check interrupt".
 * (Actually an exception.)
 * When an Machine Check Interrupt happens, the hardware sets the register MCSR
 * (Machine Check Syndrome Register) to reflect the source of the exception and
 * then jumps to the associated interrupt handler (on Calypso offset 0x10 from
 * IVPR). The interrupt handler points to an assembler routine which saves the
 * special registers, loads the vector number (exception number) as arguments
 * and then calls this function.
 *
 * This handler is called repeatedly while not all syndrome bits have been
 * cleared.
 */
void ppc_handler_mce(struct arch_reg_frame *regs, unsigned int vector)
{
	unsigned long fault;
	unsigned long mcsr;
	unsigned int err;

	//assert((regs->srr1 & MSR_PR) != 0);
	assert(vector == 1);

	fault = ppc_get_spr(SPR_MCAR);
	mcsr = ppc_get_spr(SPR_MCSR);

	/* Handle NMI */
	if (mcsr & MCSR_NMI) {
		/* NMI */
		board_nmi_dispatch(vector);
		/* w1c */
		ppc_set_spr(SPR_MCSR, MCSR_NMI);
		return;
	}

	/* Handle MPU exceptions */
	if ((mcsr & MCSR_MPU_BITS) != 0) {
		if (mcsr & MCSR_BUS_WRERR) {
			err = HM_ERROR_MPU_ERROR_WRITE;
		} else if (mcsr & MCSR_BUS_DRERR) {
			err = HM_ERROR_MPU_ERROR_READ;
		} else {
			err = HM_ERROR_MPU_ERROR_CODE;
		}

		if ((regs->srr1 & MSR_PR) == 0) {
			/* kernel mode access */
			hm_exception(regs, 1, err, vector, fault, mcsr);
			goto out_rwx_error;
		}

		/* user access */
		if (((mcsr & (MCSR_BUS_WRERR|MCSR_BUS_DRERR)) != 0) &&
		    (regs->regs[1] == fault)) {
			/* stack access */
			err = HM_ERROR_STACK_OVERFLOW;
		}
		hm_exception(regs, 0, err, vector, fault, mcsr);

		/* w1c */
out_rwx_error:
		ppc_set_spr(SPR_MCSR, MCSR_MPU_BITS);
		return;
	}

	/* FIXME: further dispatching based on ESR possible */
	err = HM_ERROR_ASYNC_BUS_ERROR;

	hm_exception(regs, 1, HM_ERROR_ASYNC_BUS_ERROR, vector, fault, mcsr);
}

/* DSI, DTLB */
void ppc_handler_data(struct arch_reg_frame *regs, unsigned int vector)
{
	unsigned long fault;
	unsigned long esr;
	unsigned int err;

	fault = ppc_get_spr(SPR_DEAR);
	esr = ppc_get_spr(SPR_ESR);

	if (esr & ESR_ST) {
		err = HM_ERROR_MPU_ERROR_WRITE;
	} else {
		err = HM_ERROR_MPU_ERROR_READ;
	}

	if (esr & ESR_XTE) {
		err = HM_ERROR_SYNC_BUS_ERROR;
	}

	if ((regs->srr1 & MSR_PR) == 0) {
		/* exception in kernel */
		hm_exception(regs, 1, err, vector, fault, esr);
		return;
	}

	/* stack fault? */
	if (fault == regs->regs[1]) {
		err = HM_ERROR_STACK_OVERFLOW;
	}

	hm_exception(regs, 0, err, vector, fault, esr);
}

/* ISI, ITLB */
void ppc_handler_inst(struct arch_reg_frame *regs, unsigned int vector)
{
	unsigned long fault;
	unsigned long esr;
	unsigned int err;

	fault = regs->srr0;
	esr = ppc_get_spr(SPR_ESR);

	if (esr & ESR_XTE) {
		err = HM_ERROR_SYNC_BUS_ERROR;
	} else {
		err = HM_ERROR_MPU_ERROR_CODE;
	}

	if ((regs->srr1 & MSR_PR) == 0) {
		/* exception in kernel */
		hm_exception(regs, 1, err, vector, fault, esr);
		return;
	}

	hm_exception(regs, 0, err, vector, fault, esr);
}

/* ALIGN */
void ppc_handler_align(struct arch_reg_frame *regs, unsigned int vector)
{
	unsigned long fault;
	unsigned long esr;
	unsigned int err;

	fault = ppc_get_spr(SPR_DEAR);
	esr = ppc_get_spr(SPR_ESR);

	err = HM_ERROR_UNALIGNED_DATA;

	if ((regs->srr1 & MSR_PR) == 0) {
		/* exception in kernel */
		hm_exception(regs, 1, err, vector, fault, esr);
		return;
	}

	hm_exception(regs, 0, err, vector, fault, esr);
}

/* PROGRAM */
void ppc_handler_program(struct arch_reg_frame *regs, unsigned int vector)
{
	unsigned long fault;
	unsigned long esr;
	unsigned int err;

	fault = ppc_get_spr(SPR_DEAR);
	esr = ppc_get_spr(SPR_ESR);

	if (esr & ESR_PPR) {
		err = HM_ERROR_PRIVILEGED_INSTRUCTION;
	} else if (esr & ESR_PTR) {
		err = HM_ERROR_TRAP;
	} else {
		/* unimplemented or illegal (PIL) */
		err = HM_ERROR_ILLEGAL_INSTRUCTION;
	}

	if ((regs->srr1 & MSR_PR) == 0) {
		/* exception in kernel */
		hm_exception(regs, 1, err, vector, fault, esr);
		return;
	}

	hm_exception(regs, 0, err, vector, fault, esr);
}

/* IVOR 32, 33, and 34 */
void ppc_handler_spe(struct arch_reg_frame *regs, unsigned int vector)
{
	unsigned long esr;
	unsigned int err;

	esr = ppc_get_spr(SPR_ESR);

	if (vector == 32) {
		err = HM_ERROR_FPU_ACCESS;
	} else {
		err = HM_ERROR_FPU_ERROR;
	}

	if ((regs->srr1 & MSR_PR) == 0) {
		/* exception in kernel */
		hm_exception(regs, 1, err, vector, 0, esr);
		return;
	}

	hm_exception(regs, 0, err, vector, 0, esr);
}


__init void arch_init_exceptions(void)
{
	ppc_set_ivors();
}

/** switch over to the kernel stack and invoke next() */
__init void arch_switch_to_kernel_stack(void (*next)(void *))
{
	struct sched_state *sched = current_sched_state();

	ppc_switch_to_kernel_stack(sched->arch.kern_stack, next);
}

/*==================[internal function definitions]===========================*/
/*==================[end of file]=============================================*/
