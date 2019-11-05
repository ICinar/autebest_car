/*
 * 8250_serial.c
 *
 * x86 8250 serial UART
 *
 * azuepke, 2013-03-24: initial
 * azuepke, 2013-11-22: PowerPC version
 * azuepke, 2014-06-03: adapted to MPU
 */

#include <ppc_io.h>
#include <board.h>
#include <board_stuff.h>
#include <hv_error.h>


/* COM1: 0x4500, COM2: 0x4600 */
#define PORT (BOARD_CCSRBAR_VIRT + 0x04500)
#define CLOCK 1843200

#define uart_read(reg)			readb((uint8_t*)(PORT + (reg)))
#define uart_write(reg, val)	writeb((uint8_t*)(PORT + (reg)), (val))

unsigned int board_putc(int c)
{
	/* poll until transmitter is empty */
	while (!(uart_read(5) & 0x20)) {
		return E_OS_NOFUNC;
	}

	uart_write(0, c);
	return E_OK;
}

__init void serial_init(unsigned int baudrate)
{
	unsigned int div;

	/* disable all interrupts */
	uart_write(1, 0);

	/* enable DLAB, set baudrate */
	div = (CLOCK + (baudrate * 16) / 2) / (baudrate * 16);
	uart_write(3, 0x80);
	uart_write(0, div & 0xff);
	uart_write(1, div >> 8);

	/* 8n1 mode */
	uart_write(3, 0x03);
	/* enable FIFO, 14 byte threshold */
	uart_write(2, 0xc7);

	/* clear pending status bits */
	uart_read(5);
}
