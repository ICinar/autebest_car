/*
 * serial.c
 *
 * serial I/O (actually, only O) for MPC5646C
 *
 * tjordan, 2014-07-14: initial
 */

#include <ppc_io.h>
#include <board.h>
#include <board_stuff.h>
#include <hv_error.h>

/* we WILL use a real serial interface later.
 * but at the moment, debugging might be easier using a memory buffer instead.
 * BUFSIZE *must* be 2^x
 */
#define BUFSIZE  (32*1024)
char serial_ringbuffer[BUFSIZE];
unsigned int serial_ringbuffer_index;

unsigned int board_putc(int c)
{
	serial_ringbuffer[serial_ringbuffer_index] = (char) c;
	/* FIXME: non-reentrant */
	serial_ringbuffer_index = (serial_ringbuffer_index + 1) & (BUFSIZE - 1);

	return E_OK;
}

__init void serial_init(unsigned int baudrate __unused)
{
	/* nothing to do for now */
}
