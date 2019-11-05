/*
 * uart.c
 *
 * 16550-like UART driver
 *
 * azuepke, 2013-03-24: initial
 * azuepke, 2015-06-29: adapted for LPC176x, use register-spread 4
 */

#include <arm_io.h>
#include <board.h>
#include <board_stuff.h>
#include <hv_error.h>

/*        baseaddr   irq   tx  rx cts dcd rts
 * UART0: 4000'C000  5=21  79  80              <<< connected to USB-IF on mbed
 * UART1: 4001'0000  6=22  47  48  46  45  44
 * UART2: 4009'8000  7=23  39  40
 * UART3: 4009'c000  8=24   7   6
 * NOTE: see page 307 AD0[7] AD0[6]
 */
#define PORT	0x4000c000
#define CLOCK	96*1000*1000

/* register accessors */
static inline uint32_t rd(unsigned int reg)
{
	return readl((volatile void *)(PORT + reg*4));
}

static inline void wr(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(PORT + reg*4), val);
}

unsigned int board_putc(int c)
{
	/* poll until transmitter is empty */
	while (!(rd(5) & 0x20)) {
		return E_OS_NOFUNC;
	}

	wr(0, c);
	return E_OK;
}

__init void serial_init(unsigned int baudrate)
{
	unsigned int div;

	/* standard sequence for NS16550 UART */

	/* disable all interrupts */
	wr(1, 0);

	/* enable DLAB, set baudrate */
	div = (CLOCK + (baudrate * 16) / 2) / (baudrate * 16);
	wr(3, 0x80);
	wr(0, div & 0xff);
	wr(1, div >> 8);

	/* 8n1 mode */
	wr(3, 0x03);
	/* enable FIFO, 14 byte threshold */
	wr(2, 0xc7);

	/* clear pending status bits */
	rd(5);
}
