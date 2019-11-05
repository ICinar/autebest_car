/*
 * ppc_insn.h
 *
 * PPC architecture specific instructions.
 *
 * azuepke, 2013-11-21: initial PPC port
 */

#ifndef __PPC_INSN_H__
#define __PPC_INSN_H__

#include <ppc_spr.h>
#include <hv_compiler.h>

/** clear reservation */
static inline void ppc_clear_reservation(void)
{
	/* clear reservation by a dummy atomic store */
#ifdef CODEWARRIOR
	/* force variable allocation on stack, or compiler fails with "out of registers" */
	volatile unsigned int tmp;
#else
 	unsigned int tmp;
#endif
	__asm__ volatile ("stwcx. %0, 0, %1" : "=&r"(tmp) : "r"(&tmp) : "memory", "cr0");
}

/** Get PIR */
static inline unsigned long ppc_get_smp_id(void)
{
	return ppc_get_spr(SPR_PIR);
}

/** Get r2 */
static inline unsigned long ppc_get_r2(void)
{
#ifdef CODEWARRIOR
	unsigned long r2;

	__asm__ ("mr %0, r2" : "=r" (r2));

	return r2;
#elif defined VLE_ON
     register unsigned long r2 __asm__ ("r2");
     return r2;
#else
	register unsigned long ___r2 __asm__ ("r2");

#ifdef __clang__
	/* LLVM/Clang requires this, otherwise ___r2 is uninitialized */
	__asm__ ("" : "=r" (___r2));
#endif

	return ___r2;
#endif
}

/** Set r2 */
static inline void ppc_set_r2(unsigned long val)
{
#ifdef VLE_ON
    __asm__ volatile ("se_mr 2, %0" : : "r"(val) : "memory");
#else
	__asm__ volatile ("mr 2, %0" : : "r"(val) : "memory");
#endif
}


///--------------------------------------------------------------------------

/** Get USPRG0 */
static inline unsigned long ppc_get_usprg0(void)
{
	return ppc_get_spr(SPR_USPRG0);
}

/** Set USPRG0 */
static inline void ppc_set_usprg0(unsigned long val)
{
	ppc_set_spr(SPR_USPRG0, val);
}

///--------------------------------------------------------------------------

/** Get SPEFSCR */
static inline unsigned long ppc_get_spefscr(void)
{
#ifdef MPC5748G
	/* The CPU2 on Calypso does not have the register SPEFSCR (no floating point) */
	if ((ppc_get_spr(SPR_PIR) & 0xff) == 2) /* arch_cpu_id == 2*/
	{
		return 0u;
	}
	else
	{
		return ppc_get_spr(SPR_SPEFSCR);
	}
#else
	return ppc_get_spr(SPR_SPEFSCR);
#endif
}

/** Set SPEFSCR */
static inline void ppc_set_spefscr(unsigned long val)
{
#ifdef MPC5748G
   if ((ppc_get_spr(SPR_PIR) & 0xff) != 2)
   {
	   ppc_set_spr(SPR_SPEFSCR, val);
   }
#else
	ppc_set_spr(SPR_SPEFSCR, val);
#endif
}

///--------------------------------------------------------------------------

/** SYNC */
static inline void ppc_sync(void)
{
	__asm__ volatile ("sync" : : : "memory");
}

/** ISYNC */
static inline void ppc_isync(void)
{
#ifdef VLE_ON
    __asm__ volatile ("se_isync" : : : "memory");
#else
	__asm__ volatile ("isync" : : : "memory");
#endif
}

/** EIEIO */
static inline void ppc_eieio(void)
{
	__asm__ volatile ("eieio" : : : "memory");
}

/** TLBWE */
static inline void ppc_tlbwe(void)
{
	__asm__ volatile ("tlbwe" : : : "memory");
}

#endif
