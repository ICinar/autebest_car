/*
 * ppc_msr.h
 *
 * Bits in PPC specific control registers, safe to include from assembler.
 *
 * azuepke, 2013-11-21: initial PPC port
 * azuepke, 2014-06-03: adaptions to e200
 */

#ifndef __PPC_MSR_H__
#define __PPC_MSR_H__

#ifndef __ASSEMBLER__

/** get MSR */
static inline unsigned long ppc_get_msr(void)
{
	unsigned long ret;
	__asm__ ("mfmsr %0" : "=r" (ret));
	return ret;
}

/** set MSR */
static inline void ppc_set_msr(unsigned long val)
{
	__asm__ volatile ("mtmsr %0" : : "r" (val) : "memory");
}

#endif

/** MSR bits */
#define MSR_RI				0x00000002	/* recoverable interrupt (sticky) */
#define MSR_PMM				0x00000004	/* performance monitor mark */
#define MSR_DS				0x00000010	/* data space */
#define MSR_IS				0x00000020	/* instruction space */
#define MSR_FE1				0x00000100	/* FPU exception mode 1 */
#define MSR_DE				0x00000200	/* debug interrupt enable */
#define MSR_FE0				0x00000800	/* FPU exception mode 0 */
#define MSR_ME				0x00001000	/* machine exception enable */
#define MSR_FP				0x00002000	/* FPU enable */
#define MSR_PR				0x00004000	/* privilege level (user mode) */
#define MSR_EE				0x00008000	/* external interrupt enable */
#define MSR_CE				0x00020000	/* critical interrupt enable */
#define MSR_WE				0x00040000	/* wait state enable (idle CPU) */
#define MSR_SPE				0x02000000	/* SPE enable */
#define MSR_GS				0x10000000	/* guest state (500mc) */

#endif
