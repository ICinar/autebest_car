/*
 * ti_uart.c
 *
 * TI AM335x UART driver
 *
 * azuepke, 2013-09-17: initial
 * azuepke, 2013-12-23: Beaglebone Black port
 * azuepke, 2014-05-06: adapted to MPU
 */

/*
 * See chapters 19 of the TI manual:
 * "AM335x ARM Cortex-A8 Microprocessors (MPUs) Technical Reference Manual"
 * October 2011 -- Revised December 2013
 *
 * Specs:
 * - the device has 64 byte RX and TX FIFOs
 * - we always use 16x over sampling (clocked via FCLOCK)
 * -
 */

#include <arm_io.h>
#include <board.h>
#include <board_stuff.h>
#include <hv_error.h>

/** UART registers always accessible */
#define UART_LCR				0x0c	/* line control register */
#define UART_TCR				0x18	/* transmission control register */
#define UART_TLR				0x1c	/* trigger level register */
#define UART_MDR1				0x20	/* mode definition register 1 */
#define UART_MDR2				0x24	/* mode definition register 2 */
#define UART_MDR3				0x80	/* mode definition register 3 */
/* FIFO registers 0x64..0x84, see page 3984 */

#define UART_MVR				0x50	/* module version register */
#define UART_SYSC				0x54	/* system configuration register */
#define UART_SYSS				0x58	/* system status register */
#define UART_WER				0x5c	/* wake-up enable register */

/** UART registers in configuration mode b */
#define UART_DLL				0x00	/* divisor latches low register */
#define UART_DLH				0x04	/* divisor latches high register */
#define UART_EFR				0x08	/* enhanced feature register */
#define UART_UASR				0x38	/* autobauding status register */
#define UART_SCR				0x40	/* supplementary control register */
#define UART_SSR				0x44	/* supplementary status register */

/** UART registers in operational mode */
#define UART_THR				0x00	/* transmit holding register */
#define UART_RHR				0x00	/* receiver holding register */
#define UART_IER				0x04	/* interrupt enable register */
#define UART_FCR				0x08	/* FIFO control register */
#define UART_MCR				0x10	/* modem control register */
#define UART_LSR				0x14	/* line status register */
#define UART_MSR				0x18	/* modem status register */
#define UART_SPR				0x1c	/* scratchpad register */

/* register accessors */
static inline uint16_t rd(unsigned int reg)
{
	return readw((volatile void *)(UART_BASE + reg));
}

static inline void wr(unsigned int reg, uint16_t val)
{
	writew((volatile void *)(UART_BASE + reg), val);
}

/* bits in LSR */
#define UART_LSR_RXFIFOE		0x01	/* RX FIFO is not empty*/
#define UART_LSR_RXOE			0x02	/* overrun error */
#define UART_LSR_RXPE			0x04	/* parity error */
#define UART_LSR_RXFE			0x08	/* framing error */
#define UART_LSR_RXBI			0x10	/* break indicator */
#define UART_LSR_TXFIFOE		0x20	/* TX FIFO is empty */
#define UART_LSR_TXSRE			0x40	/* TX hold register empty */
#define UART_LSR_RXFIFOSTS		0x80	/* at least one error in RX FIFO */

/* bits in SSR */
#define UART_SSR_TXFIFOFULL		0x01	/* TX FIFO is full */
#define UART_SSR_RXCTSDSRBLABLA	0x02	/* detected falling edge on RX|CTS|DSR */
#define UART_SSR_DMACOUNTERRST	0x04	/* reset DMA counters */

unsigned int board_putc(int c)
{
	/* poll until TX FIFO is no longer full */
	while (rd(UART_SSR) & UART_SSR_TXFIFOFULL) {
		return E_OS_NOFUNC;
	}

	wr(UART_THR, (unsigned char)c);
	return E_OK;
}

__init void serial_init(unsigned int baudrate)
{
	unsigned int div;

	/* see chapter 19.4.1 "UART Programming Model" in the manual! */

	//////////////////////////////////////////////////////////////

	/* reset UART and poll until completed */
	wr(UART_SYSC, 0x0002);
	while ((rd(UART_SYSS) & 0x0001) == 0)
		;

	//////////////////////////////////////////////////////////////

	/* FIFO and DMA settings */

	/* 1. set configuration mode B */
	wr(UART_LCR, 0x00bf);

	/* 2. allow access to TCR_TLR (part 1) */
	wr(UART_EFR, 0x10);

	/* 3. set configuration mode A */
	wr(UART_LCR, 0x0080);

	/* 4. allow access to TCR_TLR (part 2) */
	wr(UART_MCR, 0x0040);

	/* 5. enables FIFO mode, clears FIFOs, DMA disabled */
	wr(UART_FCR, 0x0007);

	/* steps 6 to 12 skipped, we stay with the defaults */

	//////////////////////////////////////////////////////////////

	/* protocol, baud rate, and interrupt settings */

	/* 1. disable UART before updating clocks */
	wr(UART_MDR1, 0x0007);

	/* 2. set configuration mode B */
	wr(UART_LCR, 0x00bf);

	/* 3. allow access to IER */
	wr(UART_EFR, 0x10);

	/* 4. set operational mode */
	wr(UART_LCR, 0x0000);

	/* 5. clear IER register */
	wr(UART_IER, 0x0000);

	/* 6. set configuration mode B */
	wr(UART_LCR, 0x00bf);

	/* 7. set new divisor */
	div = (UART_CLOCK + (baudrate * 16) / 2) / (baudrate * 16);
	wr(UART_DLH, (div >> 8) & 0xff);
	wr(UART_DLL, (div >> 0) & 0xff);

	/* 8. set operational mode */
	wr(UART_LCR, 0x0000);

	/* 9. reprogram IER register (we keep all interrupts disabled) */
	wr(UART_IER, 0x0000);

	/* 10. set configuration mode B */
	wr(UART_LCR, 0x00bf);

	/* 11. disable any flow control */
	wr(UART_EFR, 0);

	/* 12. set 8n1 mode */
	wr(UART_LCR, 0x03);

	/* 13. set UART 16x mode */
	wr(UART_MDR1, 0x0000);
}
