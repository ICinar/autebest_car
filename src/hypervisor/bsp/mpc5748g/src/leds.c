/**
 * \file     leds.c
 * \brief    Implementation of the LEDs driver.
 * \see      leds.h
 * \date     18.08.2015
 * \author   easycore GmbH, 91058 Erlangen, Germany
 */

/*==================[inclusions]==============================================*/

#include "leds.h"

/*==================[internal data]===========================================*/


/*==================[external function definitions]===========================*/

void leds_init(void)
{
    /* Assign LED ports as GPIO outputs */
    LED_CFG[LED1] = MSCR_OUTPUT;
    LED_CFG[LED2] = MSCR_OUTPUT;
    LED_CFG[LED3] = MSCR_OUTPUT;
    LED_CFG[LED4] = MSCR_OUTPUT;

    LED_OUTPUT[LED1] = LED_OFF;
    LED_OUTPUT[LED2] = LED_OFF;
    LED_OUTPUT[LED3] = LED_OFF;
    LED_OUTPUT[LED4] = LED_OFF;

    /* Buttons on CalypsoEVB */

    BTN1_CFG = MSCR_INPUT;
    BTN2_CFG = MSCR_INPUT;
    BTN3_CFG = MSCR_INPUT;
    BTN4_CFG = MSCR_INPUT;
}

void led_on(unsigned int nr)
{
    if (nr < NR_LEDS)
    {
        LED_OUTPUT[nr] = LED_ON;
    }
}

void led_off(unsigned int nr)
{
    if (nr < NR_LEDS)
    {
        LED_OUTPUT[nr] = LED_OFF;
    }
}

void led_toggle(unsigned int nr)
{
    if (nr < NR_LEDS)
    {
        LED_OUTPUT[nr] = !LED_OUTPUT[nr];
    }
}

void led_display(unsigned int nr)
{
    /* loop unrolling intended */

    if ((nr & 0x01) == 0)
    {
        LED_OUTPUT[0] = LED_OFF;
    }
    else
    {
        LED_OUTPUT[0] = LED_ON;
    }
    
    if ((nr & 0x02) == 0)
    {
        LED_OUTPUT[1] = LED_OFF;
    }
    else
    {
        LED_OUTPUT[1] = LED_ON;
    }
    
    if ((nr & 0x04) == 0)
    {
        LED_OUTPUT[2] = LED_OFF;
    }
    else
    {
        LED_OUTPUT[2] = LED_ON;
    }
    
    if ((nr & 0x08) == 0)
    {
        LED_OUTPUT[3] = LED_OFF;
    }
    else
    {
        LED_OUTPUT[3] = LED_ON;
    }
}

/*=================[end of file]==============================================*/