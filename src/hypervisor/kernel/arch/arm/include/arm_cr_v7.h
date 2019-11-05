/*
 * arm_cr_v7.h
 *
 * Bits in ARM specific control registers, safe to include from assembler.
 *
 * azuepke, 2013-09-11: initial
 * azuepke, 2013-11-24: MPU version
 */

#ifndef __ARM_CR_V7_H__
#define __ARM_CR_V7_H__

/** CPU modes in CPSRs */
#define CPSR_MODE_USER		0x10
#define CPSR_MODE_FIQ		0x11
#define CPSR_MODE_IRQ		0x12
#define CPSR_MODE_SVC		0x13
#define CPSR_MODE_MONITOR	0x16
#define CPSR_MODE_ABORT		0x17
#define CPSR_MODE_UNDEF		0x1b
#define CPSR_MODE_SYSTEM	0x1f
#define CPSR_MODE_MASK		0x1f

#ifndef __ASSEMBLER__

/** Set FIQ stack */
static inline void arm_set_fiq_stack(unsigned long val)
{
	__asm__ volatile (
		"cps	%1\n"
		"mov	sp, %0\n"
		"cps	%2\n"
		: : "r"(val), "i"(CPSR_MODE_FIQ), "i"(CPSR_MODE_SVC) : "memory");
}

#endif

/** CPSR bits */
#define CPSR_T				0x00000020	/* Thumb enable */
#define CPSR_F				0x00000040	/* FIQ disable */
#define CPSR_I				0x00000080	/* IRQ disable */
#define CPSR_A				0x00000100	/* asynchronous abort disable */
#define CPSR_E				0x00000200	/* big endian */

#define CPSR_IT1			0x0000fc00	/* Thumb2 IT flags */
#define CPSR_GE				0x000f0000	/* GE SIMD flags */

#define CPSR_J				0x01000000	/* Jazelle */
#define CPSR_IT2			0x06000000	/* Thumb2 IT flags */
#define CPSR_Q				0x08000000	/* Q flag */
#define CPSR_V				0x10000000	/* V flag */
#define CPSR_C				0x20000000	/* C flag */
#define CPSR_Z				0x40000000	/* Z flag */
#define CPSR_N				0x80000000	/* N flag */


/** CPSR default user bits */
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define CPSR_USER_BITS		0x00000210	/* user mode, big endian */
#define CPSR_IDLE_BITS		0x00000213	/* SVC mode, big endian */
#else
#define CPSR_USER_BITS		0x00000010	/* user mode, little endian */
#define CPSR_IDLE_BITS		0x00000013	/* SVC mode, little endian */
#endif

/** FPEXC bits */
#define FPEXC_EX			0x80000000	/* exception, save FPINST */
#define FPEXC_EN			0x40000000	/* enable */
#define FPEXC_DEX			0x20000000	/* unsupported vector instruction */
#define FPEXC_FP2V			0x10000000	/* exception, save FPINST2 */


/** SCTLR -- System Control Register (cp15, c1, c0, 0) */
/* NOTE: enables a feature when the bit is set, otherwise documented */
#define SCTLR_M				0x00000001	/* MMU/MPU */
#define SCTLR_A				0x00000002	/* strict alignment */
#define SCTLR_C				0x00000004	/* data/unified cache */
#define SCTLR_W				0x00000008	/* write buffer (ARMv6) */

#define SCTLR_B				0x00000080	/* big endian */
#define SCTLR_S				0x00000100	/* system protection (deprecated) */
#define SCTLR_R				0x00000200	/* ROM protection (deprecated) */
#define SCTLR_F				0x00000400	/* implementation defined (ARMv6) */
#define SCTLR_SW			0x00000400	/* SWP (ARMv7) */

#define SCTLR_Z				0x00000800	/* branch prediction */
#define SCTLR_I				0x00001000	/* instruction cache */
#define SCTLR_V				0x00002000	/* high vectors */
#define SCTLR_RR			0x00004000	/* round-robin replacement in caches */
#define SCTLR_L4			0x00008000	/* inhibit Thumb interworking (ARMv6) */

#define SCTLR_BR			0x00020000	/* MPU background region enable (MPU) */
#define SCTLR_HA			0x00020000	/* hardware access flag (ARMv7) */
#define SCTLR_DZ			0x00080000	/* divide-by-zero generates an UNDEF exception */
#define SCTLR_FI			0x00200000	/* low interrupt latency */
#define SCTLR_U				0x00400000	/* unaligned loads and stores */
#define SCTLR_XP			0x00800000	/* ARMv6 MMU mode */
#define SCTLR_VE			0x01000000	/* vectored interrupts */
#define SCTLR_EE			0x02000000	/* big endian exception entry */
#define SCTLR_L2			0x04000000	/* L2 unified cache (ARMv6) */

#define SCTLR_NMFI			0x08000000	/* non-maskable FIQs (ARMv7) */
#define SCTLR_TRE			0x10000000	/* TEX remapping (ARMv7) */
#define SCTLR_AFE			0x20000000	/* force AP bit (ARMv7) */
#define SCTLR_TE			0x40000000	/* thumb exception (ARMv7) */
#define SCTLR_IE			0x80000000	/* big-endian instructions */

/** SCTLR default bits: caches on, MMU disabled, vectors low */
/* vectors must be set to "low", so we can override the vectors with VBAR */
/* SCTLR_C, SCTLR_I and SCTLR_W are only enabled when caches are available */
/* SCTLR_EE and SCTLR_B depend on the processor's endianess */
/* NOTE: SCTLR_M must be set by board code */
#define SCTLR_CLR			(SCTLR_TE | SCTLR_AFE | SCTLR_TRE | \
							 SCTLR_NMFI | SCTLR_L2 | \
							 SCTLR_VE | SCTLR_FI | SCTLR_HA | \
							 SCTLR_L4 | SCTLR_RR | SCTLR_V | SCTLR_R | \
							 SCTLR_S  | SCTLR_A | SCTLR_M)
#define SCTLR_SET			(SCTLR_XP | SCTLR_U | \
							 SCTLR_Z | SCTLR_W)


/** ACTLR -- Auxiliary Control Register (cp15, c1, c0, 1) */
/* Implementation defined -- check manuals! */


/** CPACR -- Coprocessor Access Control Register (cp 15, c1, c0, 1) */
#define CPACR_CP10			0x00300000	/* enable access to CP10 */
#define CPACR_CP11			0x00c00000	/* enable access to CP11 */
#define CPACR_TTA			0x10000000	/* trap user access to trace regs (ARMv8) */
#define CPACR_D32DIS		0x40000000	/* disable D16-D31 (ARMv7) */
#define CPACR_ASEDIS		0x80000000	/* disable SIMD (ARMv7) */

/** DFSR and IFSR status bits */
#define FSR_FSMASK			0x000f
#define FSR_FSBIT4			0x0400
#define FSR_WRITE			0x0800
#define FSR_EXTABT			0x1000

#endif
