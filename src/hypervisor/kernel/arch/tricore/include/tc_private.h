/*
 * tc_private.h
 *
 * Tricore private functions
 *
 * azuepke, 2014-12-19: initial
 */

#ifndef __TC_PRIVATE_H__
#define __TC_PRIVATE_H__

#include <arch_regs.h>
#include <hv_compiler.h>

/* exception.c */
__tc_fastcall void tc_handler_nmi(void *lower, unsigned long higher_cx);
__tc_fastcall void tc_handler_trap_kern(void *lower, unsigned long higher_cx, unsigned long class, unsigned long tin);
__tc_fastcall void tc_handler_trap_user(void *lower, unsigned long higher_cx, unsigned long class, unsigned long tin);
__tc_fastcall void tc_handler_fcu(unsigned long higher_cx, unsigned long pc);

/* entry.S */
void tc_trap_table_start(void);
void tc_trap_table_end(void);
void tc_switch_to_kernel_stack(
	void *stack,
	void (*next)(void *),
	unsigned long pcxi,
	unsigned long fcx,
	unsigned long lcx,
	unsigned long psw) __noreturn __tc_fastcall;

#endif
