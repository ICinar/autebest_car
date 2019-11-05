/**
 * \file   scu_cfg.h
 * \brief  Initialisation values of the System Control Unit (SCU) registers.
 *
 * \details This file also provides macros for reading the various SCU
 * registers. The definitions and macros are sorted and grouped by register.
 *
 * \date   25.02.2015
 * \author easycore GmbH, 91058 Erlangen, Germany
 */
#ifndef SCU_CFG_H
#define SCU_CFG_H

#include <stdint.h> /* uint32_t */

/*-----------------[OSCCON]---------------------------------------------------*/

/** Address of the OSCCON register. */
#define SCU_OSCCON                 (*(volatile uint32_t*)0xF0036010u)

/* Bitfield offsets inside the OSCCON register. */

#define SCU_OSCCON_OSCRES_OFF      (2)
#define SCU_OSCCON_GAINSEL_OFF     (3)
#define SCU_OSCCON_MODE_OFF        (5)
#define SCU_OSCCON_OSCVAL_OFF      (16)

/* Initial register values */

#define SCU_OSCCON_OSCRES_INITVAL  (0x1u)
#define SCU_OSCCON_GAINSEL_INITVAL (0x3u)
#define SCU_OSCCON_MODE_INITVAL    (0x0u)
#define SCU_OSCCON_OSCVAL_INITVAL  (0x7u)

#define SCU_OSCCON_INITVAL  \
    ( (SCU_OSCCON_OSCRES_INITVAL  << SCU_OSCCON_OSCRES_OFF) \
    | (SCU_OSCCON_GAINSEL_INITVAL << SCU_OSCCON_GAINSEL_OFF) \
    | (SCU_OSCCON_MODE_INITVAL    << SCU_OSCCON_MODE_OFF)\
    | (SCU_OSCCON_OSCVAL_INITVAL  << SCU_OSCCON_OSCVAL_OFF))


/*-----------------[PLLSTAT]--------------------------------------------------*/

/** Address of the PLLSTAT register. */
#define SCU_PLLSTAT              (*(volatile uint32_t*)0xF0036014u)

#define SCU_PLLSTAT_VCOBYST_BITS (0x1)
#define SCU_PLLSTAT_VCOBYST_OFF  (0)
#define SCU_PLLSTAT_VCOBYST_MASK (SCU_PLLSTAT_VCOBYST_BITS << SCU_PLLSTAT_VCOBYST_OFF)
#define SCU_PLLSTAT_VCOBYST_READ() ((SCU_PLLSTAT & SCU_PLLSTAT_VCOBYST_MASK) >> SCU_PLLSTAT_VCOBYST_OFF)

#define SCU_PLLSTAT_VCOLOCK_BITS (0x1)
#define SCU_PLLSTAT_VCOLOCK_OFF  (2)
#define SCU_PLLSTAT_VCOLOCK_MASK (SCU_PLLSTAT_VCOLOCK_BITS << SCU_PLLSTAT_VCOLOCK_OFF)

#define SCU_PLLSTAT_FINDIS_BITS  (0x1)
#define SCU_PLLSTAT_FINDIS_OFF   (3)
#define SCU_PLLSTAT_FINDIS_MASK  (SCU_PLLSTAT_FINDIS_BITS << SCU_PLLSTAT_FINDIS_OFF)
#define SCU_PLLSTAT_FINDIS_READ()  ((SCU_PLLSTAT & SCU_PLLSTAT_FINDIS_MASK) >> SCU_PLLSTAT_FINDIS_OFF)



/*-----------------[CCUCON0]--------------------------------------------------*/

/** Address of register CCUCON0 */
#define SCU_CCUCON0               (*(volatile uint32_t*)0xF0036030u)

/* Offsets and masks inside the CCUCON0 register */

#define SCU_CCUCON0_BAUD1DIV_OFF  (0)
#define SCU_CCUCON0_BAUD2DIV_OFF  (4)

#define SCU_CCUCON0_SRIDIV_OFF    (8)
#define SCU_CCUCON0_SRIDIV_BITS   (0xf)
#define SCU_CCUCON0_SRIDIV_MASK   (SCU_CCUCON0_SRIDIV_BITS << SCU_CCUCON0_SRIDIV_OFF)
#define SCU_CCUCON0_SRIDIV_READ() ((SCU_CCUCON0 & SCU_CCUCON0_SRIDIV_MASK) >> SCU_CCUCON0_SRIDIV_OFF)


#define SCU_CCUCON0_LPDIV_OFF     (12)
#define SCU_CCUCON0_SPBDIV_OFF    (16)
#define SCU_CCUCON0_FSI2DIV_OFF   (20)
#define SCU_CCUCON0_FSIDIV_OFF    (24)
#define SCU_CCUCON0_ADCCLKSEL_OFF (26)

#define SCU_CCUCON0_CLKSEL_OFF    (28)
#define SCU_CCUCON0_CLKSEL_BITS   (0x3)
#define SCU_CCUCON0_CLKSEL_MASK   (SCU_CCUCON0_CLKSEL_BITS << SCU_CCUCON0_CLKSEL_OFF)
#define SCU_CCUCON0_CLK_BACKUP    (0)
#define SCU_CCUCON0_CLK_FPLL      (1)
#define SCU_CCUCON0_CLKSEL_READ() ((SCU_CCUCON0 & SCU_CCUCON0_CLKSEL_MASK) >> SCU_CCUCON0_CLKSEL_OFF)


#define SCU_CCUCON0_UP_OFF        (30)
#define SCU_CCUCON0_UP_BITS       (0x1)
#define SCU_CCUCON0_UP_MASK       (SCU_CCUCON0_UP_BITS << SCU_CCUCON0_UP_OFF)

#define SCU_CCUCON0_LCK_OFF       (31)
#define SCU_CCUCON0_LCK_BITS      (0x1)
#define SCU_CCUCON0_LCK_MASK      (SCU_CCUCON0_LCK_BITS << SCU_CCUCON0_LCK_OFF)

/* Initial values of the CCUCON0 fields. */

#define SCU_CCUCON0_BAUD1DIV_INITVAL  (0x2u)
#define SCU_CCUCON0_BAUD2DIV_INITVAL  (0x1u)
#define SCU_CCUCON0_SRIDIV_INITVAL    (0x1u)
#define SCU_CCUCON0_LPDIV_INITVAL     (0x0u)
#define SCU_CCUCON0_SPBDIV_INITVAL    (0x2u)
#define SCU_CCUCON0_FSI2DIV_INITVAL   (0x1u)
#define SCU_CCUCON0_FSIDIV_INITVAL    (0x2u)
#define SCU_CCUCON0_ADCCLKSEL_INITVAL (0x0u)
#define SCU_CCUCON0_CLKSEL_INITVAL    (0x1u)
#define SCU_CCUCON0_UP_INITVAL        (0x1u)

#define SCU_CCUCON0_INITVAL \
    ( (SCU_CCUCON0_BAUD1DIV_INITVAL  << SCU_CCUCON0_BAUD1DIV_OFF)\
    | (SCU_CCUCON0_BAUD2DIV_INITVAL  << SCU_CCUCON0_BAUD2DIV_OFF)\
    | (SCU_CCUCON0_SRIDIV_INITVAL    << SCU_CCUCON0_SRIDIV_OFF)\
    | (SCU_CCUCON0_LPDIV_INITVAL     << SCU_CCUCON0_LPDIV_OFF)\
    | (SCU_CCUCON0_SPBDIV_INITVAL    << SCU_CCUCON0_SPBDIV_OFF)\
    | (SCU_CCUCON0_FSI2DIV_INITVAL   << SCU_CCUCON0_FSI2DIV_OFF)\
    | (SCU_CCUCON0_FSIDIV_INITVAL    << SCU_CCUCON0_FSIDIV_OFF)\
    | (SCU_CCUCON0_ADCCLKSEL_INITVAL << SCU_CCUCON0_ADCCLKSEL_OFF)\
    | (SCU_CCUCON0_CLKSEL_INITVAL    << SCU_CCUCON0_CLKSEL_OFF)\
    | (SCU_CCUCON0_UP_INITVAL        << SCU_CCUCON0_UP_OFF))


/*-----------------[CCUCON1]--------------------------------------------------*/

/** Address of the CCUCON1 register. */
#define SCU_CCUCON1                (*(volatile uint32_t*)0xF0036034u)

/* Offsets and masks inside the CCUCON1 register */

#define SCU_CCUCON1_CANDIV_OFF     (0)
#define SCU_CCUCON1_ERAYDIV_OFF    (4)

#define SCU_CCUCON1_STMDIV_OFF     (8)
#define SCU_CCUCON1_STMDIV_BITS    (0xf)
#define SCU_CCUCON1_STMDIV_MASK    (SCU_CCUCON1_STMDIV_BITS << SCU_CCUCON1_STMDIV_OFF)
#define SCU_CCUCON1_STMDIV_READ()  ((SCU_CCUCON1 & SCU_CCUCON1_STMDIV_MASK) >> SCU_CCUCON1_STMDIV_OFF)

#define SCU_CCUCON1_GTMDIV_OFF     (12)
#define SCU_CCUCON1_ETHDIV_OFF     (16)
#define SCU_CCUCON1_ASCLINFDIV_OFF (20)
#define SCU_CCUCON1_ASCLINSDIV_OFF (24)
#define SCU_CCUCON1_INSEL_OFF      (28)

#define SCU_CCUCON1_UP_OFF         (30)
#define SCU_CCUCON1_UP_BITS        (0x1)
#define SCU_CCUCON1_UP_MASK        (SCU_CCUCON1_UP_BITS << SCU_CCUCON1_UP_OFF)

#define SCU_CCUCON1_LCK_OFF        (31)
#define SCU_CCUCON1_LCK_BITS       (0x1)
#define SCU_CCUCON1_LCK_MASK       (SCU_CCUCON1_LCK_BITS << SCU_CCUCON1_LCK_OFF)

/* Initial register values */

#define SCU_CCUCON1_CANDIV_INITVAL     (0x1u)
#define SCU_CCUCON1_ERAYDIV_INITVAL    (0x1u)
#define SCU_CCUCON1_STMDIV_INITVAL     (0x2u)
#define SCU_CCUCON1_GTMDIV_INITVAL     (0x2u)
#define SCU_CCUCON1_ETHDIV_INITVAL     (0x1u)
#define SCU_CCUCON1_ASCLINFDIV_INITVAL (0x0u)
#define SCU_CCUCON1_ASCLINSDIV_INITVAL (0x0u)
#define SCU_CCUCON1_INSEL_INITVAL      (0x1u)
#define SCU_CCUCON1_UP_INITVAL         (0x1u)
#define SCU_CCUCON1_LCK_INITVAL        (0x0u)

#define SCU_CCUCON1_INITVAL \
    ( (SCU_CCUCON1_CANDIV_INITVAL      << SCU_CCUCON1_CANDIV_OFF)\
    | (SCU_CCUCON1_ERAYDIV_INITVAL     << SCU_CCUCON1_ERAYDIV_OFF)\
    | (SCU_CCUCON1_STMDIV_INITVAL      << SCU_CCUCON1_STMDIV_OFF)\
    | (SCU_CCUCON1_GTMDIV_INITVAL      << SCU_CCUCON1_GTMDIV_OFF)\
    | (SCU_CCUCON1_ETHDIV_INITVAL      << SCU_CCUCON1_ETHDIV_OFF)\
    | (SCU_CCUCON1_ASCLINFDIV_INITVAL  << SCU_CCUCON1_ASCLINFDIV_OFF)\
    | (SCU_CCUCON1_ASCLINSDIV_INITVAL  << SCU_CCUCON1_ASCLINSDIV_OFF)\
    | (SCU_CCUCON1_INSEL_INITVAL       << SCU_CCUCON1_INSEL_OFF)\
    | (SCU_CCUCON1_UP_INITVAL          << SCU_CCUCON1_UP_OFF)\
    | (SCU_CCUCON1_LCK_INITVAL         << SCU_CCUCON1_LCK_OFF))


/*-----------------[CCUCON2]--------------------------------------------------*/

/** Address of the CCUCON2 register. */
#define SCU_CCUCON2             (*(volatile uint32_t*)0xF0036040u)

/* Offsets and masks inside the CCUCON2 register */

#define SCU_CCUCON2_BBBDIV_OFF  (0)
#define SCU_CCUCON2_BBBDIV_BITS (0xf)
#define SCU_CCUCON2_BBBDIV_MASK (SCU_CCUCON2_BBBDIV_BITS << SCU_CCUCON2_BBBDIV_OFF)

#define SCU_CCUCON2_UP_OFF      (30)
#define SCU_CCUCON2_UP_BITS     (0x1)
#define SCU_CCUCON2_UP_MASK     (SCU_CCUCON2_UP_BITS << SCU_CCUCON2_UP_OFF)

#define SCU_CCUCON2_LCK_OFF     (31)
#define SCU_CCUCON2_LCK_BITS    (0x1)
#define SCU_CCUCON2_LCK_MASK    (SCU_CCUCON2_LCK_BITS << SCU_CCUCON2_LCK_OFF)


/* Initial register values */

#define SCU_CCUCON2_BBBDIV_INITVAL (0x2u)
#define SCU_CCUCON2_UP_INITVAL     (0x1u)

#define SCU_CCUCON2_INITVAL \
    ( (SCU_CCUCON2_BBBDIV_INITVAL << SCU_CCUCON2_BBBDIV_OFF)\
    | (SCU_CCUCON2_UP_INITVAL     << SCU_CCUCON2_UP_OFF))


/*-----------------[CCUCON6-8]------------------------------------------------*/

#define SCU_CCUCON6  (*(volatile uint32_t*)0xF0036080u)
#define SCU_CCUCON7  (*(volatile uint32_t*)0xF0036084u)
#define SCU_CCUCON8  (*(volatile uint32_t*)0xF0036088u)


#define SCU_CCUCON_CPUnDIV_OFF  (0)
#define SCU_CCUCON_CPUnDIV_BITS (0x3f)
#define SCU_CCUCON_CPUnDIV_MASK (SCU_CCUCON_CPUnDIV_BITS << SCU_CCUCON_CPUnDIV_OFF)

#define SCU_CCUCON6_CPU0DIV_READ() ((SCU_CCUCON6 & SCU_CCUCON_CPUnDIV_MASK) >> SCU_CCUCON_CPUnDIV_OFF)
#define SCU_CCUCON7_CPU1DIV_READ() ((SCU_CCUCON7 & SCU_CCUCON_CPUnDIV_MASK) >> SCU_CCUCON_CPUnDIV_OFF)
#define SCU_CCUCON8_CPU2DIV_READ() ((SCU_CCUCON8 & SCU_CCUCON_CPUnDIV_MASK) >> SCU_CCUCON_CPUnDIV_OFF)

/*-----------------[PLLCON0]--------------------------------------------------*/

/** Address of the PLLCON0 register. */
#define SCU_PLLCON0                (*(volatile uint32_t*)0xF0036018u)

/* Offsets and masks inside the PLLCON0 register */

#define SCU_PLLCON0_VCOBYP_OFF     (0)
#define SCU_PLLCON0_VCOBYP_BITS    (0x1)
#define SCU_PLLCON0_VCOBYP_MASK    (SCU_PLLCON0_VCOBYP_BITS << SCU_PLLCON0_VCOBYP_OFF)

#define SCU_PLLCON0_VCOPWD_OFF     (1)
#define SCU_PLLCON0_MODEN_OFF      (2)

#define SCU_PLLCON0_SETFINDIS_OFF  (4)
#define SCU_PLLCON0_SETFINDIS_BITS (0x1)
#define SCU_PLLCON0_SETFINDIS_MASK (SCU_PLLCON0_SETFINDIS_BITS << SCU_PLLCON0_SETFINDIS_OFF)

#define SCU_PLLCON0_CLRFINDIS_OFF  (5)
#define SCU_PLLCON0_CLRFINDIS_BITS (0x1)
#define SCU_PLLCON0_CLRFINDIS_MASK (SCU_PLLCON0_CLRFINDIS_BITS << SCU_PLLCON0_CLRFINDIS_OFF)

#define SCU_PLLCON0_OSCDISCDIS_OFF (6)

#define SCU_PLLCON0_NDIV_OFF       (9)
#define SCU_PLLCON0_NDIV_BITS      (0x7f)
#define SCU_PLLCON0_NDIV_MASK      (SCU_PLLCON0_NDIV_BITS << SCU_PLLCON0_NDIV_OFF)
#define SCU_PLLCON0_NDIV_READ()    (((SCU_PLLCON0 & SCU_PLLCON0_NDIV_MASK) >> SCU_PLLCON0_NDIV_OFF) + 1)

#define SCU_PLLCON0_PLLPWD_OFF     (16)
#define SCU_PLLCON0_RESLD_OFF      (18)

#define SCU_PLLCON0_PDIV_OFF       (24)
#define SCU_PLLCON0_PDIV_BITS      (0xf)
#define SCU_PLLCON0_PDIV_MASK      (SCU_PLLCON0_PDIV_BITS << SCU_PLLCON0_PDIV_OFF)
#define SCU_PLLCON0_PDIV_READ()    (((SCU_PLLCON0 & SCU_PLLCON0_PDIV_MASK) >> SCU_PLLCON0_PDIV_OFF) + 1)


/* Initial register values */

#define SCU_PLLCON0_VCOBYP_INITVAL      (0x0u)
#define SCU_PLLCON0_VCOPWD_INITVAL      (0x0u)
#define SCU_PLLCON0_MODEN_INITVAL       (0x0u)
#define SCU_PLLCON0_SETFINDIS_INITVAL   (0x0u)
#define SCU_PLLCON0_CLRFINDIS_INITVAL   (0x0u)
#define SCU_PLLCON0_OSCDISCDIS_INITVAL  (0x0u)
#define SCU_PLLCON0_NDIV_INITVAL        (0x3Bu)
#define SCU_PLLCON0_PLLPWD_INITVAL      (0x1u)
#define SCU_PLLCON0_RESLD_INITVAL       (0x0u)
#define SCU_PLLCON0_PDIV_INITVAL        (0x1u)

#define SCU_PLLCON0_INITVAL \
    ( (SCU_PLLCON0_VCOBYP_INITVAL      << SCU_PLLCON0_VCOBYP_OFF    )\
    | (SCU_PLLCON0_VCOPWD_INITVAL      << SCU_PLLCON0_VCOPWD_OFF    )\
    | (SCU_PLLCON0_MODEN_INITVAL       << SCU_PLLCON0_MODEN_OFF     )\
    | (SCU_PLLCON0_SETFINDIS_INITVAL   << SCU_PLLCON0_SETFINDIS_OFF )\
    | (SCU_PLLCON0_CLRFINDIS_INITVAL   << SCU_PLLCON0_CLRFINDIS_OFF )\
    | (SCU_PLLCON0_OSCDISCDIS_INITVAL  << SCU_PLLCON0_OSCDISCDIS_OFF)\
    | (SCU_PLLCON0_NDIV_INITVAL        << SCU_PLLCON0_NDIV_OFF      )\
    | (SCU_PLLCON0_PLLPWD_INITVAL      << SCU_PLLCON0_PLLPWD_OFF    )\
    | (SCU_PLLCON0_RESLD_INITVAL       << SCU_PLLCON0_RESLD_OFF     )\
    | (SCU_PLLCON0_PDIV_INITVAL        << SCU_PLLCON0_PDIV_OFF      ))

/*-----------------[PLLCON1]--------------------------------------------------*/

/** Address of the PLLCON1 register. */
#define SCU_PLLCON1            (*(volatile uint32_t*)0xF003601Cu)

/* Offsets and masks inside the PLLCON1 register */

#define SCU_PLLCON1_K2DIV_OFF  (0)
#define SCU_PLLCON1_K2DIV_BITS (0x7f)
#define SCU_PLLCON1_K2DIV_MASK (SCU_PLLCON1_K2DIV_BITS << SCU_PLLCON1_K2DIV_OFF)
#define SCU_PLLCON1_K2DIV_READ() (((SCU_PLLCON1 & SCU_PLLCON1_K2DIV_MASK) >> SCU_PLLCON1_K2DIV_OFF) + 1)

#define SCU_PLLCON1_K3DIV_OFF  (8)
#define SCU_PLLCON1_K3DIV_BITS (0x7f)
#define SCU_PLLCON1_K3DIV_MASK (SCU_PLLCON1_K3DIV_BITS << SCU_PLLCON1_K3DIV_OFF)

#define SCU_PLLCON1_K1DIV_OFF  (16)
#define SCU_PLLCON1_K1DIV_BITS (0x7f)
#define SCU_PLLCON1_K1DIV_MASK (SCU_PLLCON1_K1DIV_BITS << SCU_PLLCON1_K1DIV_OFF)
#define SCU_PLLCON1_K1DIV_READ() (((SCU_PLLCON1 & SCU_PLLCON1_K1DIV_MASK) >> SCU_PLLCON1_K1DIV_OFF) + 1)

/* Initial register values */

#define SCU_PLLCON1_K1DIV_INITVAL (0x2u)
#define SCU_PLLCON1_K2DIV_INITVAL (0x2u)
#define SCU_PLLCON1_K3DIV_INITVAL (0x0u)

#define SCU_PLLCON1_INITVAL \
    ((SCU_PLLCON1_K1DIV_INITVAL << SCU_PLLCON1_K1DIV_OFF)\
    |(SCU_PLLCON1_K2DIV_INITVAL << SCU_PLLCON1_K2DIV_OFF)\
    |(SCU_PLLCON1_K3DIV_INITVAL << SCU_PLLCON1_K3DIV_OFF))



#endif /* SCU_CFG_H */

