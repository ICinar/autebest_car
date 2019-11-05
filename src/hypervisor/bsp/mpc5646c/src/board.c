/*
 * board.c
 *
 * Board initialization for MPC5646C.
 *
 * tjordan, 2014-07-14: ported to Bolero3M
 */

#include <kernel.h>
#include <assert.h>
#include <ppc_tlb.h>
#include <board.h>
#include <ppc_private.h>
#include <ppc_io.h>
#include <board_stuff.h>
#include <linker.h>
#include <sched.h>	/* num_cpus */
#include <hm.h>

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
__cold void __board_halt(void)
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

/* reset the board */
static inline void __board_reset(void)
{
	/* FIXME not clear: functional or destructive reset? */
	board_ME_switch(BOARD_ME_FRESET); /* try functional reset for now */

	__board_halt();
}

__cold void board_halt(haltmode_t mode)
{
	if ((mode == BOARD_RESET) || (mode == BOARD_HM_RESET)) {
		__board_reset();
	}

	/* just halt the machine */
	__board_halt();
}

void board_idle(void)
{
	unsigned long msr;

	/* only effective if HID0[DOZE] is enabled! */
	msr = ppc_get_msr();
	msr |= MSR_WE;

	/* see e500corerm 6.4.1 "Software Considerations for Power Management" */
	ppc_sync();
	ppc_set_msr(msr);
	ppc_isync();
	while (1) {
		/* wait and stay in idle mode */
	}
}

void __init board_mpu_init(void)
{
	/* PowerPC uses TLB entries set at boot */
}

void __init board_cpu0_up(void)
{
}

void __init board_startup_complete(void)
{
}

void board_nmi_dispatch(unsigned int vector __unused)
{
	hm_system_error(HM_ERROR_NMI, vector);
}

int board_hm_exception(
	struct arch_reg_frame *regs __unused,
	int fatal __unused,
	unsigned int hm_error_id __unused,
	unsigned long vector __unused,
	unsigned long fault_addr __unused,
	unsigned long aux __unused)
{
	return 0;	/* exception not handled */
}

void board_tp_switch(
	unsigned int prev_timepart __unused,
	unsigned int next_timepart __unused,
	unsigned int tpwindow_flags __unused)
{
}

void __init board_init_clocks(void)
{
	unsigned int tmp;

	/* set up system clock dividers */
	BOARD_CGM_Z0_DCR = BOARD_HIGHSPEED_DIVIDER;
	BOARD_CGM_FEC_DCR = BOARD_HIGHSPEED_DIVIDER;
	BOARD_CGM_FLASH_DCR = BOARD_HIGHSPEED_DIVIDER;
	BOARD_CGM_SC_DC0 = BOARD_CGM_SC_DCx_EN | (BOARD_DC0_VALUE - 1);
	BOARD_CGM_SC_DC1 = BOARD_CGM_SC_DCx_EN | (BOARD_DC1_VALUE - 1);
	BOARD_CGM_SC_DC2 = BOARD_CGM_SC_DCx_EN | (BOARD_DC2_VALUE - 1);

	/* select FIRC for PLL input */
	BOARD_CGM_AC0_SC = BOARD_CGM_AC0_SELCTL(BOARD_CGM_AC0_FIRC);

	/* set up PLL parameters */
	BOARD_PLL_CR =   BOARD_PLL_CR_IDF(BOARD_IDF_VALUE)
	               | BOARD_PLL_CR_ODF(BOARD_ODF_VALUE)
	               | BOARD_PLL_CR_NDIV(BOARD_NDIV_VALUE);

	/* configure RUN0 mode to activate and use FMPLL */
	tmp = BOARD_ME_RUN0_MC & BOARD_ME_MC_SYSCLK_IMASK;
	BOARD_ME_RUN0_MC = tmp | BOARD_ME_MC_FMPLLON | BOARD_ME_MC_SYSCLK_FMPLL;
	/* switch to RUN0 */
	board_ME_switch(BOARD_ME_RUN0);
}

void __init board_init(void)
{
	/* fire up cache */
	board_cache_init();

	serial_init(115200);
	printf("Starting up ...\n");

	printf("assuming ROM from %08x to %08x\n", BOARD_ROM_PHYS, BOARD_ROM_PHYS + BOARD_ROM_SIZE);
	printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
	printf("assuming RAM from %08x to %08x\n", BOARD_RAM_PHYS, BOARD_RAM_PHYS + BOARD_RAM_SIZE);
	printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
	printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);

	board_intc_init();

	ppc_timer_init(1000);	/* HZ -- FIXME: what's a good value here? */

	/* enter the kernel */
	/* NOTE: all processors take the same entry point! */
	kernel_main(0);
}
