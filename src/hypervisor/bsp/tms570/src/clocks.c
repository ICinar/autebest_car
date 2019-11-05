/*
 * clocks.c
 *
 * TMS570 clocks, pins and PLLs
 *
 * azuepke, 2014-04-24: initial (imported from HalCoGen generated code)
 */

#include <board_stuff.h>
#include <stdint.h>
#include <assert.h>
#include <sysregs.h>
#include <flashregs.h>



/* trim value for LP0 */
#define LPO_TRIM_VALUE	(((*(volatile uint32_t *)0xF00801B4U) & 0xFFFF0000U)>>16U)

/** @enum systemClockSource
*   @brief Alias names for clock sources
*
*   This enumeration is used to provide alias names for the clock sources:
*     - Oscillator
*     - Pll1
*     - External1
*     - Low Power Oscillator Low
*     - Low Power Oscillator High
*     - PLL2
*     - External2
*     - Synchronous VCLK1
*/
enum systemClockSource
{
    SYS_OSC       		= 0U,  /**< Alias for oscillator clock Source                */
    SYS_PLL1      		= 1U,  /**< Alias for Pll1 clock Source                      */
    SYS_EXTERNAL1  		= 3U,  /**< Alias for external clock Source                  */
    SYS_LPO_LOW   		= 4U,  /**< Alias for low power oscillator low clock Source  */
    SYS_LPO_HIGH  		= 5U,  /**< Alias for low power oscillator high clock Source */
    SYS_PLL2    		= 6U,  /**< Alias for Pll2 clock Source                      */
    SYS_EXTERNAL2 		= 7U,  /**< Alias for external 2 clock Source                */
    SYS_VCLK      		= 9U   /**< Alias for synchronous VCLK1 clock Source         */
};


void __init setup_pll(void)
{
	/* Disable PLL1 and PLL2 */
	SYSREG1->CSDISSET = (1U << 1U) | (1U << 6U);

	while ((SYSREG1->CSDIS & 0x42U) != 0x42U) {
		/* Wait */
	}

    /** - Configure PLL control registers */
    /** @b Initialize @b Pll1: */

    /**   - Setup pll control register 1:
    *     - Setup reset on oscillator slip
    *     - Setup bypass on pll slip
    *     - setup Pll output clock divider to max before Lock
    *     - Setup reset on oscillator fail
    *     - Setup reference clock divider
    *     - Setup Pll multiplier
    */
    SYSREG1->PLLCTL1 =  (uint32_t)0x00000000U
                        |  (uint32_t)0x20000000U
                        | (((uint32_t)0x1FU)<< 24U)
                        |  (uint32_t)0x00000000U
                        | (((uint32_t)6U - 1U)<< 16U)
                        | (((uint32_t)135U - 1U)<< 8U);

    /**   - Setup pll control register 2
    *     - Enable/Disable frequency modulation
    *     - Setup spreading rate
    *     - Setup bandwidth adjustment
    *     - Setup internal Pll output divider
    *     - Setup spreading amount
    */
    SYSREG1->PLLCTL2 =  0x00000000U
                        | (255U << 22U)
                        | (7U << 12U)
                        | ((2U - 1U)<< 9U)
                        |  61U;

    /** @b Initialize @b Pll2: */

    /**   - Setup pll2 control register :
    *     - setup Pll output clock divider to max before Lock
    *     - Setup reference clock divider 
    *     - Setup internal Pll output divider
    *     - Setup Pll multiplier          
    */
    SYSREG2->PLLCTL3 = ((2U - 1U) << 29U)
                        | ((0x1FU)<< 24U) 
                        | ((6U - 1U)<< 16U) 
                        | ((135U - 1U) << 8U);

	/** - Enable PLL(s) to start up or Lock */
    SYSREG1->CSDIS = 0x00000000U
                      | 0x00000000U 
                      | 0x00000008U 
                      | 0x00000080U 
                      | 0x00000000U 
                      | 0x00000000U 
                      | 0x00000000U;
}

void __init trim_LPO(void)
{
    /** @b Initialize Lpo: */
    /** Load TRIM values from OTP if present else load user defined values */
    if (LPO_TRIM_VALUE != 0xFFFFU) {
        SYSREG1->LPOMONCTL  = (1U << 24U)
                                | LPO_TRIM_VALUE;
    } else {

        SYSREG1->LPOMONCTL  = (1U << 24U)
                                 | (16U << 8U)
                                 | 8U;
    }
}

void __init setup_flash(void)
{
    /** - Setup flash read mode, address wait states and data wait states */
    FLASHWREG->FRDCNTL =  0x00000000U
                       | (3U << 8U)
                       | (1U << 4U)
                       |  1U;

    /** - Setup flash access wait states for bank 7 */
    FSM_WR_ENA_HL    = 0x5U;
    EEPROM_CONFIG_HL = 0x00000002U
                     | (3U << 16U);

    /** - Disable write access to flash state machine registers */
    FSM_WR_ENA_HL    = 0xAU;

    /** - Setup flash bank power modes */
    FLASHWREG->FBFALLBACK = 0x00000000U
                          | (FLASH_BANK_ACTIVE << 14U)
                          | (FLASH_BANK_ACTIVE << 2U)
                          |  FLASH_BANK_ACTIVE;
}

void __init periph_init(void)
{
    /** - Disable Peripherals before peripheral powerup*/
    SYSREG1->CLKCNTL &= 0xFFFFFEFFU;

    /** - Release peripherals from reset and enable clocks to all peripherals */
    /** - Power-up all peripherals */
    PCRREG->PSPWRDWNCLR0 = 0xFFFFFFFFU;
    PCRREG->PSPWRDWNCLR1 = 0xFFFFFFFFU;
    PCRREG->PSPWRDWNCLR2 = 0xFFFFFFFFU;
    PCRREG->PSPWRDWNCLR3 = 0xFFFFFFFFU;

    /** - Enable Peripherals */
    SYSREG1->CLKCNTL |= 1U << 8U;
}




void __init map_clocks(void)
{
	uint32_t SYS_CSVSTAT, SYS_CSDIS;

    /** @b Initialize @b Clock @b Tree: */
    /** - Disable / Enable clock domain */
    SYSREG1->CDDIS= (0 << 4U ) /* AVCLK 1 OFF */
                      |(0 << 5U ) /* AVCLK 2 OFF */
	                  |(0 << 8U ) /* VCLK3 OFF */
					  |(0 << 9U ) /* VCLK4 OFF */
	                  |(1 << 10U) /* AVCLK 3 OFF */
                      |(0 << 11U); /* AVCLK 4 OFF */


    /* Work Around for Errata SYS#46:
     * 
     * Errata Description:            
     *            Clock Source Switching Not Qualified with Clock Source Enable And Clock Source Valid
     * Workaround:
     *            Always check the CSDIS register to make sure the clock source is turned on and check 
     * the CSVSTAT register to make sure the clock source is valid. Then write to GHVSRC to switch the clock.
     */
	/** - Wait for until clocks are locked */
	SYS_CSVSTAT = SYSREG1->CSVSTAT;
	SYS_CSDIS = SYSREG1->CSDIS;
    while ((SYS_CSVSTAT & ((SYS_CSDIS ^ 0xFFU) & 0xFFU)) != ((SYS_CSDIS ^ 0xFFU) & 0xFFU))
    { 
		SYS_CSVSTAT = SYSREG1->CSVSTAT;
		SYS_CSDIS = SYSREG1->CSDIS;
	} /* Wait */


	/* Now the PLLs are locked and the PLL outputs can be sped up */
	/* The R-divider was programmed to be 0xF. Now this divider is changed to programmed value */
    SYSREG1->PLLCTL1 = (SYSREG1->PLLCTL1 & 0xE0FFFFFFU)|((1U - 1U)<< 24U);
	/*SAFETYMCUSW 134 S MR:12.2 <APPROVED> " Clear and write to the volatile register " */
    SYSREG2->PLLCTL3 = (SYSREG2->PLLCTL3 & 0xE0FFFFFFU)|((1U - 1U)<< 24U);

	/** - Map device clock domains to desired sources and configure top-level dividers */
	/** - All clock domains are working off the default clock sources until now */
	/** - The below assignments can be easily modified using the HALCoGen GUI */
    
    /** - Setup GCLK, HCLK and VCLK clock source for normal operation, power down mode and after wakeup */
    SYSREG1->GHVSRC = (SYS_PLL1 << 24U)
                       | (SYS_PLL1 << 16U) 
                       |  SYS_PLL1;
                       
    /** - Setup synchronous peripheral clock dividers for VCLK1, VCLK2, VCLK3 */
    SYSREG1->CLKCNTL  = (SYSREG1->CLKCNTL & 0xF0F0FFFFU)
						 | (1U << 24U)  
						 | (1U << 16U);  
	SYSREG2->CLK2CNTL = (SYSREG2->CLK2CNTL & 0xFFFFF0F0U)
	                     | ((1U) << 8U)
						 | (1U);


    /** - Setup RTICLK1 and RTICLK2 clocks */
    SYSREG1->RCLKSRC = (1U << 24U)
                        | (SYS_VCLK << 16U) 
                        | (1U << 8U)  
                        |  SYS_VCLK;

    /** - Setup asynchronous peripheral clock sources for AVCLK1 and AVCLK2 */
    SYSREG1->VCLKASRC = (SYS_VCLK << 8U)
                          |  SYS_VCLK;

    SYSREG2->VCLKACON1 = ((1U - 1U ) << 24U)
                           | (0U << 20U) 
                           | (SYS_VCLK << 16U)
                           | ((1U - 1U ) << 8U)
                           | (0U << 4U) 
                           | SYS_VCLK;
}

void __init set_eclk(void)
{
    /** - set ECLK pins functional mode */
    SYSREG1->SYSPC1 = 0U;

    /** - set ECLK pins default output value */
    SYSREG1->SYSPC4 = 0U;

    /** - set ECLK pins output direction */
    SYSREG1->SYSPC2 = 1U;

    /** - set ECLK pins open drain enable */
    SYSREG1->SYSPC7 = 0U;

    /** - set ECLK pins pullup/pulldown enable */
    SYSREG1->SYSPC8 = 0U;

    /** - set ECLK pins pullup/pulldown select */
    SYSREG1->SYSPC9 = 1U;

    /** - Setup ECLK */
    SYSREG1->ECPCNTL = (0U << 24U)
                        | (0U << 23U)
                        | ((8U - 1U) & 0xFFFFU);

    // FIXME: enable EMIF
    SYSREG1->GPREG1 |= 0x80000000;
}
