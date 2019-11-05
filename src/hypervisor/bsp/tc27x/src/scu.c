/**
 * \file   scu.c
 * \brief  System Control Unit (SCU) driver.
 * \see    scu.h
 * \date   25.02.2015
 * \author easycore GmbH, 91058 Erlangen, Germany
 */

/*==================[inclusions]==============================================*/

#include <kernel.h> /* arch_cpu_id */
#include "scu.h"
#include "scu_cfg.h"
#include "wdt.h"

/*==================[macros]==================================================*/

#define unlock_safety_watchdog_old()  wdt_set_endinit(WDT_BASE + WDT_S_OFFSET, 0)
#define lock_safety_watchdog_old()    wdt_set_endinit(WDT_BASE + WDT_S_OFFSET, 1)

/*==================[internal function declarations]==========================*/
/*==================[external function definitions]===========================*/

/*------------------[initialize PLL system]-----------------------------------*/

void scu_init_pll(void)
{

    SCU_OSCCON = SCU_OSCCON_INITVAL;

    while ((SCU_CCUCON1 & SCU_CCUCON1_LCK_MASK) != 0u)
        ;

    SCU_CCUCON1 = SCU_CCUCON1_INITVAL | SCU_CCUCON1_UP_MASK;

    while ((SCU_CCUCON1 & SCU_CCUCON1_LCK_MASK) != 0u)
        ;

    SCU_CCUCON1 = SCU_CCUCON1_INITVAL;

    while ((SCU_CCUCON2 & SCU_CCUCON2_LCK_MASK) != 0)
        ;

    SCU_CCUCON2 = SCU_CCUCON2_INITVAL | SCU_CCUCON2_UP_MASK;


    while ((SCU_CCUCON2 & SCU_CCUCON2_LCK_MASK) != 0)
        ;

    SCU_CCUCON2 = SCU_CCUCON2_INITVAL;

    SCU_PLLCON0 |= (SCU_PLLCON0_VCOBYP_MASK | SCU_PLLCON0_SETFINDIS_MASK);

    /* set K1,K2 divider */
    SCU_PLLCON1 = SCU_PLLCON1_INITVAL;

    /* set P,N divider */
    SCU_PLLCON0 = SCU_PLLCON0_INITVAL
                      | (SCU_PLLCON0_VCOBYP_MASK | SCU_PLLCON0_CLRFINDIS_MASK);

    while ((SCU_CCUCON0 & SCU_CCUCON0_LCK_MASK) != 0)
        ;

    SCU_CCUCON0 = SCU_CCUCON0_INITVAL | SCU_CCUCON0_UP_MASK;

    while ((SCU_CCUCON0 & SCU_CCUCON0_LCK_MASK) != 0)
        ;

    /* if you want to slow down CPU0, setup CCUCON6.CPU0DIV */

    SCU_CCUCON0 = SCU_CCUCON0_INITVAL;


    /* no prescaler mode requested */
    if ((SCU_PLLCON0_INITVAL & SCU_PLLCON0_VCOBYP_MASK) == 0)
    {
        /* wait for PLL locked */
        while ((SCU_PLLSTAT & SCU_PLLSTAT_VCOLOCK_MASK) == 0)
            ;

        /* enter the normal mode by disabling the VCO bypass
         * (set the PLLCON0.VCOBYP bit to 0) */
        SCU_PLLCON0 &= (~ SCU_PLLCON0_VCOBYP_MASK);
    }
}

/*------------------[read internal clock]-------------------------------------*/

unsigned long scu_get_internal_clock(void)
{
    unsigned long clock = BACKUP_TIMER_FREQ;
    unsigned int clksel = SCU_CCUCON0_CLKSEL_READ();
    switch (clksel)
    {
        case SCU_CCUCON0_CLK_BACKUP:
            clock = BACKUP_TIMER_FREQ;
            break;
        case SCU_CCUCON0_CLK_FPLL:
            clock = scu_get_pll_clock();
            break;
    }

    return clock;
}

/*------------------[read CPU clock]------------------------------------------*/

unsigned long scu_get_cpu_clock(void)
{
    uint32_t clock     = 0;
    uint32_t cpu_id    = 0;
    uint32_t cpu_div   = 0;
    uint32_t sridiv    = 0;
    uint32_t fSRI      = 0;
    uint32_t cpu_clock = 0;

    clock  = scu_get_internal_clock();
    cpu_id = arch_cpu_id();

    switch (cpu_id)
    {
        case 0:
            cpu_div = SCU_CCUCON6_CPU0DIV_READ();
            break;
        case 1:
            cpu_div = SCU_CCUCON7_CPU1DIV_READ();
            break;
        case 2:
            cpu_div = SCU_CCUCON8_CPU2DIV_READ();
            break;
        default:
            cpu_div = SCU_CCUCON6_CPU0DIV_READ();
            break;
    }

    sridiv = SCU_CCUCON0_SRIDIV_READ();

    if (sridiv != 0)
    {
        fSRI = clock / sridiv;
        cpu_clock = fSRI * ((64u - cpu_div) / 64u);
    }
    return cpu_clock;
}

/*------------------[read STM clock]------------------------------------------*/

unsigned long scu_get_stm_clock(void)
{
    unsigned long frequency = scu_get_internal_clock();
    unsigned long stmdiv = SCU_CCUCON1_STMDIV_READ();
    unsigned long stm_clock = 0;

    if (stmdiv != 0)
    {
        stm_clock = (frequency / stmdiv);
    }

    return stm_clock;
}

/*------------------[read PLL clock]------------------------------------------*/

unsigned long scu_get_pll_clock(void)
{
    uint32_t frequency = EXTCLK;
    uint32_t vcobyst = 0u;
    uint32_t findis  = 0u;
    uint32_t ndiv    = 0u;
    uint32_t pdiv    = 0u;
    uint32_t k1div   = 0u;
    uint32_t k2div   = 0u;

    vcobyst = SCU_PLLSTAT_VCOBYST_READ();
    findis  = SCU_PLLSTAT_FINDIS_READ();

    if (vcobyst == 0)
    {
        /* Freerunning / Normal Mode is entered
         * Read NDIV, PDIV and K2DIV */

        ndiv    = SCU_PLLCON0_NDIV_READ();
        pdiv    = SCU_PLLCON0_PDIV_READ();
        k2div   = SCU_PLLCON1_K2DIV_READ();

        if (findis == 0)
        {
            /* The input clock from the oscillator is connected
             * to the VCO part -> normal mode */

            frequency *= ndiv;
            frequency /= pdiv;
            frequency /= k2div;
        }
        else
        {
            /* freerunning mode */
            frequency = VCOBASE_FREQ;
            frequency /= k2div;
        }
    }
    else
    {
        /* Prescaler Mode is entered
         * Read K1DIV */
        k1div   = SCU_PLLCON1_K1DIV_READ();
        frequency /= k1div; /* fOSC/K1 */
    }

    return (unsigned long)frequency;
}

/*==================[internal function definitions]===========================*/
/*==================[end of file]=============================================*/

