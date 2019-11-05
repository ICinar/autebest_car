/*
 * usart.c
 *
 * UART driver for USART on STM32F4-series
 *
 * azuepke, 2015-07-01: initial
 */

#include <arm_io.h>
#include <board.h>
#include <board_stuff.h>
#include <hv_error.h>
#include <arm_cr.h>

/*
 * See the "Universal synchronous asynchronous receiver transmitter (USART)"
 * chapter in the STM reference manual, page 953.
 *
 * Base addresses:
 * - 4001'1000  USART1
 * - 4000'4400  USART2
 * - 4000'4800  USART3
 * - 4000'4c00  USART4
 * - 4000'5000  USART5
 * - 4001'1400  USART6 <--- available on the stm32-discovery base board
 */
#define BASE	0x40011400	/* USART6 */
#define CLOCK	84*1000*1000

/* status register */
#define SR		_REG32(BASE + 0x00)
#define SR_CTS		(1u << 9)
#define SR_LBD		(1u << 8)
#define SR_TXE		(1u << 7)
#define SR_TC		(1u << 6)
#define SR_RXNE		(1u << 5)
#define SR_IDLE		(1u << 4)
#define SR_ORE		(1u << 3)
#define SR_NF		(1u << 2)
#define SR_FE		(1u << 1)
#define SR_PE		(1u << 0)

/* data register */
#define DR		_REG32(BASE + 0x04)

/* baud rate divider */
#define BRR		_REG32(BASE + 0x08)

/* control register 1: parity and interrupts */
#define CR1		_REG32(BASE + 0x0c)
#define CR1_OVER8	(1u << 15)	/* 8x oversampling */
#define CR1_UE		(1u << 13)	/* enable */
#define CR1_M		(1u << 12)	/* word length */
#define CR1_WAKE	(1u << 11)	/* wake mode */
#define CR1_PCE		(1u << 10)	/* parity enable */
#define CR1_PS		(1u << 9)	/* parity odd */
#define CR1_PEIE	(1u << 8)	/* PE interrupt enable */
#define CR1_TXEIE	(1u << 7)	/* TXE interrupt enable */
#define CR1_TCIE	(1u << 6)	/* TX interrupt enable */
#define CR1_RXNEIE	(1u << 5)	/* RXNE interrupt enable */
#define CR1_IDLEIE	(1u << 4)	/* IDLE interrupt enable */
#define CR1_TE		(1u << 3)	/* transmitter enable */
#define CR1_RE		(1u << 2)	/* receiver enable */
#define CR1_RWU		(1u << 1)	/* receiver wakeup */
#define CR1_SBK		(1u << 0)	/* send break */

/* control register 2: LIN mode, stop bits, clock phase */
#define CR2		_REG32(BASE + 0x10)
#define CR2_LINEN	(1u << 14)	/* LIN mode */
#define CR2_STOP1	(1u << 13)	/* STOP bits: 00==1 stop bit */
#define CR2_STOP0	(1u << 12)
#define CR2_CLKEN	(1u << 11)	/* SCLK enable */
#define CR2_CPHA	(1u << 11)	/* clock phase */

/* control register 3: flow control, DMA */
#define CR3		_REG32(BASE + 0x14)
#define CR3_ONEBIT	(1u << 11)	/* one-bit sampling */
#define CR3_CTSIE	(1u << 10)	/* CTS interrupt enable */
#define CR3_CTSE	(1u << 9)	/* CTS enable */
#define CR3_RTSE	(1u << 8)	/* RTS enable */
#define CR3_DMAT	(1u << 7)	/* DMA enable transmitter */
#define CR3_DMAR	(1u << 6)	/* DMA enable receiver */
#define CR3_SCEN	(1u << 5)	/* Smartcard mode enable */
#define CR3_NACK	(1u << 4)	/* Smartcard NACK enable */
#define CR3_HDSEL	(1u << 3)	/* half-duplex selection */
#define CR3_IRLP	(1u << 2)	/* IrDA low-power */
#define CR3_IREN	(1u << 1)	/* IrDA mode enable */
#define CR3_EIE		(1u << 0)	/* error interrupt enable */

/* guard time and prescaler */
#define GTPR	_REG32(BASE + 0x18)

unsigned int board_putc(int c)
{
	/* poll until previous transmission is complete */
	while ((SR & SR_TC) == 0) {
		return E_OS_NOFUNC;
	}

	DR = c & 0xff;
	return E_OK;
}

__init void serial_init(unsigned int baudrate)
{
	unsigned int div;
	unsigned int tmp;

	/* configure for 8n1 mode */
	CR1 &= ~CR1_UE;
	CR1 &= ~CR1_OVER8;
	CR1 &= ~CR1_M;
	CR1 &= ~CR1_PCE;
	/* disable all interrupts */
	CR1 &= ~(CR1_PEIE | CR1_TXEIE | CR1_TCIE | CR1_RXNEIE | CR1_IDLEIE);

	CR2 &= ~(CR2_LINEN | CR2_STOP1 | CR2_STOP0 | CR2_CLKEN);
	CR3 &= ~(CR3_ONEBIT | CR3_CTSIE | CR3_CTSE | CR3_RTSE |
	         CR3_DMAT | CR3_DMAR | CR3_SCEN | CR3_HDSEL | CR3_IREN | CR3_EIE);

	/* Determine the integer part */
	div = (25 * CLOCK) / (4 * baudrate);
	tmp = (div / 100) << 4;
	/* Determine the fractional part */
	div = div - (100 * (tmp >> 4));
	tmp |= (((div * 16) + 50) / 100) & 0xf;
	BRR = tmp;

	GTPR = (GTPR & 0xffff0000) | 0x0101;

	/* enable transmitter. this sends an idle frame */
	CR1 |= CR1_TE;
	CR1 |= CR1_UE;
}
