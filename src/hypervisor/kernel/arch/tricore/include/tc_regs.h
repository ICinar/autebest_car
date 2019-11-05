/*
 * tc_regs.h
 *
 * Tricore architecture specific registers.
 *
 * azuepke, 2014-10-25: initial
 */

#ifndef __TC_REGS_H__
#define __TC_REGS_H__

/* IDs of Core Special Function Registers (CSFRs) */
#define CSFR_PC			0xfe08	/* program counter */
#define CSFR_PSW		0xfe04	/* program status word */
#define CSFR_PCXI		0xfe00	/* previous context and interrupt state */
#define CSFR_FCX		0xfe38	/* free context */
#define CSFR_LCX		0xfe3c	/* last context */

#define CSFR_SYSCON		0xfe14
#define CSFR_CPU_ID		0xfe18
#define CSFR_CORE_ID	0xfe1c

#define CSFR_BIV		0xfe20	/* base interrupt vector table pointer */
#define CSFR_BTV		0xfe24	/* base trap vector table pointer */
#define CSFR_ISP		0xfe28	/* interrupt stack pointer */
#define CSFR_ICR		0xfe2c	/* interrupt control register */

/* Trap Status Registers */
#define CSFR_PSTR		0x9200	/* Program Synchronous Error Trap */
#define CSFR_DSTR		0x9010	/* Data Synchronous Error Trap */
#define CSFR_DATR		0x9018	/* Data Asynchronous Error Trap */
#define CSFR_DEADD		0x901c	/* Data Error Address Register */

#define CSFR_PIETR		0x9214	/* Program Integrity Error Trap */
#define CSFR_PIEAR		0x9210	/* Program Integrity Error Address */
#define CSFR_DIETR		0x9024	/* Data Integrity Error Trap */
#define CSFR_DIEAR		0x9020	/* Data Integrity Error Address */

/* Memory Type Registers */
#define CSFR_PMA0		0x8100	/* Segment Data Accesses Cacheablilty (DAC) */
#define CSFR_PMA1		0x8104	/* Segment Code Accesses Cacheablilty (CAC) */
#define CSFR_PMA2		0x8108	/* Segment Peripheral Space Designator (PSD) */

/* Program and Data Memory Configuration Registers */
#define CSFR_PCON0		0x920c
#define CSFR_PCON1		0x9204
#define CSFR_PCON2		0x9208
#define CSFR_DCON0		0x9040
#define CSFR_DCON1		0x9008
#define CSFR_DCON2		0x9000

/* Debug and Performance Counters */
#define CSFR_CCTRL		0xfc00	/* Counter Control Register */
#define CSFR_CCNT		0xfc04	/* CPU Clock Count Register */
#define CSFR_ICNT		0xfc08	/* Instruction Count Register */
#define CSFR_M1CNT		0xfc0c	/* Multi Count Register 1 */
#define CSFR_M2CNT		0xfc10	/* Multi Count Register 2 */
#define CSFR_M3CNT		0xfc14	/* Multi Count Register 3 */
#define CSFR_DBGSR		0xfd00  /* Debug Status Register DBGST */

/* Special values of register field CPU.DBGST.HALT */
#define CPU_DBGST_HALT_RUN     (0)
#define CPU_DBGST_HALT_HALT    (1)

/* Structure of register field CPU.DBGST.HALT */

/* HALT is 2 bits wide */
#define CPU_DBGSR_HALT_BITS    (0x3)
/* Offset of the HALT bits */
#define CPU_DBGSR_HALT_OFF     (1)
/* Mask of the HALT bitfield */
#define CPU_DBGSR_HALT_MASK    (CPU_DBGSR_HALT_BITS << CPU_DBGSR_HALT_OFF)

#ifdef TRICORE_TC161
/* TC1.6.1 defines a simpler MPU model without range pairs */
/* MPU registers: access permissions */
#define CSFR_CPXE(x)	(0xe000+4*(x))	/* Set 0..3 execute permissions */
#define CSFR_DPRE(x)	(0xe010+4*(x))	/* Set 0..3 read permissions */
#define CSFR_DPWE(x)	(0xe020+4*(x))	/* Set 0..3 write permissions */

/* MPU registers: ranges */
#define CSFR_DPRL(x)	(0xc000+8*(x))	/* Data prot. range 0..15 lower bound */
#define CSFR_DPRU(x)	(0xc004+8*(x))	/* Data prot. range 0..15 upper bound */
#define CSFR_CPRL(x)	(0xd000+8*(x))	/* Code prot. range 0..15 lower bound */
#define CSFR_CPRU(x)	(0xd004+8*(x))	/* Code prot. range 0..15 upper bound */

#else
/* TC1.3 to TC1.6 define a pair baised range model */
#error TC1.6 pair based range model not implemented!

#endif


/* Bits in PSW (program status word) */
/* Integer ALU flags */
#define PSW_C			0x80000000	/* Carry */
#define PSW_V			0x40000000	/* Overflow */
#define PSW_SV			0x20000000	/* Sticky Overflow */
#define PSW_AV			0x10000000	/* Advance Overflow */
#define PSW_SAV			0x08000000	/* Sticky Advance Overflow */

/* FPU flags (in PSW) */
#define PSW_FS			0x80000000	/* Some Exception */
#define PSW_FI			0x40000000	/* Invalid Operation */
#define PSW_FV			0x20000000	/* Overflow */
#define PSW_FZ			0x10000000	/* Divide by zero */
#define PSW_FU			0x08000000	/* Underflow */
#define PSW_FX			0x04000000	/* Inexact */
#define PSW_RM(x)		((x)<<24)	/* FPU rounding mode selection */

#define PSW_S			0x00004000	/* Safe Task */
#define PSW_PRS(x)		((x)<<12)	/* Protection Register Set 0..3 */
#define PSW_IO_SVC		0x00000800	/* 00: User-0, 01: User-1, 10: Supervisor */
#define PSW_IO_USER1	0x00000400
#define PSW_IO_USER0	0x00000000

#define PSW_IS			0x00000200	/* Shared Interrupt Stack */
#define PSW_GW			0x00000100	/* Global Address Register Write Permission */

#define PSW_CDE			0x00000080	/* Call Depth Counter Enable */
									/* Bits 6..0: Call Depth Counter */


/** PSW default user bits */
#define PSW_USER_BITS		(PSW_PRS(1) | PSW_IO_USER0 | PSW_CDE)

/** PSW default kernel bits */
#define PSW_IDLE_BITS		(PSW_PRS(0) | PSW_S | PSW_IO_SVC | PSW_IS | PSW_CDE)


/* Convert CSAs in PCXI, FCX, and LCX to pointer and vice versa */
#define CX_S_MASK		0x000f0000	/* Segment */
#define CX_O_MASK		0x0000ffff	/* Offset */
#define PTR_TO_CX(ptr)	((((unsigned long)(ptr) >> 12) & CX_S_MASK) | (((unsigned long)(ptr) >> 6) & CX_O_MASK))
#define CX_TO_PTR(cx)	((void*)((((cx) & CX_S_MASK) << 12) | ((cx) & CX_O_MASK) << 6))

/* Additional Bits in PCXI */
#ifdef TRICORE_TC161
/* TC1.6.1 states that PCPN, PIE and UL bits are shifted right by two bits! */
#define PCXI_PCPN(x)	((x) >> 22) & 0xff)	/* Previous CPU Priority Number */
#define PCXI_PIE		0x00200000	/* Previous Interrupt Enable */
#define PCXI_UL			0x00100000	/* Context type: 1=Upper Context, 0=Lower */
#else
/* TC1.3 and TC1.6 */
#define PCXI_PCPN(x)	((x) >> 24) & 0xff)	/* Previous CPU Priority Number */
#define PCXI_PIE		0x00800000	/* Previous Interrupt Enable */
#define PCXI_UL			0x00400000	/* Context type: 1=Upper Context, 0=Lower */
#endif

/* Test bits for PCXI */
#define CX_IS_UPPER(cx)	(((cx) & PCXI_UL) != 0)
#define CX_IS_LOWER(cx)	(((cx) & PCXI_UL) == 0)

/* Bits in ICR (Interrupt Control Register) */
#define ICR_PIPN(x)		(((x) >> 16) & 0xff)	/* pending interrupt priority number */
#ifdef TRICORE_TC161
/* TC1.6.1 uses bit 15 for IE */
#define ICR_IE			0x8000					/* interrupts enabled */
#else
/* TC1.3 and TC1.6 use bit 8 instead */
#define ICR_IE			0x0100					/* interrupts enabled */
#endif
#define ICR_CCPN(x)		(((x) >> 0) & 0xff)		/* current CPU priority number */


/* Bits in BIV */
#ifdef TRICORE_TC161
/* TC1.6.1 only: VSS allows either 8 byte or 32 byte spacing between vectors */
#define BIV_VSS			0x1		/* Vector Size Select: 8 byte spacing between */
/* NOTE: for single vector mode, place BIV at offset 0x7f8 with 8 bytes spacing */
#endif

/* Bits in SYSCON */
#define SYSCON_U1_IOS	0x00020000	/* Allow User-1 to access all peripherals */
#define SYSCON_U1_IED	0x00010000	/* Allow User-1 to disable interrupts */
#define SYSCON_TS		0x00000010	/* PSW.S bit for trap handlers */
#define SYSCON_IS		0x00000008	/* PSW.S bit for interrupt handlers */
#define SYSCON_TPROTEN	0x00000004	/* Temporal Protection Enable */
#define SYSCON_PROTEN	0x00000002	/* Memory Protection Enable */
#define SYSCON_FCDSF	0x00000001	/* Free Context List Depleted Sticky Flag */

/* Trap Classes */
#define TRAP_MMU		0	/* MMU */
#define TRAP_PROT		1	/* Internal Protection */
#define TRAP_INST		2	/* Instruction Errors */
#define TRAP_CTXT		3	/* Context Management */
#define TRAP_BUS		4	/* System Bus and Peripheral Errors */
#define TRAP_ASSERT		5	/* Assertion Traps */
#define TRAP_SYS		6	/* System Call */
#define TRAP_NMI		7	/* NMI */


/* Trap Identification Numbers (TIN) */
							/* Class 0 -- MMU */
#define TIN_VAF			0
#define TIN_VAP			1

							/* Class 1 -- Internal Protection */
#define TIN_PRIV		1
#define TIN_MPR			2
#define TIN_MPW			3
#define TIN_MPX			4
#define TIN_MPP			5
#define TIN_MPN			6
#define TIN_GRWP		7

							/* Class 2 -- Instruction Errors */
#define TIN_IOPC		1
#define TIN_UOPC		2
#define TIN_OPD			3
#define TIN_ALN			4
#define TIN_MEM			5

							/* Class 3 -- Context Management */
#define TIN_FCD			1
#define TIN_CDO			2
#define TIN_CDU			3
#define TIN_FCU			4
#define TIN_CSU			5
#define TIN_CTYP		6
#define TIN_NEST		7

							/* Class 4 -- System Bus and Peripheral Errors */
#define TIN_PSE			1
#define TIN_DSE			2
#define TIN_DAE			3
#define TIN_CAE			4
#define TIN_PIE			5
#define TIN_DIE			6
#define TIN_TAE			7

							/* Class 5 -- Assertion Traps */
#define TIN_OVF			1
#define TIN_SOVF		2


#ifndef __ASSEMBLER__

/** move from core register */
#define MFCR(reg)	({	\
		unsigned long __ret;	\
		__asm__ volatile ("mfcr %0, %1" : "=d"(__ret) : "i"(reg) : "memory");	\
		__ret;	\
	})

/** move to core register */
#define MTCR(reg, val)	({	\
		__asm__ volatile ("mtcr %1, %0" : : "d"(val), "i"(reg) : "memory");	\
	})

/** move to core register + isync */
#define MTCR_ISYNC(reg, val)	({	\
		__asm__ volatile ("mtcr %1, %0 \n isync" : : "d"(val), "i"(reg) : "memory");	\
	})

/** ISYNC */
#define ISYNC() __asm__ volatile ("isync" : : : "memory")

/** DSYNC */
#define DSYNC() __asm__ volatile ("dsync" : : : "memory")

/** RSTV -- Reset Overflow Bits in PSW */
#define RSTV() __asm__ volatile ("rstv" : : : "memory")

/** Get a8 */
static inline void *tc_get_a8(void)
{
	register void *___a8 __asm__ ("a8");

#ifdef __clang__
	/* LLVM/Clang requires this, otherwise ___a8 is uninitialized */
	__asm__ ("" : "=r" (___a8));
#endif

	return ___a8;
}

/** Set a8 */
static inline void tc_set_a8(void *___a8)
{
	unsigned long psw;

	psw = MFCR(CSFR_PSW);
	MTCR_ISYNC(CSFR_PSW, psw | PSW_GW);
	__asm__ volatile ("mov.aa %%a8, %0" : : "a"(___a8) : "memory");
	MTCR_ISYNC(CSFR_PSW, psw);
}

/** Set a0 and a1 (user __SDATA__ stuff) */
static inline void tc_set_a0_a1(void *___a0, void *___a1)
{
	unsigned long psw;

	psw = MFCR(CSFR_PSW);
	MTCR_ISYNC(CSFR_PSW, psw | PSW_GW);
	__asm__ volatile ("mov.aa %%a0, %0" : : "a"(___a0) : "memory");
	__asm__ volatile ("mov.aa %%a1, %0" : : "a"(___a1) : "memory");
	MTCR_ISYNC(CSFR_PSW, psw);
}

#endif

#endif
