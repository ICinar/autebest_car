/*
 * ppc_spr.h
 *
 * Common PowerPC SPRs.
 *
 * azuepke, 2013-11-22: initial PPC port
 */

#ifndef __PPC_SPR_H__
#define __PPC_SPR_H__

#ifndef __ASSEMBLER__

//#include <hv_compiler.h>
#define __stringify2(x...)	#x
#define __stringify(x...)	__stringify2(x)

/* NOTE: evil magic ahead! We can't use static inliners here! */

/** get SPR */
#define ppc_get_spr(spr) ({	\
		unsigned long __val;	\
		__asm__ volatile ("mfspr %0," __stringify(spr) : "=r" (__val));	\
		__val;	\
	})

/** get SPR with implicit barrier */
#define ppc_get_spr_barrier(spr) ({	\
		unsigned long __val;	\
		__asm__ volatile ("mfspr %0," __stringify(spr) : "=r" (__val) : : "memory");	\
		__val;	\
	})

/** set SPR (barrier always included) */
#define ppc_set_spr(spr, val)	\
		__asm__ volatile ("mtspr " __stringify(spr) ", %0" : : "r" (val) : "memory")

#endif


/* general purpose SPRs */
#define SPR_SPRG0		272
#define SPR_SPRG1		273
#define SPR_SPRG2		274
#define SPR_SPRG3		275		/* user readable via 259 */
#define SPR_SPRG4		276		/* user readable via 260 */
#define SPR_SPRG5		277		/* user readable via 261 */
#define SPR_SPRG6		278		/* user readable via 262 */
#define SPR_SPRG7		279		/* user readable via 263 */

/* USPRG0 / */
#define SPR_USPRG0		256

/* decrementer */
#define SPR_DEC			22		/* decrementer */
#define SPR_DECAR		54		/* decrementer auto reload */
#define SPR_TCR			340		/* timer control register */
#define SPR_TSR			336		/* timer status register, W1C semantic */

/* time base */
#define SPR_TBL			268		/* time base lower / upper (read only, user) */
#define SPR_TBU			269
#define SPR_TBL_W		284		/* time base lower / upper (write only) */
#define SPR_TBU_W		285

#define SPR_ATBL		526		/* alternative time base */
#define SPR_ATBU		527

/* configuration registers */
#define SPR_PIR			286		/* processor ID register */
#define SPR_PVR			287		/* processor version register */
#define SPR_SVR			1023	/* system version register */

/* exception handling */
#define SPR_SRR0		26		/* save/restore registers 0/1 */
#define SPR_SRR1		27

#define SPR_CSRR0		58		/* critical SRR0/1 */
#define SPR_CSRR1		59

#define SPR_MCSRR0		570		/* machine check SRR0/1 */
#define SPR_MCSRR1		571

#define SPR_ESR			62		/* exception syndrome register */
#define SPR_DEAR		61		/* data exception address register */
#define SPR_MCSR		572		/* machine check syndrome register */
#define SPR_MCAR		573		/* machine check address register */

/* exception vectors */
#define SPR_IVPR		63
#define SPR_IVOR0		400
#define SPR_IVOR1		401
#define SPR_IVOR2		402
#define SPR_IVOR3		403
#define SPR_IVOR4		404
#define SPR_IVOR5		405
#define SPR_IVOR6		406
#define SPR_IVOR7		407
#define SPR_IVOR8		408
#define SPR_IVOR9		409
#define SPR_IVOR10		410
#define SPR_IVOR11		411
#define SPR_IVOR12		412
#define SPR_IVOR13		413
#define SPR_IVOR14		414
#define SPR_IVOR15		415

#define SPR_IVOR32		528
#define SPR_IVOR33		529
#define SPR_IVOR34		530
#define SPR_IVOR35		531

/* debug registers */
#define SPR_DBCR0		308		/* debug control registers */
#define SPR_DBCR1		309
#define SPR_DBCR2		310
#define SPR_DBSR		304		/* debug status register */

#define SPR_IAC1		312		/* instruction address compare */
#define SPR_IAC2		313
#define SPR_DAC1		316		/* data address compare */
#define SPR_DAC2		317

/* SPE */
#define SPR_SPEFSCR		512		/* SPE FP status/control register */

/* branch target buffer registers  */
#define SPR_BBEAR		513		/* branch buffer entry address register */
#define SPR_BBTAR		514		/* branch buffer target address register */
#define SPR_BUCSR		1013	/* branch control and status register */

/* L1 cache configuration registers */
#define SPR_L1CFG0		515		/* data cache */
#define SPR_L1CFG1		516		/* instruction cache */
#define SPR_L1CSR0		1010
#define SPR_L1CSR1		1011

/* special purpose registers */
#define SPR_HID0		1008
#define SPR_HID1		1009

/* MMU control and status */
#define SPR_MMUCSR0		1012
#define SPR_MAS0		624
#define SPR_MAS1		625
#define SPR_MAS2		626
#define SPR_MAS3		627
#define SPR_MAS4		628
#define SPR_MAS5		629
#define SPR_MAS6		630
#define SPR_MAS7		944
#define SPR_PID0		48
#define SPR_PID1		633
#define SPR_PID2		634

#define SPR_MMUCFG		1015	/* read only */
#define SPR_TLB0CFG		688		/* read only */
#define SPR_TLB1CFG		689		/* read only */


/* bits in ESR (Exception Syndrome Register, currently only e500 bits) */
#define ESR_SPE			0x00000080	/* SPE exception */
#define ESR_BO			0x00020000	/* byte-ordering */
#define ESR_ILK			0x00100000	/* instruction cache locking */
#define ESR_DLK			0x00200000	/* data cache locking */
#define ESR_ST			0x00800000	/* store */
#define ESR_PTR			0x02000000	/* trap */
#define ESR_PPR			0x04000000	/* privileged instruction */
#define ESR_PIL			0x08000000	/* illegal instruction */

#endif
