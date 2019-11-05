/*
 * arm_uidivmod.S
 *
 * Division routine for ARM
 *
 * The division routines are based on example code from ARM:
 * http://infocenter.arm.com/help/topic/com.arm.doc.dui0473c/CEGECDGD.html
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

/*
 * ARM unsigned integer division and modulo
 *
 * unsigned __aeabi_uidiv(unsigned numerator, unsigned denominator);
 *
 * typedef struct {
 *     unsigned quot;
 *     unsigned rem;
 * } uidiv_return;
 * __value_in_regs uidiv_return __aeabi_uidivmod(unsigned numerator, unsigned denominator);
 *
 */
FUNC_PROLOG(__aeabi_uidivmod)
FUNC_PROLOG(__aeabi_uidiv)
	mov		r2, r1
	mov		r3, r1
	mov		r1, r0

	cmp		r3, r1, lsr #1
1:	movls	r3, r3, lsl #1
	cmp		r3, r1, lsr #1
	bls		1b

	mov		r0, #0
2:	cmp		r1, r3
	subcs	r1, r1, r3
	adc		r0, r0, r0
	mov		r3, r3, lsr #1
	cmp		r3, r2
	bhs		2b

	bx		lr
FUNC_EPILOG(__aeabi_uidivmod)
FUNC_EPILOG(__aeabi_uidiv)
