/*
 * arch_regs.h
 *
 * Register frame on Tricore.
 *
 * azuepke, 2014-10-25: initial
 */

#ifndef __ARCH_REGS_H__
#define __ARCH_REGS_H__

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <hv_compiler.h>

/*
 * Register Usage:
 *
 * a0       _SMALL_DATA_ (adspace specific)
 * a1       _SMALL_DATA2_ (adspace specific)
 * a2..3    return values
 * a4..7    arguments <<-- a4 is used for arg0!
 * a8       kernel specific
 * a9       kernel specific (currently not used)
 * a10      Stack Pointer, stack grows down, always aligned to 8 bytes
 * a11      Return Address, Initial PC
 * a12..15  scratch
 *
 * d0..1    scratch
 * d2..3    return values
 * d4..7    arguments
 * d8..15   scratch
 */

/*
 * Register Context Save Areas (CSAs):
 *
 * In a task's CSA area, the place the contexts in the following order:
 *    0: LOWER (always saved by SVLCX, PCX points to entry #1)
 *    1: initial UPPER on task start (PCX = NULL)
 *    2: 1st free (FCX points here, starting the free chain)
 *    3: 2nd free
 *       ...
 *  n-3: third last free
 *  n-2: second last free (LCX points here)
 *  n-1: last free (end of free chain, PCX = NULL)
 *
 * On initial start of a task, the initial UPPER context becomes free on RFE
 * and is usable for the task's user space CALL & RETURN operations.
 *
 * On kernel entry, the UPPER context is saved in any of the free slots
 * and the LOWER context is stored in the first. LOWER's PCX points to
 * the UPPER entry for context restoration.
 *
 * When the CSA becomes full, an FCD trap triggers, saving UPPER in the second
 * last slot and leaving the last entry free for NMIs or ASYNC traps.
 *
 * When an FCD trap triggers in the kernel entry stubs on kernel entry,
 * the trap handler let the kernel continue normally and ignores the trap,
 * as the kernel will switch to the kernel's CSA.
 *
 * ---
 *
 * We maintain a dedicated CSA for each task, and a dedicated CSA for each
 * partition's kernel stack. Additionally, we have an IRQ CSA for interrupts
 * and an NMI CSA for NMIs.
 */

/** Hardware Context (lower or upper context, 16 byte aligned) */
struct arch_ctxt_frame {
	uint32_t pcxi;
	union {
		/* lower context */
		struct {
			uint32_t pc;		/* a11 = PC */
			uint32_t a2;
			uint32_t a3;

			uint32_t d0;		/* OUT2 */
			uint32_t d1;		/* OUT3 */
			uint32_t d2;		/* return value */
			uint32_t d3;		/* return value 64-bit upper bits / OUT1 */

			uint32_t a4;		/* first pointer argument */
			uint32_t a5;
			uint32_t a6;
			uint32_t a7;

			uint32_t d4;
			uint32_t d5;
			uint32_t d6;
			uint32_t d7;
		} l;
		/* upper context */
		struct {
			uint32_t psw;
			uint32_t a10;		/* a10 = SP */
			uint32_t a11;		/* a11 = user RA */

			uint32_t d8;
			uint32_t d9;
			uint32_t d10;
			uint32_t d11;

			uint32_t a12;
			uint32_t a13;
			uint32_t a14;
			uint32_t a15;

			uint32_t d12;
			uint32_t d13;
			uint32_t d14;
			uint32_t d15;
		} u;
	} cx;
} __aligned(64);

/** exception register frame (user registers) */
struct arch_reg_frame {
	/* const -- set once by arch_ctxt_frame_init() etc */
	struct arch_ctxt_frame *csa;	/* pointer to context save area (CSA) */
	/* NOTE: the first array entry is used to save the task's LOWER entry! */
	uint32_t lcx;		/* LCX (points to &csa[csa_num - 2]) */
	uint32_t csa_num;	/* number of contexts in CSA */

	/* dynamic */
	uint32_t a9;		/* thread local storage */
} __aligned(16);

/** FPU register frame */
struct arch_fpu_frame {
	/* empty, FPU instructions use integer registers */
};

#endif

/** offsets in struct arch_reg_frame */
#define REGS_LOWER		 0
#define REGS_LCX		 4
#define REGS_CSA_NUM	 8
#define REGS_A9			12

/** offsets in struct arch_ctxt_frame */
#define CTXT_CSA		 0
#define CTXT_PCXI		 0
#define CTXT_PC			 4
#define CTXT_D0			16
#define CTXT_D2			24
#define CTXT_SIZE		64

#endif
