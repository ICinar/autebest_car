/*
 * board.c
 *
 * Board initialization for MPC5646C.
 *
 * tjordan, 2014-07-14: ported to Bolero3M
 */

#include <ppc_msr.h>
#include <ppc_insn.h>
#include <test_support.h>
#include <board_stuff.h>

/* FMPLL registers */
#define BOARD_PLL_CR		(*((volatile unsigned int *) 0xC3FE00A0u))
/* bits and shift values for register fields */
#define BOARD_PLL_CR_IDF(n)		((n) << 26)
#define BOARD_PLL_CR_ODF(n)		((n) << 24)
#define BOARD_PLL_CR_NDIV(n)	((n) << 16)
#define BOARD_PLL_CR_SLOCK		0x08u

/* system clock dividers */
#define BOARD_CGM_Z0_DCR	(*((volatile unsigned char *) 0xC3FE00C0))
#define BOARD_CGM_FEC_DCR	(*((volatile unsigned char *) 0xC3FE00E0))
#define BOARD_CGM_FLASH_DCR	(*((volatile unsigned char *) 0xC3FE0120))
#define BOARD_CGM_SC_DC0	(*((volatile unsigned char *) 0xC3FE037C))
#define BOARD_CGM_SC_DC1	(*((volatile unsigned char *) 0xC3FE037D))
#define BOARD_CGM_SC_DC2	(*((volatile unsigned char *) 0xC3FE037E))
#define BOARD_CGM_SC_DCx_EN	0x80u

#define BOARD_CGM_AC0_SC	(*((volatile unsigned int *) 0xC3FE0380u))
#define BOARD_CGM_AC0_SELCTL(n)		((n) << 24)
#define BOARD_CGM_AC0_FIRC			0x01u

/* Mode Entry registers */
#define BOARD_ME_GS			(*((volatile unsigned int *) 0xC3FDC000u))
#define BOARD_ME_MCTL		(*((volatile unsigned int *) 0xC3FDC004u))
#define BOARD_ME_ME			(*((volatile unsigned int *) 0xC3FDC008u))
#define BOARD_ME_RUN0_MC	(*((volatile unsigned int *) 0xC3FDC030u))
#define BOARD_ME_RUN1_MC	(*((volatile unsigned int *) 0xC3FDC034u))

#define BOARD_ME_MC_FMPLLON			0x40u
#define BOARD_ME_MC_SYSCLK_IMASK	0xFFFFFFF0u
#define BOARD_ME_MC_SYSCLK_FIRC		0x00u
#define BOARD_ME_MC_SYSCLK_FMPLL	0x04u

/* Mode Entry register bits/masks - only those we care about */
#define BOARD_ME_GS_MTRANS	0x08000000u
#define BOARD_ME_MCTL_KEY	0x00005AF0u
#define BOARD_ME_MCTL_IKEY	0x0000A50Fu
#define BOARD_ME_ME_VALUE	0x0000A01Du /* reset value + enable STANDBY */
#define BOARD_ME_MASK_MODE	0xF0000000u
#define BOARD_ME_MASK_CLK	0x0000000Fu

/* Mode Entry target modes - suitable to use in MCTL and GS */
enum board_ME_targets {
	BOARD_ME_FRESET		=	0x00000000u,
	BOARD_ME_DRUN		=	0x30000000u,
	BOARD_ME_RUN0		=	0x40000000u,
	BOARD_ME_RUN1		=	0x50000000u,
	BOARD_ME_STANDBY	=	0xD0000000u,
	BOARD_ME_DRESET		=	0xF0000000u
};

static inline void board_ME_switch(enum board_ME_targets target)
{
	BOARD_ME_MCTL = target | BOARD_ME_MCTL_KEY;
	BOARD_ME_MCTL = target | BOARD_ME_MCTL_IKEY;
	while ((BOARD_ME_GS & BOARD_ME_GS_MTRANS) != 0) {
		/* wait for mode transition to complete */
	}
	while ((BOARD_ME_GS & BOARD_ME_MASK_MODE) != target) {
		/* target mode not reached, something's horribly wrong - loop forever */
	}
}


/* enter idle mode with all interrupts disabled */
void board_halt(void)
{
	unsigned long msr;

	/* disable all interrupts */
	msr = ppc_get_msr();
	msr &= ~(MSR_EE | MSR_CE | MSR_ME | MSR_DE);
	msr |= MSR_WE;

	ppc_sync();
	ppc_set_msr(msr);
	ppc_isync();
	/* now switch to low-power mode, i.e. STANDBY */
	board_ME_switch(BOARD_ME_STANDBY);
	while (1) {
		/* wait and stay in idle mode, with interrupts disabled */
	}
}
