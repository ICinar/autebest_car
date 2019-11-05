/**
 * \file     leds.c
 * \brief    Implementation of the LEDs driver.
 * \see      leds.h
 * \date     25.02.2015
 * \author   easycore GmbH, 91058 Erlangen, Germany
 */

/*==================[inclusions]==============================================*/

#include "leds.h"

/*==================[internal data]===========================================*/


/*==================[external function definitions]===========================*/

void leds_init(void)
{
    /* initialise LEDs P33.6 to P33.7 */
    PORT33_IOCR4 =
        ( (PIN_OUTPUT << IOCR4_PC6_OFF)
        | (PIN_OUTPUT << IOCR4_PC7_OFF)
        );

    /* initialise LEDs P33.8 to P33.11 */
    PORT33_IOCR8 =
        ( (PIN_OUTPUT << IOCR8_PC8_OFF)
        | (PIN_OUTPUT << IOCR8_PC9_OFF)
        | (PIN_OUTPUT << IOCR8_PC10_OFF)
        | (PIN_OUTPUT << IOCR8_PC11_OFF)
        );

    /* initialise LEDs P33.12 to P33.13 */
    PORT33_IOCR12 =
        ( (PIN_OUTPUT << IOCR12_PC12_OFF)
        | (PIN_OUTPUT << IOCR12_PC13_OFF)
        );

    /* all LEDs OFF */
    PORT33_OMR = (ALL_LEDS_MASK << FIRST_LED_PIN);
}

void led_on(unsigned int nr)
{
    if (nr < NR_LEDS)
    {
        /* Clear bit */
        PORT33_OMR = (1 << (FIRST_LED_PIN + OMR_PCL_OFF + nr));
    }
}

void led_off(unsigned int nr)
{
    if (nr < NR_LEDS)
    {
        /* Set bit */
        PORT33_OMR = (1 << (FIRST_LED_PIN + nr));
    }
}

void led_toggle(unsigned int nr)
{
    if (nr < NR_LEDS)
    {
        /* Set and Clear pin toogles it */
        PORT33_OMR = ( (1 << (FIRST_LED_PIN + OMR_PCL_OFF + nr))
                     | (1 << (FIRST_LED_PIN + nr)));
    }
}

#ifdef USE_LED_TASK
static void leds_print_number(unsigned int nr)
{
    unsigned int ledno = 0;

    while (ledno < NR_LEDS)
    {
        if ((nr & (0x80 >> ledno)) != 0)
        {
            led_on(ledno);
        }
        else
        {
            led_off(ledno);
        }

        ledno++;
    }
}

void leds_task(void)
{
    static unsigned int call_counter = 0u;
    static unsigned int ledoutput    = 0u;
    call_counter++;

    if (call_counter >= 1000)
    {
        call_counter = 0;
        ledoutput++;
        ledoutput = ledoutput & 0xFF;
        leds_print_number(ledoutput);
    }
}
#endif

/*=================[end of file]==============================================*/
