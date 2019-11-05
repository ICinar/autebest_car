/**
 * \file   serial.h
 * \brief  Interface of the serial module.
 * \details
 *
 * The serial interface is connected to the module asclin0.
 *
 * \par Settings of the serial driver
 *
 * - baudrate:  38400
 * - data bits: 8
 * - stop bits: 1
 * - parity:    no
 * - no handshake (no flow control)
 *
 * \par Baud rate computation
 *
 * The baudate is computed using the following variables:
 *
 * - fA:           The system clock.
 *                 We assume this clock is 100 Mhz.
 * - prescaler:    Divider of fA.
 *                 Set up in register field BITCON.PRESCALER.
 *                 The value of this register field is defined by symbol
 *                 ASC0_BITCON_PRESCALER_INITVAL.
 * - numerator:    Nominator of the fractional divider.
 *                 Set up in BRG.NUMERATOR.
 *                 The value of this register field is defined by symbol
 *                 ASC0_BRG_NUMERATOR_INITVAL.
 * - denominator:  Denominator of the fractional divider.
 *                 Set up in register field BRG.DENOMINATOR.
 *                 The value of this register field is defined by symbol
 *                 ASC0_BRG_DENOMINATOR_INITVAL.
 * - oversampling: Division ratio of the baudrate post divider.
 *                 Set up in BITCON.OVERSAMPLING.
 *                 The value of this register field is defined by symbol
 *                 ASC0_BITCON_OVERSAMPLING_INITVAL.
 *
 * The baud rate is computed using the formula:
 *
 *   baudrate = (((fA/prescaler) * numerator) / denominator) / oversampling
 *
 * The prescaler and oversampling are incremented by one.
 * For this reason we store them decremented by one. So in order to setup the
 * prescaler to 10, we store it as 9.
 *
 * See also User Manual, section "19.5.1 Baud Rate Generation".
 *
 * The output of the ASC0 serial interface goes to the USB port and to pin P15.2.
 * So if you plan to measure the real hardware baud rate you might want to use
 * an oscilloscope connected to P15.2.
 *
 * \author easycore GmbH, 91058 Erlangen, Germany
 * \date   02.03.2015
 */

#ifndef SERIAL_H
#define SERIAL_H

/*===========[Register definitions and initialisation values]=================*/


/*-----------------[FLAGS]-------------------------------------------------*/

/* Address of the Flags Register */
#define ASC0_FLAGS (*(volatile unsigned int*)0xF0000634u)

/* Offset of the TFL (Transmit FIFO Level Flag) bit in the FLAGSSET register. */
#define ASC0_FLAGS_TFL_OFF (31)

/*-----------------[FLAGSSET]-------------------------------------------------*/
/* Flags Set Register.
 * The FLAGSSET register contains the write only bits used to set the
 * corresponding bits in the FLAGS register by software.
 * Setting a flag bit triggers an interrupt, if the corresponding interrupt
 * enable bit is set. */

/*Address of the Flags Set Register. */
#define ASC0_FLAGSSET (*(volatile unsigned int*)0xF0000638u)

/*-----------------[FLAGSCLEAR]-----------------------------------------------*/

/** Address of the Flags Clear Register. */

#define ASC0_FLAGSCLEAR  (*(volatile unsigned int*)0xF000063Cu)

/*-----------------[FLAGSENABLE]-----------------------------------------------*/
/* The FLAGSENABLE register contains the read write bits that enable the error
 * interrupt in case the corresponding event has occurred. */

/* Address of the FLAGSENABLE register. */
#define ASC0_FLAGSENABLE         (*(volatile unsigned int*)0xF0000640u)

/* Enable transmission interrupts. */
#define ASC0_FLAGSENABLE_INITVAL (1 << ASC0_FLAGS_TFL_OFF)


/*-----------------[CLC]------------------------------------------------------*/

/* Address of the Clock Control Register (CLC) */
#define ASC0_CLC (*(volatile unsigned int*)0xF0000600u)


/*-----------------[CSR]------------------------------------------------------*/


/* Address of the Clock Selection Register (CSR) */
#define ASC0_CSR (*(volatile unsigned int*)0xF000064Cu)

/* No clock source, this shuts down ASC0 so we can program it.*/
#define ASC0_CSR_NO_SOURCE               (0)
/* Select CLC as clock source fA for ASC0. */
#define ASC0_CSR_CLK                     (1)

/*-----------------[FRAMECON]-------------------------------------------------*/
/* The parameters regarding the properties of the message frame of the ASC
 * communication are controlled by the Frame Control Register FRAMECON. */

/** Address of register Frame Control Register (FRAMECON) of ASC0 */
#define ASC0_FRAMECON  (*(volatile unsigned int*)0xF0000618u)

/* Offset of bit field STOP in the FRAMECON register. */
#define ASC0_FRAMECON_STOP_OFF     (9)
#define ASC0_FRAMECON_STOP_INITVAL (1)/* STOP: 1 bit */

/* Offset of bit field MODE in the FRAMECON register. */
#define ASC0_FRAMECON_MODE_OFF     (16)
#define ASC0_FRAMECON_MODE_INIT    (0)/* MODE: Init */
#define ASC0_FRAMECON_MODE_ASC     (1)/* MODE: ASC */

/* Offset of bit field PEN in the FRAMECON register. */
#define ASC0_FRAMECON_PEN_OFF      (30)
#define ASC0_FRAMECON_PEN_INITVAL  (0)/* PEN: no parity */

/* Value of the FRAMECON register with MODE set to INIT */
#define ASC0_FRAMECON_INITVAL_INIT \
    ( (ASC0_FRAMECON_STOP_INITVAL << ASC0_FRAMECON_STOP_OFF)\
    | (ASC0_FRAMECON_MODE_INIT    << ASC0_FRAMECON_MODE_OFF)\
    | (ASC0_FRAMECON_PEN_INITVAL  << ASC0_FRAMECON_PEN_OFF))

/* Value of the FRAMECON register with MODE set to ASC */
#define ASC0_FRAMECON_INITVAL_ASC \
    ( (ASC0_FRAMECON_STOP_INITVAL << ASC0_FRAMECON_STOP_OFF)\
    | (ASC0_FRAMECON_MODE_ASC     << ASC0_FRAMECON_MODE_OFF)\
    | (ASC0_FRAMECON_PEN_INITVAL  << ASC0_FRAMECON_PEN_OFF))



/*-----------------[DATCON]---------------------------------------------------*/
/* The DATCON register defines the number of bits in the ASC and SPI frames. */

/* Address of the DATCON register. */
#define ASC0_DATCON  (*(volatile unsigned int*)0xF000061Cu)

/* Offset of the DATLEN bit field. */
#define ASC0_DATCON_DATLEN_OFF   (0)
/* Data length: 8 bits. This is represented by a 7. See user manual.
 * The number of stop bits is stored in register FRAMECON.STOP. */
#define ASC0_DATCON_DATALEN_INITVAL (7)

/* Initialisation value of the register DATCON. */
#define ASC0_DATCON_INTIVAL (ASC0_DATCON_DATALEN_INITVAL << ASC0_DATCON_DATLEN_OFF)


/*-----------------[TXFIFOCON]------------------------------------------------*/
/* TX FIFO Configuration Register. Bit fields we set:
 * - FLUSH - empty the FIFO
 * - ENO - enable tx fifo outlet, this is only for LIN usage important
 * - INW - number of bytes written to the Tx FIFO with one FPI bus write
 */

/* Address of register TXFIFOCON. */
#define ASC0_TXFIFOCON  (*(volatile unsigned int*)0xF000060Cu)

/* Offset of the FLUSH bit field. */
#define ASC0_TXFIFOCON_FLUSH_OFF     (0)
/* Initial value of the FLUSH bit field.
 * 1 means empty the FIFO, 0 has no effect. */
#define ASC0_TXFIFOCON_FLUSH_INITVAL (1)


#define ASC0_TXFIFOCON_ENO_OFF       (1)
#define ASC0_TXFIFOCON_ENO_INITVAL   (1)

/* Offset of the INW bit field. */
#define ASC0_TXFIFOCON_INW_OFF       (6)
/* Initialisation value of INW bit field. This is how many bytes we want to be
 * written to TXFIFO when we write the TXDATA register.
 * 1 means one byte, 3 means 4 bytes. */
#define ASC0_TXFIFOCON_INW_INITVAL   (1)

/* Initialisation value of the TXFIFOCON register. */
#define ASC0_TXFIFOCON_INITVAL \
    ( (ASC0_TXFIFOCON_FLUSH_INITVAL << ASC0_TXFIFOCON_FLUSH_OFF)\
    | (ASC0_TXFIFOCON_ENO_INITVAL   << ASC0_TXFIFOCON_ENO_OFF)\
    | (ASC0_TXFIFOCON_INW_INITVAL)  << ASC0_TXFIFOCON_INW_OFF);


/* True if the TX FIFO is busy with transmission */
#define ASC0_TXFIFO_ISBUSY()   ((ASC0_FLAGS & (1u << ASC0_FLAGS_TFL_OFF)) == 0)

/* True if the TX FIFO is ready to send. */
#define ASC0_TXFIFO_ISREADY()  ((ASC0_FLAGS & (1u << ASC0_FLAGS_TFL_OFF)) != 0)

/* Set the transmission FIFO in idle mode (ready for transmission). */
#define ASC0_TXFIFO_SETREADY() (ASC0_FLAGSSET = (1u << ASC0_FLAGS_TFL_OFF))

/* Set TX FIFO in busy mode */
#define ASC0_TXFIFO_SETBUSY()  (ASC0_FLAGSCLEAR = (1u << ASC0_FLAGS_TFL_OFF))


/*-----------------[TXDATA]---------------------------------------------------*/
/* Transmit Data Register.
 * Writing data to this register enters the data to the TXFIFO.
 * This is where we write we write the byte to transmit.
 */

/* Address of the Transmit Data Register. */
#define ASC0_TXDATA (*(volatile unsigned int*)0xF0000644u)

/* Write a byte to the TX FIFO. */
#define PUTCHAR(c)    (ASC0_TXDATA = (unsigned int)c)

/*-----------------[BITCON]---------------------------------------------------*/
/* Bit Configuration Register.
 * The BITCON Register defines the integer timer parameters in the baud rate
 * generation block. Bit fields we set:
 * - PRESCALER - Prescaling of the Fractional Divider
 * - OVERSAMPLING - Defines the bit length in ticks
 * - SAMPLEPOINT - Sample Point Position
 * - SM - Sample Mode, Number of samples per bit
 */

/* Address of the BITCON register. */
#define ASC0_BITCON (*(volatile unsigned int*)0xF0000614u)

/* Offset and initialisation value of the PRESCALER. */
#define ASC0_BITCON_PRESCALER_OFF        (0)
#define ASC0_BITCON_PRESCALER_INITVAL    (10)

/* Offset and initialisation value of the OVERSAMPLING. */
#define ASC0_BITCON_OVERSAMPLING_OFF     (16)
#define ASC0_BITCON_OVERSAMPLING_INITVAL (16)

/* Offset and initialisation value of the OVERSAMPLING. Position: 7,8,9. */
#define ASC0_BITCON_SAMPLEPOINT_OFF      (24)
#define ASC0_BITCON_SAMPLEPOINT_INITVAL  (9)

/* Offset and initialisation value of the SM (Sample Mode). 3 samples per bit */
#define ASC0_BITCON_SM_OFF               (31)
#define ASC0_BITCON_SM_INITVAL           (1)

/* Initialisation value of the BITCON register. */
#define ASC0_BITCON_INITVAL \
    ( ((ASC0_BITCON_PRESCALER_INITVAL - 1)    << ASC0_BITCON_PRESCALER_OFF) \
    | ((ASC0_BITCON_OVERSAMPLING_INITVAL - 1) << ASC0_BITCON_OVERSAMPLING_OFF)\
    | (ASC0_BITCON_SAMPLEPOINT_INITVAL        << ASC0_BITCON_SAMPLEPOINT_OFF)\
    | (ASC0_BITCON_SM_INITVAL                 << ASC0_BITCON_SM_OFF));


/*-----------------[BRG]------------------------------------------------------*/
/* Baud Rate Generation Register.
 * Configures the numerator and the denominator of the fractional divider
 * in the baud rate generation block. Bit fields we set:
 * - DENOMINATOR: denominator of the fractional divider
 * - NUMERATOR: numerator of the fractional divider
 */

/* Address of the BRG register. */
#define ASC0_BRG (*(volatile unsigned int*)0xF0000620u)

/* Offset and value of DENOMINATOR. */
#define ASC0_BRG_DENOMINATOR_OFF         (0)
#define ASC0_BRG_DENOMINATOR_INITVAL     (3125)

/* Offset and value of NUMERATOR */
#define ASC0_BRG_NUMERATOR_OFF           (16)
#define ASC0_BRG_NUMERATOR_INITVAL       (48 * 4)

/* Initialisation value of the BRG register.*/
#define ASC0_BRG_INITVAL \
    ( (ASC0_BRG_DENOMINATOR_INITVAL << ASC0_BRG_DENOMINATOR_OFF)\
    | (ASC0_BRG_NUMERATOR_INITVAL   << ASC0_BRG_NUMERATOR_OFF));


/*-----------------[PORT 15]--------------------------------------------------*/


/* Output pin of the ASCLIN0 is P15.2  */
#define PORT15_IOCR0 (*(volatile unsigned int*)0xF003B510u)

/* Offset of the PC2 bit field in the P15_IOCR0 register.
 * This bit field is where we setup the function of pin P15.2.*/
#define IOCR0_PC2_OFF (19)

/* Address of the OMR register of port P15.
 * This is where we set and reset the pins of port 15. */
#define PORT15_OMR   (*(volatile unsigned int*)0xF003B504u)

/* Offset of the PS2 bit in the OMR register.
 * This bit is used to set the pin P15.2. */
#define OMR_PS2_OFF  (2)

/* Port P15.2 setting: ASCLIN0 output (Output Alternate 2 Function), Push/Pull  */
#define PIN_OUT_ALT2  (0x12)

/*-----------------[Interrupt number]-----------------------------------------*/

/* see IRQ table in tc_irq_conf_tsim.c! */
#define ASC0_IRQ     (3)

/*==================[external function declarations]==========================*/

/**
 * Initialisation of the serial module.
 * This function initialises the serial module (asynchronous interface ASC0)
 * and sets up the ASC0 registers.
 */
void serial_init(void);

/**
 * Interrupt handler.
 *
 * This function is called every time when the serial interface ASC0 has
 * completed the transmission of a character.
 */
void serial_irq_handler(unsigned int irq);

#endif /* SERIAL_H */
/*==================[end of file]=============================================*/
