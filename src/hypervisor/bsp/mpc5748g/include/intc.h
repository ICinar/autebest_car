/**
 * \file     intc.h
 * \brief    Interface of the INTC driver for Calypso MPC5748G.
 * \details
 *
 * \date      05.10.2015
 * \author    Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author    easycore GmbH, 91058 Erlangen, Germany
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

#if (!defined INTC_H)
#define INTC_H

/*==================[inclusions]==============================================*/
/*==================[macros]==================================================*/

/*------------------[Interrupt source numbers]--------------------------------*/

/* The numbers where taken from "MPC5748G Reference Manual", chapter
 * "INTC interrupt sources".
 * When an interrupt occurs, the register field INTC_IACKRx.INTVEC holds the
 * source number.
 */


/*------------------[STM]-----------------------------------------------------*/

/**
 * \brief   Interrupt number of the STM0 peripheral, channel 0.
 * \details To be used when enabling the STM0 interrupt in the Interrupt
 *          Controller INTC.
 */
#define INTC_SOURCE_STM_0_CIR0 (36u)

/**
 * \brief   Interrupt number of the STM1 peripheral, channel 0.
 * \details To be used when enabling the STM1 interrupt in the Interrupt
 *          Controller INTC.
 */
#define INTC_SOURCE_STM_1_CIR0 (40u)

/**
 * \brief   Interrupt number of the STM2 peripheral, channel 0.
 * \details To be used when enabling the STM2 interrupt in the Interrupt
 *          Controller INTC.
 */
#define INTC_SOURCE_STM_2_CIR0 (44u)

/*------------------[Software setable flags]----------------------------------*/

/* Sources of software interrupts. These are used to implement the IPI
 * (Inter-processor interrupt) mechanism. */

#define INTC_SOURCE_SSCIR1     (1u)
#define INTC_SOURCE_SSCIR2     (2u)
#define INTC_SOURCE_SSCIR3     (3u)
#define INTC_SOURCE_SSCIR4     (4u)
#define INTC_SOURCE_SSCIR5     (5u)
#define INTC_SOURCE_SSCIR6     (6u)

#define IRQ_SSCIR_SET           0x02u
#define IRQ_SSCIR_CLR           0x01u

/* INTC Software Set/Clear Interrupt Register INTC_SSCIR0 - INTC_SSCIR23 */
#define IRQ_INTC_SSCIR(x)      (*((volatile unsigned char *) (0xFC040040u + (x))))


/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/




#endif /* if (!defined INTC_H) */
/*==================[end of file]=============================================*/

