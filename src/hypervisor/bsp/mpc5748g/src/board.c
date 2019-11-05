/**
 * \file      board.c
 * \brief     Board specific functions and initialization for MPC5748G Calypso.
 * \details
 *
 * \see       board.h for documentation of the most board functions defined in
 *            this file.
 *
 * \date      14.07.2014
 *
 * \author    tjordan
 * \author    Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author    easycore GmbH, 91058 Erlangen, Germany
 * \version
 * \par       History
 * - tjordan, 2014-07-14: ported to Bolero3M
 * \par       License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 *
 * \copyright Copyright 2015 easycore GmbH, 91058 Erlangen, Germany.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include <kernel.h>
#include <assert.h>
#include <board.h>
#include <board_stuff.h>
#include <linker.h>
#include <sched.h>      /* num_cpus */
#include <hm.h>
#include "memif.h"
#include "leds.h"
#include "smpu.h"
#include "stm.h"
#include "clocks.h"
#include "intc.h"

#ifdef HSM
#include <ht2hsm.h>
#ifdef SECURE_BOOT
#include <secure_boot.h>
#endif /* SECURE_BOOT */
#endif /* HSM */

/*==================[macros]==================================================*/

/* FMPLL registers */
#define BOARD_PLL_CR            (*((volatile unsigned int *) 0xC3FE00A0u))
/* bits and shift values for register fields */
#define BOARD_PLL_CR_IDF(n)             ((n) << 26)
#define BOARD_PLL_CR_ODF(n)             ((n) << 24)
#define BOARD_PLL_CR_NDIV(n)            ((n) << 16)
#define BOARD_PLL_CR_SLOCK              0x08u

/* system clock dividers */
#define BOARD_CGM_Z0_DCR        (*((volatile unsigned char *) 0xC3FE00C0))
#define BOARD_CGM_FEC_DCR       (*((volatile unsigned char *) 0xC3FE00E0))
#define BOARD_CGM_FLASH_DCR     (*((volatile unsigned char *) 0xC3FE0120))
#define BOARD_CGM_SC_DC0        (*((volatile unsigned char *) 0xC3FE037C))
#define BOARD_CGM_SC_DC1        (*((volatile unsigned char *) 0xC3FE037D))
#define BOARD_CGM_SC_DC2        (*((volatile unsigned char *) 0xC3FE037E))
#define BOARD_CGM_SC_DCx_EN     0x80u

#define BOARD_CGM_AC0_SC        (*((volatile unsigned int *) 0xC3FE0380u))
#define BOARD_CGM_AC0_SELCTL(n) ((n) << 24)
#define BOARD_CGM_AC0_FIRC      0x01u

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

/*==================[type definitions]========================================*/

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

/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/

static inline void board_ME_init(void);
static inline void board_ME_switch(enum board_ME_targets target);
static inline void board_ME_gate_peripherals(void);
inline void __board_reset(void);

/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/

#ifdef SMP
static volatile unsigned int board_cpus_online;
#endif

#ifdef HSM
unsigned int hsm_version = -1;
#endif

/*==================[internal data]===========================================*/
/*==================[external function definitions]===========================*/

/*------------------[Initialize PLL]------------------------------------------*/

/**
 * \note This does not initialize the STM clock.
 *       The STM clock is initialized in stm_init.
 */
void __init board_init_clocks(void)
{
    /* Configure clock dividers */

    /* Configure S160 clock max 160 MHz */
    S160_CLOCK = ((1 << CLOCK_ENABLE_OFF) | (S160_CLOCK_DIVIDER << CLOCK_DIVIDER_OFF));

    /* Configure S80 clock max 80 MHz */
    S80_CLOCK = ((1 << CLOCK_ENABLE_OFF) | (S80_CLOCK_DIVIDER << CLOCK_DIVIDER_OFF));

    /* Configure S40 clock max 80 MHz */
    S40_CLOCK = ((1 << CLOCK_ENABLE_OFF) | (S40_CLOCK_DIVIDER << CLOCK_DIVIDER_OFF));

    /* Configure FS80 clock equal S80 */
    FS80_CLOCK = ((1 << CLOCK_ENABLE_OFF) | (FS80_CLOCK_DIVIDER << CLOCK_DIVIDER_OFF));

    /* Select XOSC as PLL source */
    MC_CGM_AC5_SC = MC_CGM_AC5_SC_SELCTL_FXOSC;

    /* Configure PLL0 Dividers - 160MHz from 40Mhx XOSC */

    PLLDIG_PLLDV = PLLDIG_PLLDV_INITVAL;
    PLLDIG_PLLCAL3 = PLLDIG_PLLCAL3_INITVAL;
    PLLDIG_PLLFD = PLLDIG_PLLFD_INITVAL;
    /* Ignore all interrupts. */
    PLLDIG_PLLCR = PLLDIG_PLLCR_INITVAL;
}

/*------------------[Initialize board]----------------------------------------*/

/**
 * \brief Board initialization.
 * \details This function is the first one called from assembler start up code.
 *          Entry point into the C world.
 */
void __init board_init(void)
{
    unsigned int cpu_id = arch_cpu_id();
#ifdef HSM
    unsigned int hsm_temp = 0;
#endif
    assert(cpu_id < num_cpus);
   
    if (cpu_id == 0u)
    {
#ifdef SMP
        board_cpus_online = 1;
#endif

#ifdef HSM
        /* 
         * First check if the HSM is available 
         * HSM must be bit #27 (not documented!), see ref. man pg. 1213
         * */
        if (BOARD_ME_CS & 0x10)
        {
            /* Check if the flags are set and the HSM is in the waitloop */
            hsm_temp = HSM2HTS; while(HSM2HTS == 0) { hsm_temp = HSM2HTS; }
            hsm_version = hsm_temp;
            hsm_temp = HSM2HTF; while(HSM2HTF != 1) { hsm_temp = HSM2HTF; }
        }
#endif
        board_config_memory();

        /* assign run modes to peripherals */
        board_ME_gate_peripherals();

        /* Initialize run modes */
        board_ME_init();

        /* Initialize PLL */
        board_init_clocks();

        /* note: at this point the run mode is still DRUN.
         * It will change to RUN0 in board_startup_complete, when the kernel has
         * finished all initialisations and when all CPUs are up and running. */

#ifdef HSM
        /* signal the HSM init is done */
        HT2HSMF = 2;
#endif
    }

    /* fire up cache */
    board_cache_init();

    if (cpu_id == 0u)
    {
        /* Initialize serial interface */
        serial_init(115200);
        printf("Starting up ...\n");

        printf("assuming ROM from %08x to %08x\n", BOARD_ROM_PHYS, BOARD_ROM_PHYS + BOARD_ROM_SIZE);
        printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
        printf("assuming RAM from %08x to %08x\n", BOARD_RAM_PHYS, BOARD_RAM_PHYS + BOARD_RAM_SIZE);
        printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
        printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);
#ifdef HSM
        if (hsm_version != (unsigned int) -1)
        {
            printf("Found HSM, version: 0x%08x\n", hsm_version);
        }
        else
        {
            printf("HSM not enabled!\n");
        }
#ifdef SECURE_BOOT
        secure_boot();
#endif
#endif

        /* Initialize LEDs*/
        leds_init();
    }
    else
    {
        printf("Starting up on CPU %d ...\n", cpu_id);
    }

    /* Initialize Interrupt Controller */
    board_intc_init();

    /* Initialize Timer */
    stm_init(1000);

    /* enter the kernel */
    /* NOTE: all processors take the same entry point! */
    kernel_main(cpu_id);
}

/*-----------------[initialize secondary CPUs]--------------------------------*/

#ifdef SMP

/* Core Status Register */
#define MC_ME_CS     0xFFFB81C0u

/* Core2 Control Register */
#define MC_ME_CCTL2  0xFFFB81C8u
/* Core3 Control Register */
#define MC_ME_CCTL3  0xFFFB81CAu

/* Core2 Boot Address Register */
#define MC_ME_CADDR2 0xFFFB81E8u

/* Core3 Boot Address Register */
#define MC_ME_CADDR3 0xFFFB81ECu

void __cpu1entry(void);
void __cpu2entry(void);

/**
 * \brief Start secondary CPUs.
 * \details A secondary CPU is started using these steps:
 * - Enable at least the DRUN mode for that CPU in the respective
 *   control register (MC_ME_CCTL2, MC_ME_CCTL3).
 * - Write the start address of the CPU in address register
 *   (MC_ME_CADDR2, MC_ME_CADDR3).
 * - The last bit of the address register (MC_ME_CADDRx.RMC) must be set to 1.
 * - Mode change to DRUN.
 *
 * \note At the moment, while testing, only the CPU1 is started.
 */
void __init board_start_secondary_cpus(void)
{
    uint32_t boot_address = 0;

    /* make sure only CPU0 is running */
    assert(board_cpus_online == 1);
    /* make sure we are on CPU0 */
    assert(arch_cpu_id() == 0);

    /* Read core active status from MC_ME_CS */
    /* assert only z4a ~ S_CORE1 ist active, i.e. MC_ME.CS == 2
     * when the HSM ist active, it will turn on the 5th bit so mask out the
     * first bits corresponding to the 3 main CPUs.
     */
    assert((MEMORY_WORD(MC_ME_CS) & 0x0E) == 0x02);

    /* enable all modes for cpu1
     * careful! this is a 16 bit field */
    MEMORY_HALF(MC_ME_CCTL2) = 0xFE;

    /* default boot address is 0x00404100 which is the BAF.
     * We override the boot address */
    boot_address  = (uint32_t)__cpu1entry;
    /* Set the Reset on Mode Change bit - this will actually make cpu2 start
     * when we change mode next time.
     * "This field is automatically cleared when the core reset triggered by the
     * RMC is executed"
     */
    boot_address |= 0x01;
    MEMORY_WORD(MC_ME_CADDR2) = boot_address;
    /* mode transition to DRUN
     * this actually starts the cpu */
    board_ME_switch(BOARD_ME_DRUN);

    /* Wail until the CPU has entered the kernel code. */
    while (!(board_cpus_online & (1u << 1))) ;

    /*----------------------------------------------------------*/
    MEMORY_HALF(MC_ME_CCTL3) = 0xFE;
    boot_address  = (uint32_t)__cpu2entry;
    boot_address |= 0x01;
    MEMORY_WORD(MC_ME_CADDR3) = boot_address;
    board_ME_switch(BOARD_ME_DRUN);

    while (!(board_cpus_online & (1u << 2))) ;
}
#endif

/*-----------------[Send Inter-processor interrupt]---------------------------*/

#ifdef SMP

void board_ipi_broadcast(unsigned long cpu_mask)
{
    unsigned int this_cpu = 0;

    assert(num_cpus > 1);

    this_cpu = arch_cpu_id();

    switch (this_cpu)
    {
        case CPU0:
            if ((cpu_mask & (1u << CPU1)) != 0)
            {
                IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR1) = IRQ_SSCIR_SET;
            }

            if ((cpu_mask & (1u << CPU2)) != 0)
            {
                IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR2) = IRQ_SSCIR_SET;
            }

            break;

        case CPU1:
            if ((cpu_mask & (1u << CPU0)) != 0)
            {
                IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR3) = IRQ_SSCIR_SET;
            }

            if ((cpu_mask & (1u << CPU2)) != 0)
            {
                IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR4) = IRQ_SSCIR_SET;
            }

            break;

        case CPU2:
            if ((cpu_mask & (1u << CPU0)) != 0)
            {
                IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR5) = IRQ_SSCIR_SET;
            }

            if ((cpu_mask & (1u << CPU1)) != 0)
            {
                IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR6) = IRQ_SSCIR_SET;
            }

            break;
    }
}
#endif

/*------------------[CPU0 up confirmation]------------------------------------*/

void __init board_cpu0_up(void)
{
}

/*-----------------[end of CPU1 and CPU2 initialization]----------------------*/

#ifdef SMP
void __init board_secondary_cpu_up(unsigned int cpu)
{
    assert(cpu == arch_cpu_id());
    board_cpus_online |= (1u << cpu);

    /* serve core watchdogs ... */
}
#endif

/*------------------[kernel initialization complete]--------------------------*/

void __init board_startup_complete(void)
{
    /* this practically turns on PLL because RUN0 is setup to use it as clock
     * source (SYSCLK) */
    board_ME_switch(BOARD_ME_RUN0);
    /* Start all STM timers.
     * If SMP is not enabled, this will only start STM_0. */
    stm_release_all();

#if (defined SMP)
    /* enable the RAM SMPU */
    SMPU_1_CES0 = SMPU_CES0_ENABLE;
    /* enable the flash and peripherals SMPU */
    SMPU_0_CES0 = SMPU_CES0_ENABLE;
#endif
}

/*------------------[board halt]----------------------------------------------*/

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

    while (1)
    {
        /* wait and stay in idle mode, with interrupts disabled */
    }
}

__cold void board_halt(haltmode_t mode)
{
    if ((mode == BOARD_RESET) || (mode == BOARD_HM_RESET))
    {
        __board_reset();
    }

    /* just halt the machine */
    __board_halt();
}

/*------------------[board idle]----------------------------------------------*/

void board_idle(void)
{
    while (1)
    {
        /* wait and stay in idle mode */
    }
}

/*------------------[handle NMI]----------------------------------------------*/

void board_nmi_dispatch(unsigned int vector __unused)
{
    hm_system_error(HM_ERROR_NMI, vector);
}


/*------------------[HM Exception]--------------------------------------------*/

int board_hm_exception
(
    struct arch_reg_frame *regs __unused,
    int fatal __unused,
    unsigned int hm_error_id __unused,
    unsigned long vector __unused,
    unsigned long fault_addr __unused,
    unsigned long aux __unused
)
{
    return 0;       /* exception not handled */
}


/*------------------[Partition switch]----------------------------------------*/

void board_tp_switch
(
    unsigned int prev_timepart  __unused,
    unsigned int next_timepart  __unused,
    unsigned int tpwindow_flags __unused
)
{
}


/*==================[internal function definitions]===========================*/

/*------------------[Initialize MC_ME]----------------------------------------*/

/* Initialize the Mode Entry module.
 * - Enable wanted modes.
 * - Configure run modes.
 */
static inline void board_ME_init(void)
{
    /* enable all MC_ME modes */
    BOARD_ME_ME = BOARD_ME_ME_ALL_MODES;

    /* Configure RUN0 mode to switch to PLL */
    BOARD_ME_RUN0_MC =
    (  BOARD_ME_RUN0_MC_MVRON
     | BOARD_ME_RUN0_MC_FLAON
     | BOARD_ME_RUN0_MC_PLLON
     | BOARD_ME_RUN0_MC_FXOSCON
     | BOARD_ME_RUN0_MC_FIRCON
     | BOARD_ME_RUN0_MC_SYSCLK_PLL
    );
}

/*------------------[Change run mode]-----------------------------------------*/

static inline void board_ME_switch(enum board_ME_targets target)
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

/*------------------[Configure peripheral behaviour]--------------------------*/

/* Configure peripheral behavior during run modes. */
static inline void board_ME_gate_peripherals(void)
{
    /* Define behaviours*/

    /* frozen in all run modes */
    BOARD_ME_RUN_PC0 = 0x00000000;
    /* active in all run modes */
    BOARD_ME_RUN_PC1 = 0x00FC;

    /* Route peripherals to behaviour registers */

    /* SIUL: select peri. cfg. RUN_PC[1] */
    BOARD_ME_PCTL_SIUL = 1u;
   
    /*
     * Move this code somewehere else
     */
#define SIUL2_MSCR ((uint32_t*)(0xFFFC0240))
#define SIUL2_IMCR ((uint32_t*)(0xFFFC0A40))

    SIUL2_MSCR[16] = 0x32000001;
    SIUL2_MSCR[17] = 0x00080000;
    SIUL2_IMCR[188] = 0x02;

#define MC_ME_PCTL_FlexCAN_0  0xFFFB8106
#define MC_ME_PCTL_FlexCAN_1  0xFFFB8107

    MEMORY_BYTE(MC_ME_PCTL_FlexCAN_0) = 0x01;
    MEMORY_BYTE(MC_ME_PCTL_FlexCAN_1) = 0x01;
}

/*------------------[Board reset]---------------------------------------------*/

/* reset the board */
void __board_reset(void)
{
    /* FIXME not clear: functional or destructive reset? */
    board_ME_switch(BOARD_ME_FRESET); /* try functional reset for now */

    __board_halt();
}

/*==================[end of file]=============================================*/
