/**
 * \file      clocks.h
 * \brief     Interface declaration of the clocks driver for MPC5748G.
 * \details   For more details regarding the SMPU unit see document
 *            "MPC5748G Reference Manual", Rev 4, 07/2015,
 *            "Chapter 9, Clocking Overview", page 195 ff.
 *            "Chapter 31 Clock Generation Module (MC_CGM)", page 699 ff
 *            "Chapter 38 Mode Entry Module (MC_ME)", page 1079 ff
 *            "Chapter 9.9.1.5 PLLDIG initialization information", page 222 ff
 *
 * \date      31.08.2015
 * \author    Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author    easycore GmbH, 91058 Erlangen, Germany
 * \version
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

#if (!defined CLOCKS_H)
#define CLOCKS_H

/*==================[inclusions]==============================================*/

#include <stdint.h> /* uint32_t */

/*==================[Register addresses and aliases]==========================*/

/*------------------[Clock Generation Module (MC_CGM)]------------------------*/

/**
 * \brief    Auxiliary Clock 5 Select Control Register.
 * \details This register is used to select the current clock source for the PLL
 *          Aux clock.
 */
#define MC_CGM_AC5_SC        (*(volatile uint32_t*) 0xFFFB08A0)

/**
 * \brief   Auxiliary Clock 5 Source Selection Control.
 * \details Selects the source for auxiliary clock 5.
 *         
 *         - 0 Fast Internal crystal osc. (FIRC)
 *         - 1 Fast external crystal osc. (FXOSC)
 */
#define MC_CGM_AC5_SC_SELCTL_OFF   (24u)
#define MC_CGM_AC5_SC_SELCTL_FXOSC (1u << MC_CGM_AC5_SC_SELCTL_OFF)


/**
 * \brief   System Clock Divider 0 Configuration Register.
 * \details This register controls system clock divider 0 - S160 clock
 */
#define MC_CGM_SC_DC0        (*(volatile uint32_t*) 0xFFFB07E8u)
#define S160_CLOCK           MC_CGM_SC_DC0
#define S160_CLOCK_DIVIDER   (0x00u)

/**
 * \brief   System Clock Divider 1 Configuration Register.
 * \details DC1 register controls  modules working on system clock divider 1 -
 *          S80 clock.
 */
#define MC_CGM_SC_DC1        (*(volatile uint32_t*) 0xFFFB07ECu)
#define S80_CLOCK             MC_CGM_SC_DC1
#define S80_CLOCK_DIVIDER    (0x01u)

/**
 * \brief   System Clock Divider 2 Configuration Register.
 * \details This register controls system clock divider 2 - S40 clock.
 */
#define MC_CGM_SC_DC2        (*(volatile uint32_t*) 0xFFFB07F0u)
#define S40_CLOCK            MC_CGM_SC_DC2
#define S40_CLOCK_DIVIDER    (0x03u)

/**
 * \brief   System Clock Divider 5 Configuration Register.
 * \details This register controls system clock divider 5 -
 *          FS80 peripheral clock.
 */
#define MC_CGM_SC_DC5        (*(volatile uint32_t*) 0xFFFB07FCu)
#define FS80_CLOCK           MC_CGM_SC_DC5
#define FS80_CLOCK_DIVIDER   S80_CLOCK_DIVIDER

/**
 * Offset of the DE bit field in any MC_CGM_SC_DC register.
 */
#define CLOCK_ENABLE_OFF     (31u)

/**
 * Offset of the DIV bit field in any MC_CGM_SC_DC register.
 */
#define CLOCK_DIVIDER_OFF    (16u)

/*------------------[PLL Digital Interface (PLLDIG)]--------------------------*/

/**
 * \brief   PLL Calibration Register 3.
 * \details PLLCAL3 contains the denominator of the fractional loop division
 *          factor.
 */
#define PLLDIG_PLLCAL3          (*(volatile uint32_t*) 0xFFFB0098u)
#define PLLDIG_PLLCAL3_INITVAL  (0x09C3C000)
//(0x00004062u)


/**
 * \brief PLLDIG PLL Control Register.
 */
#define PLLDIG_PLLCR         (*(volatile uint32_t*) 0xFFFB00A0u)

/**
 * \brief Initialization value of PLLDIG_PLLCR: ignore all interrupts.
 */
#define PLLDIG_PLLCR_INITVAL (0x00u)


/**
 * \brief PLLDIG PLL Divider Register.
 */
#define PLLDIG_PLLDV         (*(volatile uint32_t*) 0xFFFB00A8u)

/**
 * \brief   PHI1 reduced frequency divider.
 * \details This field determines the VCO clock post divider for driving the
 *          PHI1 output clock.
 */
#define PLLDIG_PLLDV_RFDPHI1     (2u)
#define PLLDIG_PLLDV_RFDPHI1_OFF (25u)

/**
 * \brief   PHI reduced frequency divider.
 * \details This 6-bit field determines the VCO clock post divider for driving
 *          the PHI output clock.
 */
#define PLLDIG_PLLDV_RFDPHI      (1u)
#define PLLDIG_PLLDV_RFDPHI_OFF  (16u)

/**
 * \brief   Input clock predivider.
 * \details This field controls the value of the divider on the input clock. The
 *          output of the predivider circuit generates the reference clock to
 *          the PLL analog loop.
 */
#define PLLDIG_PLLDV_PREDIV      (1u)
#define PLLDIG_PLLDV_PREDIV_OFF  (12u)

/**
 * \brief   Loop multiplication factor divider.
 * \details This field controls the value of the divider in the feedback loop.
 *          The value specified by the MFD bits establishes the multiplication
 *          factor applied to the reference frequency. Divider value = MFD,
 *          where MFD has the range 10...150 (Ah...96h). All other values are
 *          reserved.
 */
#define PLLDIG_PLLDV_MFD         (40u)
#define PLLDIG_PLLDV_MFD_OFF     (0u)

/**
 * \brief Initialization value of the PLLDIG_PLLDV register.
 */
#define PLLDIG_PLLDV_INITVAL  (\
         (PLLDIG_PLLDV_RFDPHI1 << PLLDIG_PLLDV_RFDPHI1_OFF)\
        |(PLLDIG_PLLDV_RFDPHI  << PLLDIG_PLLDV_RFDPHI_OFF)\
        |(PLLDIG_PLLDV_PREDIV  << PLLDIG_PLLDV_PREDIV_OFF)\
        |(PLLDIG_PLLDV_MFD     << PLLDIG_PLLDV_MFD_OFF)\
        )

/**
 * \brief PLLDIG PLL Fractional Divide Register.
 */
#define PLLDIG_PLLFD         (*(volatile uint32_t*) 0xFFFB00B0u)

/**
 * \brief Sigma Delta Modulation Enable.
 */
#define  PLLDIG_PLLFD_SMDEN      (1u)
#define  PLLDIG_PLLFD_SMDEN_OFF  (30u)
#define  PLLDIG_PLLFD_INITVAL    (PLLDIG_PLLFD_SMDEN << PLLDIG_PLLFD_SMDEN_OFF)

/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/




#endif /* if (!defined CLOCKS_H) */
/*==================[end of file]=============================================*/

