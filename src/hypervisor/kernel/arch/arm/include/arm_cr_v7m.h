/*
 * arm_cr_v7m.h
 *
 * Bits in ARM specific control registers, safe to include from assembler.
 *
 * azuepke, 2015-06-26: clone from v7
 */

#ifndef __ARM_CR_V7M_H__
#define __ARM_CR_V7M_H__

#ifndef __ASSEMBLER__
#define _REG32(addr) (*(volatile unsigned int *)(addr))
#define _REG16(addr) (*(volatile unsigned short *)(addr))
#define _REG8(addr)  (*(volatile unsigned char *)(addr))
#else
#define _REG32(addr) (addr)
#define _REG16(addr) (addr)
#define _REG8(addr)  (addr)
#endif

/** CPACR */
#define CPACR	_REG32(0xe000ed88)

#define CPACR_CP10			0x00300000	/* enable access to CP10 */
#define CPACR_CP11			0x00c00000	/* enable access to CP11 */

#define FPCCR	_REG32(0xe000ef34)
#define FPCAR	_REG32(0xe000ef38)
#define FPDSCR	_REG32(0xe000ef3c)

/** xPSR bits */
#define CPSR_EXC_MASK		0x000001ff	/* IPSR */
#define CPSR_STK			0x00000200	/* Unaligned stack requires extra pop */

#define CPSR_THUMB			0x01000000	/* Thumb state */
#define CPSR_Q				0x08000000	/* Q flag */
#define CPSR_V				0x10000000	/* V flag */
#define CPSR_C				0x20000000	/* C flag */
#define CPSR_Z				0x40000000	/* Z flag */
#define CPSR_N				0x80000000	/* N flag */

/** CPSR default user bits */
#define CPSR_USER_BITS		CPSR_THUMB
#define CPSR_IDLE_BITS		CPSR_THUMB

/** EXC_RETURN: return to Handler or Thread mode, using Main or Process stack. */
#define EXC_RETURN_H		0xfffffff1
#define EXC_RETURN_TM		0xfffffff9
#define EXC_RETURN_TP		0xfffffffd
#define EXC_RETURN_FP_H		0xffffffe1
#define EXC_RETURN_FP_TM	0xffffffe9
#define EXC_RETURN_FP_TP	0xffffffed

/** CONTROL bits */
#define CONTROL_NPRIV	0x1
#define CONTROL_SPSEL	0x2
#define CONTROL_FPCA	0x4

/* misc registers */
#define ACTLR	_REG32(0xe000e008)

#define ICSR	_REG32(0xe000ed04)
#define ICSR_NMIPENDSET		0x80000000	/* Set NMI to pending */
#define ICSR_PENDSVSET		0x10000000
#define ICSR_PENDSVCLR		0x08000000	/* Clear pending PendSV */
#define ICSR_PENDSTSET		0x04000000
#define ICSR_PENDSTCLR		0x02000000	/* Clear pending SysTick */

#define VTOR	_REG32(0xe000ed08)

#define AIRCR	_REG32(0xe000ed0c)
#define AIRCR_VECTKEY		0x05fa0000
#define AIRCR_SYSRESETREQ	0x00000004

#define SCR		_REG32(0xe000ed10)

#define CCR		_REG32(0xe000ed14)
#define CCR_STKALIGN		0x00000200
#define CCR_BFHFNMIGN		0x00000100
#define CCR_DIV_0_TRP		0x00000010
#define CCR_UNALIGN_TRP		0x00000008
#define CCR_USERSETMPEND	0x00000002
#define CCR_NONBASETHRDENA	0x00000001

#define SHPR1	_REG32(0xe000ed18)
#define SHPR2	_REG32(0xe000ed1c)
#define SHPR3	_REG32(0xe000ed20)

#define SHCSR	_REG32(0xe000ed24)
#define SHCSR_USGFAULTENA	0x00040000
#define SHCSR_BUSFAULTENA	0x00020000
#define SHCSR_MEMFAULTENA	0x00010000

#define CFSR	_REG32(0xe000ed28)

/* UFSR bits in CFSR */
#define CFSR_DIVBYZERO		0x02000000
#define CFSR_UNALIGNED		0x01000000
#define CFSR_NOCP			0x00080000
#define CFSR_INVPC			0x00040000
#define CFSR_INVSTATE		0x00020000
#define CFSR_UNDEFINSTR		0x00010000

/* BFSR bits in CFSR */
#define CFSR_BFARVALID		0x00008000
#define CFSR_LSPERR			0x00002000
#define CFSR_STKERR			0x00001000
#define CFSR_UNSTKERR		0x00000800
#define CFSR_IMPRECISERR	0x00000400
#define CFSR_PRECISERR		0x00000200
#define CFSR_IBUSERR		0x00000100

/* MMFSR bits in CFSR */
#define CFSR_MMARVALID		0x00000080
#define CFSR_MLSPERR		0x00000020
#define CFSR_MSTKERR		0x00000010
#define CFSR_MUNSTKERR		0x00000008
#define CFSR_DACCVIOL		0x00000002
#define CFSR_IACCVIOL		0x00000001

#define HFSR	_REG32(0xe000ed2c)
#define HFSR_DEBUG_VT		0x80000000
#define HFSR_FORCED			0x40000000
#define HFSR_VECTTBL		0x00000002

/* DFSR bits */
#define DFSR	_REG32(0xe000ed30)
#define DFSR_EXTERNAL		0x00000010
#define DFSR_VCATCH			0x00000008
#define DFSR_DWTTRAP		0x00000004
#define DFSR_BKPT			0x00000002
#define DFSR_HALTED			0x00000001

#define MMFAR	_REG32(0xe000ed34)

#define BFAR	_REG32(0xe000ed38)

#define AFSR	_REG32(0xe000ed3c)

/* MPU */
#define MPU_CTRL	_REG32(0xe000ed94)
#define MPU_CTRL_PRIVDEFENA	0x00000004
#define MPU_CTRL_HFNMIENA	0x00000002
#define MPU_CTRL_ENABLE		0x00000001

#define MPU_RNR		_REG32(0xe000ed98)
#define MPU_RBAR	_REG32(0xe000ed9c)
#define MPU_RASR	_REG32(0xe000eda0)


#endif
