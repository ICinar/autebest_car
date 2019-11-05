/*
 * serial.c
 *
 * serial I/O (actually, only O) for MPC5646C
 *
 * tjordan, 2014-07-14: initial
 */
#include <board_stuff.h>
#include <test_support.h>

/* we WILL use a real serial interface later.
 * but at the moment, debugging might be easier using a memory buffer instead.
 * BUFSIZE *must* be 2^x
 */
#define BUFSIZE  (8*1024)
char serial_ringbuffer[BUFSIZE];
unsigned int serial_ringbuffer_index;

void serial_put(const char * c)
{
	char ch;
	while ((ch = *c) != 0)
	{
		serial_ringbuffer[serial_ringbuffer_index] = ch;
		serial_ringbuffer_index = (serial_ringbuffer_index + 1) & (BUFSIZE - 1);
		c++;
	}
}

void serial_init(unsigned int baudrate)
{
	/* nothing to do for now */
	(void) baudrate;
}
