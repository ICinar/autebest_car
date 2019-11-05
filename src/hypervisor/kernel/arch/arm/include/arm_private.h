/*
 * arm_private.h
 *
 * ARM private functions
 *
 * azuepke, 2013-09-11: initial ARM port
 */

#ifndef __ARM_PRIVATE_H__
#define __ARM_PRIVATE_H__

#include <arch_regs.h>
#include <hv_compiler.h>

#ifdef ARM_CORTEXM

/* exception_m.c */
void arm_memory_handler_user(struct arch_reg_frame *regs);
void arm_memory_handler_kern(struct arch_reg_frame *regs);
void arm_usage_handler_user(struct arch_reg_frame *regs);
void arm_usage_handler_kern(struct arch_reg_frame *regs);
void arm_bus_handler_user(struct arch_reg_frame *regs);
void arm_bus_handler_kern(struct arch_reg_frame *regs);
void arm_debug_handler_user(struct arch_reg_frame *regs);
void arm_debug_handler_kern(struct arch_reg_frame *regs);
void arm_hard_handler_user(struct arch_reg_frame *regs);
void arm_hard_handler_kern(struct arch_reg_frame *regs);
void arm_irq_handler(unsigned int vector);
void arm_nmi_handler(struct arch_reg_frame *regs);

#else

/* exception.c */
void arm_irq_handler(struct arch_reg_frame *regs);
void arm_fiq_handler(struct arch_reg_frame *regs);

void arm_dabt_handler_user(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr);
void arm_dabt_handler_kern(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr);
void arm_pabt_handler_user(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr);
void arm_pabt_handler_kern(struct arch_reg_frame *regs, unsigned long addr, unsigned long fsr);
void arm_undef_handler_user(struct arch_reg_frame *regs);
void arm_undef_handler_kern(struct arch_reg_frame *regs);

/* entry.S */
void arm_vector_undef(void);
void arm_vector_pabt(void);
void arm_vector_dabt(void);
void arm_vector_irq(void);
void arm_vector_fiq(void);
void arm_vector_svc(void);
void arm_switch_to_kernel_stack(unsigned long stack, void (*func)(void *)) __noreturn;

#endif

#endif
