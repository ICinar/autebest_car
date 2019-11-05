/**
 * \file     leds.h
 * \brief    Interface of the LEDs driver.
 * \details  This is a simple driver for the LEDS of the Aurix board TC277.
 *           It provides functions to initialize, turn on, turn off and toggle
 *           leds.
 *           
 *           The 8 leds are connected to port 33 on lines P33.6 to P33.13. These
 *           lines are setup in Registers P33_IOCR4 (pins 6 and 7), P33_IOCR8
 *           (pins 8, 9, 10, 11) and P33_IOCR12 (pins 12 and 13) (see user
 *           manual page 14-14).
 *
 *           Turning the LEDs on and off is done by writing the register OMR of
 *           port 33. As the LEDs work with negative logic (high level when LED
 *           is off, low level when LED is on), turning on a LED is done by
 *           clearing the respective pin, turning off the LED is implemented by
 *           setting the respective pin.
 *
 * \date     25.02.2015
 * \author   easycore GmbH, 91058 Erlangen, Germany
 */

#if (!defined LEDS_H)
#define LEDS_H

/*==================[inclusions]==============================================*/

#include <stdint.h> /* uint32_t */

/*==================[macros]==================================================*/

/* Adresses of the IOCR registers. Every IOCR register is 32 bits wide and
 * contains 4 bitfields where the configuration of 4 pins is stored. */

/* Address of Port 33 Input/Output Control Register 4 */
#define PORT33_IOCR4  (*(volatile uint32_t*)0xF003D314u)

/* Address of Port 33 Input/Output Control Register 8 */
#define PORT33_IOCR8  (*(volatile uint32_t*)0xF003D318u)

/* Address of Port 33 Input/Output Control Register 12 */
#define PORT33_IOCR12 (*(volatile uint32_t*)0xF003D31Cu)

/* Address of Port 33 Output Modification Register */
#define PORT33_OMR    (*(volatile uint32_t*)0xF003D304u)

/* Offset of the PLC bits inside the OMR register */
#define OMR_PCL_OFF   (16)


/* 
 * Offsets of the pin control fields inside the IOCR registers of port 33, where
 * the leds are connected. We use these offsets to shift the pin configuration
 * within a IOCR register.  We define the offsets only for the pins we use.
 */

#define IOCR4_PC6_OFF   (19) /* Pin P33.6 inside IOCR4   */
#define IOCR4_PC7_OFF   (27) /* Pin P33.7 inside IOCR4   */
#define IOCR8_PC8_OFF   (3)  /* Pin P33.8 inside IOCR8   */
#define IOCR8_PC9_OFF   (11) /* Pin P33.9 inside IOCR8   */
#define IOCR8_PC10_OFF  (19) /* Pin P33.10 inside IOCR8  */
#define IOCR8_PC11_OFF  (27) /* Pin P33.11 inside IOCR8  */
#define IOCR12_PC12_OFF (3)  /* Pin P33.12 inside IOCR12 */
#define IOCR12_PC13_OFF (11) /* Pin P33.12 inside IOCR12 */

/**
 * Pin control: Push-Pull general purpose output.
 * This value goes into the IOCR registers for every pin we setup.
 * See user manual page 14-24, table 14-5.
 */
#define PIN_OUTPUT              (0x10u)

/** Number of available LEDs. */
#define NR_LEDS                 (8)

/** Pin number of first used LED. */
#define FIRST_LED_PIN           (6)

/**
 * Bit mask for all leds.
 * Used when doing something with all leds.
 */
#define ALL_LEDS_MASK           ((1 << NR_LEDS) - 1)


/*==================[external function declarations]==========================*/

/** Initialize LEDs. */
void leds_init(void);

/**
 * \brief Turn on led.
 * \param nr LED number in range [0..NR_LEDS].
 */
void led_on(unsigned int nr);

/**
 * \brief Turn off led.
 * \param nr LED number in range [0..NR_LEDS].
 */
void led_off(unsigned int nr);

/**
 * \brief Toggle led.
 * \param nr LED number in range [0..NR_LEDS].
 */
void led_toggle(unsigned int nr);

/* Test function. This will go away. */
void leds_task(void);



#endif /* if (!defined LEDS_H) */
/*=================[end of file]==============================================*/
