/**
* \file hsm_board.c
*
* \brief HSM (board) initialization.
*
* \author easycore GmbH, 91058 Erlangen, Germany
*
* \par  License
* Customer: @@LicenseCustomer@@,
* License type: @@LicenseType@@,
* Licensed for project: @@LicenseProject@@.

* Copyright 2015 easycore GmbH. All rights exclusively reserved for easycore GmbH,
* unless expressly agreed to otherwise.
*/
/*==================[inclusions]==================================================================*/
#include <hsm_board.h>
#include <Hsm.h>
#include <hsm2ht.h>

#ifdef SECURE_BOOT
#include <hsm_secure_boot.h>
#endif

/*==================[macros]======================================================================*/
#define HSM_CR_KEY_1  0x55AAAA55u
#define HSM_CR_KEY_2  0x11335577u
#define MDUR_BIT      1u
#define HSM_CR_ADDR   0xa3f84000u
#define HSM_CR        *((volatile unsigned int *) HSM_CR_ADDR)

/* Mode Entry registers */
#define BOARD_ME_GS             (*((volatile unsigned int *) 0xFFFB8000u))
#define BOARD_ME_MCTL           (*((volatile unsigned int *) 0xFFFB8004u))
#define BOARD_ME_ME             (*((volatile unsigned int *) 0xFFFB8008u))
#define BOARD_ME_CS             (*((volatile unsigned int *) 0xFFFB81C0u))

/* RUN0 Mode Configuration Register (MC_ME_RUN0_MC) */
#define BOARD_ME_RUN0_MC        (*((volatile unsigned int *) 0xFFFB8030u))
#define BOARD_ME_RUN1_MC        (*((volatile unsigned int *) 0xFFFB8034u))

/* Bits of the MC_ME_RUN0_MC */
#define BOARD_ME_RUN0_MC_SYSCLK_PLL (0x00000002u)
#define BOARD_ME_RUN0_MC_FIRCON     (0x00000010u)
#define BOARD_ME_RUN0_MC_FXOSCON    (0x00000020u)
#define BOARD_ME_RUN0_MC_PLLON      (0x00000040u)
#define BOARD_ME_RUN0_MC_FLAON      (0x00030000u) /* Flash is in RUN mode */
#define BOARD_ME_RUN0_MC_MVRON      (0x00100000u) /* Main voltage regulator is switched on */

/* Run Peripheral Configuration Registers 0 and 1 (MC_ME_RUN_PCn) */
#define BOARD_ME_RUN_PC0        (*((volatile uint32_t*) 0xFFFB8080u))
#define BOARD_ME_RUN_PC1        (*((volatile uint32_t*) 0xFFFB8084u))

/*  Peripheral Control Registers */

#define BOARD_ME_PCTL_BASE      (0xFFFB80C0u)
#define BOARD_ME_PCTL_SIUL      (*(volatile uint32_t*) (BOARD_ME_PCTL_BASE + 94))

#define BOARD_ME_MC_FMPLLON             0x40u
#define BOARD_ME_MC_SYSCLK_IMASK        0xFFFFFFF0u
#define BOARD_ME_MC_SYSCLK_FIRC         0x00u
#define BOARD_ME_MC_SYSCLK_FMPLL        0x04u

/* Mode Entry register bits/masks - only those we care about */
#define BOARD_ME_GS_MTRANS      0x08000000u
#define BOARD_ME_MCTL_KEY       0x00005AF0u
#define BOARD_ME_MCTL_IKEY      0x0000A50Fu
#define BOARD_ME_ME_ALL_MODES   0x0000A5FDu /* Enable all modes */
#define BOARD_ME_MASK_MODE      0xF0000000u
#define BOARD_ME_MASK_CLK       0x0000000Fu

/*==================[type definitions]============================================================*/
typedef void (*waitloop_code_ptr_t)(unsigned int, unsigned int, unsigned int);

/* Mode Entry target modes - suitable to use in MCTL and GS */
enum board_ME_targets
{
    BOARD_ME_FRESET         =       0x00000000u,
    BOARD_ME_DRUN           =       0x30000000u,
    BOARD_ME_RUN0           =       0x40000000u,
    BOARD_ME_RUN1           =       0x50000000u,
    BOARD_ME_STANDBY        =       0xD0000000u,
    BOARD_ME_DRESET         =       0xF0000000u
};

/*==================[internal function declarations]==============================================*/
void hsm_board_init(void);

/*==================[external function declarations]==============================================*/

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/
/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  Code for the HSM <-> Host handshake
 *
 * This array actually contains VLE code to signal the host core to run and waits for the host
 * until further initialization by the host is ready. 
 *
 * \param r3 Address of HT2HSMF.
 * \param r4 Value to write at HSM2HTF.
 * \param r5 Address of HSM2HTF.
 */
/* -----------------------------------------------------------------------------------------------*/
static __attribute__((aligned(32))) unsigned char waitloop_code_vle[] =
{
    0x54, 0x85, 0x00, 0x00, /* e_stw   r4,0(r5) */
    0x50, 0x83, 0x00, 0x00, /* e_lwz   r4,0(r3) */
    0x22, 0x14, 0xe2, 0xfd, /* se_cmpli r4,2 ; se_bne -6 */
    0x00, 0x04, 0x00, 0x04  /* se_blr ; se_blr */
};

/*==================[internal function definitions]===============================================*/
/* -----------------------------------------------------------------------------------------------*/
/**
 * \brief  HSM <-> Host handshake
 *
 * This function first signals the host to run (can be used for SECURE BOOT!).
 * Then, the HSM waits (in RAM) for further initialization by the host core (clock, flash, ...).
 */
/* -----------------------------------------------------------------------------------------------*/
static void hsm_wait_for_host(void)
{
  HSM2HTS = HSM_VERSION;

  (*((waitloop_code_ptr_t)waitloop_code_vle))(HT2HSMF_A, 1, HSM2HTF_A);
  HT2HSMF = 2;
}

static inline void hsm_board_ME_switch(enum board_ME_targets target)
{
    BOARD_ME_MCTL = target | BOARD_ME_MCTL_KEY;
    BOARD_ME_MCTL = target | BOARD_ME_MCTL_IKEY;

    while ((BOARD_ME_GS & BOARD_ME_GS_MTRANS) != 0)
    {
        /* wait for mode transition to complete */
    }

    while ((BOARD_ME_GS & BOARD_ME_MASK_MODE) != target)
    {
        /* target mode not reached, something's horribly wrong - loop forever */
    }
}

/*==================[external function definitions]===============================================*/
void hsm_board_reset(void)
{
    /* FIXME not clear: functional or destructive reset? */
    hsm_board_ME_switch(BOARD_ME_FRESET); /* try functional reset for now */
}

void hsm_read_mdur
(
   unsigned char *result
)
{
   *result = (unsigned char) ((HSM_CR & (1 << MDUR_BIT)) >> MDUR_BIT);
}

void hsm_toggle_mdur
(
   void
)
{
   HSM_CR = HSM_CR_KEY_1;
   HSM_CR = HSM_CR_KEY_2;

   HSM_CR ^= (1 << MDUR_BIT);

   /* to take effect, the board has to be reset */
   hsm_board_reset();
}

void hsm_board_init(void)
{
	/* disable SWT */
	SWT_HSM_SR = 0xc520;
	SWT_HSM_SR = 0xd928;
	SWT_HSM_CR = 0xFF00010A;

	/* enable read + write buffer */
	HSBI_BCR = 3;
#ifndef DISABLE_HWOPT
	/* invalidate + enable icache */
		/* enable cache */
		HSBI_CCR = 2;  /* Invalidate cache */
		while(HSBI_CCR == 2) { /* Wait for invalidate to complete */ }
		HSBI_CCR = 1;  /* Enable cache */
#endif

#ifdef SECURE_BOOT
   hsm_secure_boot();
#endif

	/* wait for main core to init board stuff */
	hsm_wait_for_host();

	/* main function does communication */
	hsm_main();

   while (42) { /* loop forever */}
}

/*==================[end of file]=================================================================*/       
