/*
 * arch_regs.h
 *
 * Register frame on PPC.
 *
 * azuepke, 2014-06-03: initial
 */

#ifndef __ARCH_REGS_H__
#define __ARCH_REGS_H__

#include <stdint.h>
#include <hv_compiler.h>

/** exception register frame (user registers) */
struct arch_reg_frame {
	uint32_t regs[32];
	uint32_t srr0;
	uint32_t srr1;
	uint32_t lr;
	uint32_t ctr;
	uint32_t cr;
	uint32_t xer;

	/* not saved on exception entry */
	uint32_t usprg0;
	uint32_t spefscr;
} __aligned(8);

/** FPU register frame (Freescale SPE FPU) */
struct arch_fpu_frame {
	uint32_t high[32];
	uint64_t acc;
};

/** Hardware Register Context */
struct arch_ctxt_frame {
	/* empty, no extra hardware contexts on PowerPC */
	unsigned char dummy;
};

#endif
