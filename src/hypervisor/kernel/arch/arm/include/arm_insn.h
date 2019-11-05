/*
 * arm_insn.h
 *
 * ARM architecture specific instructions.
 *
 * azuepke, 2013-09-11: initial
 * azuepke, 2013-11-24: initial MPU version
 */

#ifndef __ARM_INSN_H__
#define __ARM_INSN_H__

/** get CPSR */
static inline unsigned long arm_get_cpsr(void)
{
	unsigned long ret;
	__asm__ ("mrs %0, cpsr" : "=r" (ret));
	return ret;
}

/** set CPSR */
static inline void arm_set_cpsr(unsigned long val)
{
	__asm__ volatile ("msr cpsr, %0" : : "r" (val) : "memory", "cc");
}

#ifdef ARM_CORTEXM
/** set CONTROL */
static inline void arm_set_control(unsigned long val)
{
	__asm__ volatile ("msr control, %0" : : "r" (val) : "memory", "cc");
}

/** set PSP */
static inline void arm_set_psp(unsigned long val)
{
	__asm__ volatile ("msr psp, %0" : : "r" (val) : "memory", "cc");
}
#endif


#ifndef ARM_CORTEXM
/** get FPEXC */
static inline unsigned long arm_get_fpexc(void)
{
	unsigned long ret;
	__asm__ ("mrc p10, 7, %0, c8, c0, 0" : "=r" (ret));
	return ret;
}

/** set FPEXC */
static inline void arm_set_fpexc(unsigned long val)
{
	__asm__ volatile ("mcr p10, 7, %0, c8, c0, 0" : : "r" (val) : "memory");
}
#endif


/** get FPSCR */
static inline unsigned long arm_get_fpscr(void)
{
	unsigned long ret;
#ifdef ARM_CORTEXM
	__asm__ ("vmrs %0, fpscr" : "=r" (ret));
#else
	__asm__ ("mrc p10, 7, %0, c1, c0, 0" : "=r" (ret));
#endif
	return ret;
}

/** set FPSCR */
static inline void arm_set_fpscr(unsigned long val)
{
#ifdef ARM_CORTEXM
	__asm__ volatile ("vmsr fpscr, %0" : : "r" (val) : "memory");
#else
	__asm__ volatile ("mcr p10, 7, %0, c1, c0, 0" : : "r" (val) : "memory");
#endif
}


/** save FPREGS d0..d31 */
static inline void arm_vstm_d0_d31(uint64_t *fpregs)
{
#ifdef ARM_CORTEXM
	__asm__ volatile (
		"vstm	%0!, {d0-d15}"
		: "=r"(fpregs) : "0" (fpregs) : "memory");
#else
	__asm__ volatile (
		"stc	p11, cr0, [%0], #128\n"		/* vstm		r0!, {d0-d15} */
#ifdef ARM_VFP32
		"stcl	p11, cr0, [%0], #128\n"		/* vstm		r0!, {d16-d31} */
#endif
		: "=r"(fpregs) : "0" (fpregs) : "memory");
#endif
}

/** load FPREGS d0..d31 */
static inline void arm_vldm_d0_d31(uint64_t *fpregs)
{
#ifdef ARM_CORTEXM
	__asm__ volatile (
		"vldm	%0!, {d0-d15}"
		: "=r"(fpregs) : "0" (fpregs) : "memory");
#else
	__asm__ volatile (
		"ldc	p11, cr0, [%0], #128\n"		/* vldm		r0!, {d0-d15} */
#ifdef ARM_VFP32
		"ldcl	p11, cr0, [%0], #128\n"		/* vldm		r0!, {d16-d31} */
#endif
		: "=r"(fpregs) : "0" (fpregs) : "memory");
#endif
}


/** ISB */
static inline void arm_isb(void)
{
	__asm__ volatile ("isb" : : : "memory");
}

/** DSB */
static inline void arm_dsb(void)
{
	__asm__ volatile ("dsb" : : : "memory");
}

/** DMB */
static inline void arm_dmb(void)
{
	__asm__ volatile ("dmb" : : : "memory");
}

/** CLREX */
static inline void arm_clrex(void)
{
	__asm__ volatile ("clrex" : : : "memory");
}

/** YIELD */
static inline void arm_yield(void)
{
	__asm__ volatile ("yield" : : : "memory");
}

/** SEV */
static inline void arm_sev(void)
{
	__asm__ volatile ("sev" : : : "memory");
}

/** WFE */
static inline void arm_wfe(void)
{
	__asm__ volatile ("wfe" : : : "memory");
}

/** WFI */
/* NOTE: put DSB before */
static inline void arm_wfi(void)
{
	__asm__ volatile ("wfi" : : : "memory");
}



#ifndef ARM_CORTEXM
/** clean a data cache line */
static inline void arm_clean_dcache(void *addr)
{
	__asm__ volatile ("mcr p15, 0, %0, c7, c10, 1" : : "r"(addr) : "memory");
	arm_dsb();
}

/** flush a data cache line */
static inline void arm_flush_dcache(void *addr)
{
	__asm__ volatile ("mcr p15, 0, %0, c7, c14, 1" : : "r"(addr) : "memory");
	arm_dsb();
}

/** invalidate branch prediction */
/* NOTE: requires special synchronization! */
static inline void arm_inval_bp(void)
{
	__asm__ volatile ("mcr p15, 0, %0, c7, c5, 6" : : "r"(0) : "memory");
}


/** Get MPIDR (multiprocessor ID register, for MPCore) */
static inline unsigned long arm_get_mpidr(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r"(val));
	return val;
}


/** Get TLS0 (user writable context register) */
static inline unsigned long arm_get_tls0(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c13, c0, 2" : "=r"(val));
	return val;
}

/** Set TLS0 (user writable context register) */
static inline void arm_set_tls0(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c13, c0, 2" : : "r"(val) : "memory");
}



/** Get TLS1 (user readable context register) */
static inline unsigned long arm_get_tls1(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c13, c0, 3" : "=r"(val));
	return val;
}

/** Set TLS1 (user readable context register) */
static inline void arm_set_tls1(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c13, c0, 3" : : "r"(val) : "memory");
}



/** Get TLS2 (user inaccessible context register) */
static inline unsigned long arm_get_tls2(void)
{
	unsigned long val;
	__asm__ volatile ("mrc p15, 0, %0, c13, c0, 4" : "=r"(val));
	return val;
}

/** Set TLS2 (user inaccessible context register) */
static inline void arm_set_tls2(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c13, c0, 4" : : "r"(val) : "memory");
}
#endif


#ifndef ARM_CORTEXM
/* ARM Cortex R4 MPU */

/** set MPU region number */
static inline void arm_mpu_rgnr(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c6, c2, 0" : : "r"(val) : "memory");
}

/** set MPU region base address register */
static inline void arm_mpu_base(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c6, c1, 0" : : "r"(val) : "memory");
}

/** set MPU region size and enable register */
static inline void arm_mpu_size_enable(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c6, c1, 2" : : "r"(val) : "memory");
}

/** set MPU region access control */
static inline void arm_mpu_access_control(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c6, c1, 4" : : "r"(val) : "memory");
}
#endif

#ifdef ARM_MMU
/** Set CONTEXT ID (ASID) */
static inline void arm_set_asid(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c13, c0, 1" : : "r"(val) : "memory");
}

/** Set TTBR0 (user page table) */
static inline void arm_set_ttbr0(unsigned long val)
{
	__asm__ volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(val) : "memory");
}
#endif

#endif
