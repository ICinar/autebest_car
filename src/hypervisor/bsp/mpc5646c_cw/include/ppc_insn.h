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

/** clear reservation */
static inline void ppc_clear_reservation(void)
{
	/* clear reservation by a dummy atomic store */
	unsigned int tmp;
	__asm__ volatile ("stwcx. %0, 0, %1" : "=&r"(tmp) : "r"(&tmp) : "memory", "cr0");
}

/** Get PIR */
static inline unsigned long ppc_get_smp_id(void)
{
	return ppc_get_spr(SPR_PIR);
}

/** Get r2 */
#if 0
static inline unsigned long ppc_get_r2(void)
{
	register unsigned long x __asm__ ("r2");
	return x;
}
#endif

/** Set r2 */
static inline void ppc_set_r2(unsigned long val)
{
	__asm__ volatile ("mr 2, %0" : : "r"(val) : "memory");
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
	return ppc_get_spr(SPR_SPEFSCR);
}

/** Set SPEFSCR */
static inline void ppc_set_spefscr(unsigned long val)
{
	ppc_set_spr(SPR_SPEFSCR, val);
}

///--------------------------------------------------------------------------

/** SYNC */
static inline void ppc_sync(void)
{
#if 0
	__asm__ volatile ("sync" : : : "memory");
#else
	__asm__ volatile ("isync" : : : "memory");
#endif
}

/** ISYNC */
static inline void ppc_isync(void)
{
	__asm__ volatile ("isync" : : : "memory");
}

/** EIEIO */
static inline void ppc_eieio(void)
{
	__asm__ volatile ("eieio" : : : "memory");
}

///--------------------------------------------------------------------------

/** read 64-bit timebase */
#if 0
static inline unsigned long long ppc_get_timebase(void)
{
	unsigned long u1, u2;
	unsigned long lo;

	do {
		u1 = ppc_get_spr_barrier(SPR_TBLU);
		lo = ppc_get_spr_barrier(SPR_TBLL);
		u2 = ppc_get_spr_barrier(SPR_TBLU);
	} while (u1 != u2);

	return ((unsigned long long)u1 << 32) | lo;
}
#endif
/** write 64-bit timebase */
static inline void ppc_set_timebase(unsigned long long tb)
{
	/* first set lower word to zero to prevent a wrap-around */
	ppc_set_spr(SPR_TBL_W, 0);
	ppc_set_spr(SPR_TBU_W, (unsigned long)(tb >> 32));
	ppc_set_spr(SPR_TBL_W, (unsigned long)(tb & 0xffffffff));
}

///--------------------------------------------------------------------------

/** TLBSYNC */
static inline void ppc_tlbsync(void)
{
	__asm__ volatile ("tlbsync" : : : "memory");
}

/** TLBRE */
static inline void ppc_tlbre(void)
{
	__asm__ volatile ("tlbre" : : : "memory");
}

/** TLBWE */
static inline void ppc_tlbwe(void)
{
	__asm__ volatile ("tlbwe" : : : "memory");
}

/** TLBRE */
static inline void ppc_tlbsx(unsigned long addr)
{
	__asm__ volatile ("tlbsx 0, %0" : : "r"(addr) : "memory");
}

/** TLBIVAX */
#define TLBIVAX_TBL0	(0<<3)
#define TLBIVAX_TBL1	(1<<3)
#define TLBIVAX_INV_ALL	(1<<2)
static inline void ppc_tlbivax(unsigned long addr)
{
	__asm__ volatile ("tlbivax 0, %0" : : "r"(addr) : "memory");
}

#endif
