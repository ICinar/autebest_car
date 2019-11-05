/*
 * pl011_uart.c
 *
 * ARM PL011 UART driver
 *
 * Documentation of the ARM PL011 UART:
 * http://infocenter.arm.com/help/topic/com.arm.doc.ddi0183f/DDI0183.pdf
 *
 * azuepke, 2013-09-17: initial
 */

#include <arm_io.h>
#include <board.h>
#include <board_stuff.h>
#include <hv_error.h>

/*
 * PL011 UARTs on Versatile Express Cortex A15 board:
 *
 * UART0: 0x1c090000
 * UART1: 0x1c0a0000
 * UART2: 0x1c0b0000
 * UART3: 0x1c0c0000
 */
/* FIXME: use a board specific value here! */
#define PORT	0x1c090000
#define CLOCK	50000000


/** UART registers */
#define UARTDR					0x000	/* data register */
#define UARTRSR					0x004	/* receive status register (read) */
#define UARTECR					0x004	/* error clear register (write) */
#define UARTFR					0x018	/* flag register (read) */
#define UARTILPR				0x020	/* IrDA low-power counter register */
#define UARTIBRD				0x024	/* integer baud rate register */
#define UARTFBRD				0x028	/* fractional baud rate register */
#define UARTLCR_H				0x02c	/* line control register */
#define UARTCR					0x030	/* control register */
#define UARTIFLS				0x034	/* interrupt FIFO level select register */
#define UARTIMSC				0x038	/* interrupt mask set/clear register */
#define UARTRIS					0x03c	/* raw interrupt status register (read) */
#define UARTMIS					0x040	/* masked interrupt status register (read) */
#define UARTICR					0x044	/* interrupt clear register (write) */
#define UARTDMACR				0x048	/* DMA control register */

/* register accessors */
static inline uint32_t rd(unsigned int reg)
{
	return readl((volatile void *)(PORT + reg));
}

static inline void wr(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(PORT + reg), val);
}

/** bits in UARTFR register */
#define UARTFR_CTS				0x001	/* clear to send */
#define UARTFR_DSR				0x002	/* data set ready */
#define UARTFR_DCD				0x004	/* data carrier detect */
#define UARTFR_BUSY				0x008	/* busy */
#define UARTFR_RXFE				0x010	/* RX FIFO empty */
#define UARTFR_TXFF				0x020	/* TX FIFO full */
#define UARTFR_RXFF				0x040	/* RX FIFO full */
#define UARTFR_TXFE				0x080	/* TX FIFO empty */
#define UARTFR_RI				0x100	/* ring indicator */

/** bits in UARTLCR_H register */
#define UARTLCR_H_BRK			0x001	/* send break */
#define UARTLCR_H_PEN			0x002	/* parity enable */
#define UARTLCR_H_EPS			0x004	/* even parity */
#define UARTLCR_H_STP2			0x008	/* two stop bits */
#define UARTLCR_H_FEN			0x010	/* FIFO enable */
#define UARTLCR_H_WLEN0			0x020	/* word length bit 0 */
#define UARTLCR_H_WLEN1			0x040	/* word length bit 1 */
#define UARTLCR_H_SPS			0x080	/* stick parity select */

/** bits in UARTCR register */
#define UARTCR_UARTEN			0x0001	/* UART enable */
#define UARTCR_SIREN			0x0002	/* SIR enable */
#define UARTCR_SIRLP			0x0004	/* IrDA SIR low power mode */
#define UARTCR_LBE				0x0080	/* loopback enable */
#define UARTCR_TXE				0x0100	/* TX enable */
#define UARTCR_RXE				0x0200	/* RX enable */
#define UARTCR_DTR				0x0400	/* data transmit ready */
#define UARTCR_RTS				0x0800	/* request to send */
#define UARTCR_OUT1				0x1000	/* Out1 */
#define UARTCR_OUT2				0x2000	/* Out2 */
#define UARTCR_RTSEN			0x4000	/* RTS HW flow control enable */
#define UARTCR_CTSEN			0x8000	/* CTS HW flow control enable */

unsigned int board_putc(int c)
{
	/* poll until TX FIFO has space for a character */
	while (rd(UARTFR) & UARTFR_TXFF) {
		return E_OS_NOFUNC;
	}

	wr(UARTDR, (unsigned char)c);
	return E_OK;
}

__init void serial_init(unsigned int baudrate)
{
	unsigned int div;

	/* disable UART */
	wr(UARTCR, 0);
	/* flush FIFO */
	wr(UARTLCR_H_WLEN1, 0);

	/* set baudrate */
	/* we have a 16-bit divisor with 5-bit fraction */
	div = ((CLOCK + (baudrate * 16) / 2) * 32) / (baudrate * 16);
	wr(UARTIBRD, (div >> 5) & 0xffff);
	wr(UARTFBRD, (div & 0x1f));

	/* set 8n1 AFTER updating the baudrate, this will finally update the hw */
	wr(UARTLCR_H_WLEN1, UARTLCR_H_WLEN0 | UARTLCR_H_WLEN0 | UARTLCR_H_FEN);

	/* enable UART, RX and TX */
	wr(UARTCR, UARTCR_UARTEN | UARTCR_RXE | UARTCR_TXE);
}
