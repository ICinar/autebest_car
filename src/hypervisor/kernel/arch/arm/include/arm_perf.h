/*
 * arm_perf.h
 *
 * ARM performance monitoring on ARMv7.
 *
 * azuepke, 2013-09-22: initial
 */

#ifndef __ARM_PERF_H__
#define __ARM_PERF_H__

/* ARMv7 defines a mandatory cycle counter and up to 32 individual counters */

/* bits in (PMNC) control register */
#define ARM_PERF_PMCR_NUM(pmnc)	((pmnc >> 11) & 0x1f)	/* get # of counters */
#define ARM_PERF_PMCR_DP	0x20	/* disable counter in prohibited regions */
#define ARM_PERF_PMCR_X		0x10	/* export of events to ETM */
#define ARM_PERF_PMCR_D		0x08	/* cycle counter counts every 64th cycle */
#define ARM_PERF_PMCR_C		0x04	/* cycle counter reset */
#define ARM_PERF_PMCR_P		0x02	/* event counter reset */
#define ARM_PERF_PMCR_E		0x01	/* enable all counters */

/* CCNT bit in the mask registers */
#define ARM_PERF_MASK_CCNT	0x80000000
#define ARM_PERF_MASK_ALL	0xffffffff

/* user enable bit */
#define ARM_PERF_USEREN		0x00000001


#ifndef __ASSEMBLER__

/* access to performance monitor registers (ARMv7) */

/** Get PMNC (Performance Monitor Control) Register */
static inline unsigned long arm_perf_get_ctrl(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
	return val;
}

/** Set PMNC (Performance Monitor Control) Register */
static inline void arm_perf_set_ctrl(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c12, 0" : : "r"(val) : "memory");
}


/** Get Count Enable (reads CNTENS Register) */
static inline unsigned long arm_perf_counter_enabled(void)
{
	unsigned long mask;
	__asm__ volatile ("mrc p15, 0, %0, c9, c12, 1" : "=r"(mask));
	return mask;
}

/** Set Count Enable (sets CNTENS Register) */
static inline void arm_perf_enable_counter(unsigned long mask)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c12, 1" : : "r"(mask) : "memory");
}

/** Clear Count Enable (sets CNTENC Register) */
static inline void arm_perf_disable_counter(unsigned long mask)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c12, 2" : : "r"(mask) : "memory");
}


/** Get PMOVSR (Performance Monitor Overflow Status Register) */
static inline unsigned long arm_perf_int_pending(void)
{
	unsigned long mask;
	__asm__ volatile ("mrc p15, 0, %0, c9, c12, 3" : "=r"(mask));
	return mask;
}

/** Clear bits in PMOVSR (Performance Monitor Overflow Status Register) */
static inline void arm_perf_int_ack(unsigned long mask)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c12, 3" : : "r"(mask) : "memory");
}


/** Increment Counter by Software (PMSWINC) */
static inline void arm_perf_inc_counter(unsigned long mask)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c12, 4" : : "r"(mask) : "memory");
}


/** Select an Event Counter Register (writes to PMSELR) */
static inline void arm_perf_select(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c12, 5" : : "r"(val) : "memory");
}


/** Get CCNT (Cycle Count) Register */
static inline unsigned long arm_perf_get_ccnt(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r"(val));
	return val;
}

/** Set CCNT (Cycle Count) Register */
static inline void arm_perf_set_ccnt(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c13, 0" : : "r"(val) : "memory");
}


/** Get Event Type (reads PMXEVTYPERx Register) */
static inline unsigned long arm_perf_get_type(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c9, c13, 1" : "=r"(val));
	return val;
}

/** Set Event Type (reads PMXEVTYPERx Register) */
static inline void arm_perf_set_type(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c13, 1" : : "r"(val) : "memory");
}


/** Get Event Count (reads PMNx Register) */
static inline unsigned long arm_perf_get_count(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c9, c13, 2" : "=r"(val));
	return val;
}

/** Set Event Count (reads PMNx Register) */
static inline void arm_perf_set_count(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c13, 2" : : "r"(val) : "memory");
}


/** Get USEREN (User Enable) Register */
static inline unsigned long arm_perf_get_useren(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c9, c14, 0" : "=r"(val));
	return val;
}

/** Set USEREN (User Enable) Register */
static inline void arm_perf_set_useren(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c14, 0" : : "r"(val) : "memory");
}


/** Get Interrupt Enable (reads INTENS Register) */
static inline unsigned long arm_perf_int_enabled(void)
{
	unsigned long mask;
	__asm__ volatile ("mrc p15, 0, %0, c9, c14, 1" : "=r"(mask));
	return mask;
}

/** Set Interrupt Enable (sets INTENS Register) */
static inline void arm_perf_int_unmask(unsigned long mask)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c14, 1" : : "r"(mask) : "memory");
}

/** Clear Interrupt Enable (sets INTENC Register) */
static inline void arm_perf_int_mask(unsigned long mask)
{
	__asm__ volatile ("mcr p15, 0, %0, c9, c14, 2" : : "r"(mask) : "memory");
}

#endif

#endif
