/**
 * \file   scu.h
 * \brief  Interface of the System Control Unit (SCU) driver.
 *
 * \details The SCU driver initializes the PLL system and provides interfaces
 * for reading various clocks.
 *
 * \date   25.02.2015
 * \author easycore GmbH, 91058 Erlangen, Germany
 */

#ifndef SCU_H
#define SCU_H

/*==================[inclusions]==============================================*/
/*==================[macros]==================================================*/

/** External oscillator clock (20MHz). */
#define EXTCLK             (20000000)

/** Frequency of the internal backup timer is 100 MHz. */
#define BACKUP_TIMER_FREQ  (100000000ul)

/* fVCOBASE 800 MHz */
#define VCOBASE_FREQ       (800000000ul)  


/*==================[external function declarations]==========================*/

/**
 * \brief Initialize PLL system.
 */
void scu_init_pll(void);

/** 
 * \brief   Read internal frequency.
 * \details Depending on setup, this is either the internal PLL frequency
 *          or frequency of the backup clock.
 * \note This is not necessary the CPU frequency.
 */
unsigned long scu_get_internal_clock(void);

/**
 * \brief   Read the frequency of the current CPU.
 * \details The CPU clock is computed using the formula:
 * 
 *  fCPUn = fSRI * (64 - CPUnDIV / 64)
 */
unsigned long scu_get_cpu_clock(void);

/**
 * \brief Read the frequency of the STM clock.
 */
unsigned long scu_get_stm_clock(void);

/**
 * \brief Read the PLL clock.
 * Use this function only when the PLL system has been setup.
 */
unsigned long scu_get_pll_clock(void);

#endif
/*==================[end of file]=============================================*/

