/*
 * ppc_private.h
 *
 * PPC private functions
 *
 * azuepke, 2014-06-03: initial PPC port
 */

#ifndef __PPC_PRIVATE_H__
#define __PPC_PRIVATE_H__

#include <arch_regs.h>
#include <hv_compiler.h>

/* exception.c */
void ppc_handler_panic(struct arch_reg_frame *regs, unsigned int vector);
void ppc_handler_irq(struct arch_reg_frame *regs, unsigned int vector);
void ppc_handler_irq_crit(struct arch_reg_frame *regs, unsigned int vector);
void ppc_handler_mce(struct arch_reg_frame *regs, unsigned int vector);
void ppc_handler_data(struct arch_reg_frame *regs, unsigned int vector);
void ppc_handler_inst(struct arch_reg_frame *regs, unsigned int vector);
void ppc_handler_align(struct arch_reg_frame *regs, unsigned int vector);
void ppc_handler_program(struct arch_reg_frame *regs, unsigned int vector);
void ppc_handler_spe(struct arch_reg_frame *regs, unsigned int vector);

/* entry.S */
void ppc_switch_to_kernel_stack(unsigned long stack, void (*func)(void *)) __noreturn;
void ppc_set_ivors(void);

void ppc_spe_save(struct arch_fpu_frame *fpu);
void ppc_spe_restore(struct arch_fpu_frame *fpu);

#endif
