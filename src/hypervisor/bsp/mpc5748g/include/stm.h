/**
 * \file      stm.h
 * \brief     Interface declaration of the STM driver for MPC5748G.
 * \details   This files declares the interface functions and macros of the
 *            System Timer Module (STM) peripheral driver for MPC5748G
 *            (Calypso).
 *
 *            Calypso has three instances of the STM unit, one for each CPU.
 *            This driver implements handling of all STM units.
 *            Each STM peripheral has 4 channels. We use only the first one.
 *
 *            The reason we use the STM peripheral is that the MPC5748G does not
 *            have the PPC decrement timer specified in PowerPC Book-E.
 *
 *            For more details regarding the SMPU unit see document
 *            "MPC5748G Reference Manual", Rev 4, 07/2015,
 *            "Chapter 54 System Timer Module (STM)".
 *
 * \date      27.08.2015
 * \author    Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author    easycore GmbH, 91058 Erlangen, Germany
 * \version
 *
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

#if (!defined STM_H)
#define STM_H

/*==================[inclusions]==============================================*/

#include <stdint.h> /* uint32_t */

/*==================[Register addresses and special values]===================*/

/*------------------[Base address of the STM]---------------------------------*/

/**
 * \brief STM_0 base address.
 */
#define STM_BASE             (0xFC068000u)

/**
 * \brief   Memory distance between two STM instances.
 * \details Used to compute the base address of a specific STM instance.
 */
#define STM_CORE_OFFSET      (0x00004000u)

/*------------------[Global Control register]---------------------------------*/

/**
 * \brief   Offset of STM Control Register.
 * \details The STM Control Register includes the prescale value,
 *          freeze control and timer enable bits.
 */
#define STM_CR               (0x00u)

/* Timer counter Enabled */
#define STM_CR_TEN           (0x01u << 0u)
/* Freeze.
 * Allows the timer counter to be stopped when the device enters debug mode. */
#define STM_CR_FRZ           (0x01u << 1u)


/*------------------[Count Register]------------------------------------------*/

/**
 * \brief   Offset of STM Count Register.
 * \details The STM Count Register holds the timer count value.
 */
#define STM_CNT              (0x04u)


/*------------------[Channel 0 Control register]------------------------------*/

/**
 * \brief   Offset of STM Channel Control Register.
 * \details The STM Channel Control Register has the enable bit for channel 0
 *          of the timer.
 */
#define STM_CCR              (0x10u)

/* Enable the STM. */
#define STM_CCR_ENABLE       (0x01u)
/* Disable the STM. */
#define STM_CCR_DISABLE      (0x00u)

/*------------------[Interrupt register]--------------------------------------*/
/**
 * \brief   Offset of STM Channel Interrupt Register.
 * \details The STM Channel Interrupt Register has the interrupt flag for
 *          channel 0 of the timer. Write 1 to clear.
 */
#define STM_CIR              (0x14u)

/* Clear the interrupt bit. */
#define STM_CIR_CLEAR        (0x01u)

/*------------------[Compare register]----------------------------------------*/
/**
 * \brief   Offset of STM Channel Compare Register.
 * \details The STM channel compare register holds the compare value
 *          for channel 0.
 */
#define STM_CMP              (0x18u)

/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/


/*------------------[Initialize the STM timer]--------------------------------*/
/**
 * \brief   Initialize the STM timer.
 *
 * \details This function initializes the module internl data and the reload
 *          register of the STM module. It does not start the timer yet. In
 *          order to actualy start the timer, call stm_start.
 *
 * \pre     Timer module STM must not be active when calling this function.
 * \post    The timer does not count yet, only the internal data has been
 *          initialized.
 */
void stm_init(unsigned int freq);

/*------------------[Start the STM timer]-------------------------------------*/
/**
 * \brief   Start the STM timer.
 *
 * \details This function make the STM timers start counting.
 * \post    The STM timers start incrementing the count register and trigger an
 *          interrupt every time the counter register equals the compare
 *          register.
 */
void stm_release_all(void);

/*------------------[Interrupt handler]---------------------------------------*/
/**
 * \brief Interrupt handler.
 *
 * \details
 * This function is called by the INTC dispacher like this:
 *
 * - The STM counter register reaches the value in the STM compare register
 * - STM triggers an interrupt
 * - The INTC interrupt controller stores the number 36 in its interrupt source
 *   register and triggers the exception with offset 0x40
 *   (External Input Interrupt)
 * - The CPU jumps to the address SPR_IVPR + 0x40 where the corresponding
 *   exception handler begins
 * - The exception handler does some magic which makes the kernel function
 *   ppc_handler_irq be called with vector number 4 (for ivor 4)
 * - the kernel irg handler delegates the further logic and the vector number
 *   to our function board_irq_dispatch.
 * - board_irq_dispatch gets the argument 4 (external interrupt) and calls the
 *   INTC dispacher irq_INTC_dispatch
 * - The INTC dispacher reads the number 36 from the INTC module and calls our
 *   STM handler.
 */
void stm_handler(void);


/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/




#endif /* if (!defined STM_H) */
/*==================[end of file]=============================================*/

