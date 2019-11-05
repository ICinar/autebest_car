/**
 * \file     leds.h
 * \brief    Interface of the LEDs driver.
 * \details  This is a simple driver for the LEDS of the Calypso board MPC5748G.
 *
 * \date     18.08.2015
 * \author   easycore GmbH, 91058 Erlangen, Germany
 */

#if (!defined LEDS_H)
#define LEDS_H

/*==================[inclusions]==============================================*/

#include <stdint.h> /* uint32_t */

/*==================[macros]==================================================*/

/* Symbols for the single LEDs.
 * You use these names when you call the functions of the leds driver. */
#define LED1 0u
#define LED2 1u
#define LED3 2u
#define LED4 3u

/* Indexes of the led pins in the MSCR and GPDO blocks */
#define MSCR_LED1 98u
#define MSCR_LED2 99u
#define MSCR_LED3 100u
#define MSCR_LED4 101u

/* Base address of the SIUL2 module */
#define SIUL2_BASE         (0xFFFC0000u)

/* Base address of the Multiplexed Signal Configuration Register Array.
 * This is where you setup the individual pins. */
#define SIUL2_MSCR_BASE    (SIUL2_BASE + 0x0240u)

/* Array of MSCR registers beginning with LED1.
 * Indexed by led number.
 * This is where you setup the individual led pins.
 */
#define LED_CFG            ((volatile uint32_t*) (SIUL2_MSCR_BASE + (4u * MSCR_LED1)))


/* Base address of the GPIO Pad Data Output Register array.
 * This is where you send the output data. */
#define SIUL2_GPDO_BASE    (SIUL2_BASE + 0x1300u)

/* Array of GPDO registers beginning with LED1.
 * Indexed by led number.
 * This is where you send the output data.
 */
#define LED_OUTPUT         ((volatile uint8_t*) (SIUL2_GPDO_BASE + MSCR_LED1))

/* MSCR configuration: general output pin. */
#define MSCR_OUTPUT        (0x02000000u)
/* MSCR configuration: general input pin. */
#define MSCR_INPUT         (0x00080000u)


/* LEDs are connected backwards 0 for ON, 1 for OFF */
#define LED_ON  (0)
#define LED_OFF (1)

#define NR_LEDS  4u

/*------------------[Buttons]-------------------------------------------------*/

#define MSCR_BTN1 1
#define MSCR_BTN2 2
#define MSCR_BTN3 89
#define MSCR_BTN4 91

#define BTN1_CFG           (*(volatile uint32_t*) (SIUL2_MSCR_BASE + (4u * MSCR_BTN1)))
#define BTN2_CFG           (*(volatile uint32_t*) (SIUL2_MSCR_BASE + (4u * MSCR_BTN2)))
#define BTN3_CFG           (*(volatile uint32_t*) (SIUL2_MSCR_BASE + (4u * MSCR_BTN3)))
#define BTN4_CFG           (*(volatile uint32_t*) (SIUL2_MSCR_BASE + (4u * MSCR_BTN4)))

/*==================[external function declarations]==========================*/

/** Initialize LEDs. */
void leds_init(void);

/**
 * \brief Turn on led.
 * \param nr LED number in range [0..NR_LEDS-1].
 */
void led_on(unsigned int nr);

/**
 * \brief Turn off led.
 * \param nr LED number in range [0..NR_LEDS-1].
 */
void led_off(unsigned int nr);

/**
 * \brief Toggle led.
 * \param nr LED number in range [0..NR_LEDS-1].
 */
void led_toggle(unsigned int nr);


/**
 * \brief   Display binary representation of nr using leds.
 * \details The leds 0 to 3 are turned on or off according to the bits of nr.
 *          As our Calypso board has only 4 leds, we can display only numbers
 *          in range 0 to 15.
 * \param nr Number in range [0..15].
 */
void led_display(unsigned int nr);

/* Test function. This will go away. */
void leds_task(void);



#endif /* if (!defined LEDS_H) */
/*=================[end of file]==============================================*/
