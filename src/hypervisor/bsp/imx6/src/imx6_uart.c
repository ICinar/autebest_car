/*
 * imx6_uart.c
 *
 * i.MX6 UART driver
 *
 * azuepke, 2016-04-14: initial
 * azuepke, 2016-04-18: adapted to MPU
 */

/* See chapter 64 in the i.MX6 manual (page 5169) */

#include <arm_io.h>
#include <board.h>
#include <board_stuff.h>
#include <hv_error.h>

/* serial: COM1 @ phys 0x02020000, COM2 @ phys 0x021e8000 */
#define PORT	0x021e8000
#define CLOCK	80000000	/* uart clock */


/** UART registers */
#define UART_URXD				0x00	/* receiver register */
#define UART_UTXD				0x40	/* transmitter register */
#define UART_UCR1				0x80	/* control register 1 */
#define UART_UCR2				0x84	/* control register 2 */
#define UART_UCR3				0x88	/* control register 3 */
#define UART_UCR4				0x8c	/* control register 4 */
#define UART_UFCR				0x90	/* FIFO control register */
#define UART_USR1				0x94	/* status register 1 */
#define UART_USR2				0x98	/* status register 2 */
#define UART_UESC				0x9c	/* escape character register */
#define UART_UTIM				0xa0	/* escape timer register */
#define UART_UBIR				0xa4	/* BRM incremental register */
#define UART_UBMR				0xa8	/* BRM modulator register */
#define UART_UBRC				0xac	/* baud rate count register */
#define UART_ONEMS				0xb0	/* one millisecond register */
#define UART_UTS				0xb4	/* test register */
#define UART_UMCR				0xb8	/* RS-485 mode control register */


/* register accessors */
static inline uint32_t rd(unsigned long reg)
{
	return readl((volatile void *)(PORT + reg));
}

static inline void wr(unsigned long reg, uint32_t val)
{
	writel((volatile void *)(PORT + reg), val);
}

/** UART control register bits */
// ...

/** UART status register 1 bits */
#define UART_USR1_TRDY			(1<<13)	/* transmitter requires data */

/** UART status register 2 bits */
#define UART_USR2_TXFE			(1<<14)	/* transmitter buffer empty */


unsigned int board_putc(int c)
{
	/* poll until transmitter has space for a character */
	while (!(rd(UART_USR2) & UART_USR2_TXFE)) {
		return E_OS_NOFUNC;
	}

	wr(UART_UTXD, (unsigned char)c);
	return E_OK;
}

__init void serial_init(unsigned int baudrate)
{
	/* we stick with the U-BOOT settings */
	(void)baudrate;
}
