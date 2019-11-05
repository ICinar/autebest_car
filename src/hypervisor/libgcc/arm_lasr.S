/*
 * arm_lasr.S
 *
 * Shift routine for ARM
 *
 * See the following document for the ABI specification:
 * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0043d/IHI0043D_rtabi.pdf
 *
 * azuepke, 2013-09-17: initial
 * azuepke, 2014-02-02: moved to kernel/lib/libgcc_arm.S
 * azuepke, 2015-03-27: split into dedicated files
 */

#include <assembler.h>

	.text
	.syntax unified

#ifdef __ARMEB__
#define lo r1
#define hi r0
#else
#define lo r0
#define hi r1
#endif

/*
 * ARM 64-bit shifts:
 *
 * long long __aeabi_lasr(long long, int);
 */
FUNC_PROLOG(__aeabi_lasr)
	subs	r3, r2, #32
	rsb		ip, r2, #32
	lsrmi	lo, lo, r2
	asrpl	lo, hi, r3
#if 1 // defined __thumb__
	lslmi	r3, hi, ip
	orrmi	lo, lo, r3
#else
	orrmi	lo, lo, hi, lsl ip
#endif
	asr		hi, hi, r2
	bx		lr
FUNC_EPILOG(__aeabi_lasr)
