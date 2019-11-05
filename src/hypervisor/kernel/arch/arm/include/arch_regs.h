/*
 * arch_regs.h
 *
 * Register frame on ARM.
 *
 * azuepke, 2013-11-24: initial MPU version
 * azuepke, 2015-01-05: optimized register layout
 */

#ifndef __ARCH_REGS_H__
#define __ARCH_REGS_H__

#include <stdint.h>
#include <hv_compiler.h>

#ifdef ARM_VFP32
#define NUM_FPREGS 32
#elif defined ARM_VFP16
#define NUM_FPREGS 16
#else
#define NUM_FPREGS 0
#endif

#ifdef ARM_CORTEXM
/** M3/M4 auto-saved registers */
struct arch_exception_frame {
	/* volatile registers */
	unsigned long r0;
	unsigned long r1;
	unsigned long r2;
	unsigned long r3;
	unsigned long r12;
	unsigned long lr;
	unsigned long pc;
	unsigned long xpsr;
} __aligned(8);
#endif

/** exception register frame (user registers) */
struct arch_reg_frame {
#ifdef ARM_CORTEXM
	unsigned long r4[8];	/* non-volatile registers r4 to r11 */

	struct arch_exception_frame *psp;
	unsigned long xlr;		/* exception return */
#else
	unsigned long regs[16];
	unsigned long cpsr;

	/* not saved on exception entry */
	unsigned long fpscr;
	unsigned long tls0;
	unsigned long tls1;
#endif
} __aligned(8);

/** FPU register frame */
struct arch_fpu_frame {
#ifdef ARM_CORTEXM
	unsigned long fpscr;
	unsigned long padding;
#endif

	/* NOTE: we keep the FPSCR in the integer context */
	uint64_t fpregs[NUM_FPREGS];
};

/** Hardware Context */
struct arch_ctxt_frame {
	/* empty, no extra hardware contexts on ARM */
	unsigned char dummy;
};

#endif
