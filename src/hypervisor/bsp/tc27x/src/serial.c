/**
 * \file  serial.c
 * \brief Initialization of the serial interface.
 *
 * \author Liviu Beraru
 * \date   12.02.2015
 */

#include <stddef.h>
#include <board_stuff.h>
#include <hv_error.h>
#include "serial.h"
#include "wdt.h"
#include "stm_timer.h"


/*============[internal function declarations]================================*/
#if (defined USE_REAL_SERIAL_HARDWARE)

static void serial_swap_buffers(void);

#endif

/*============[internal data declarations]====================================*/
#if (! defined USE_REAL_SERIAL_HARDWARE)
#define BUFSIZE  (10*1024)

static char serial_buffer1[BUFSIZE];
static char serial_buffer2[BUFSIZE];
static char serial_buffer3[BUFSIZE];

static unsigned int serial_buffer_index1;
static unsigned int serial_buffer_index2;
static unsigned int serial_buffer_index3;

#else

/**
 * \brief Size of the serial buffer.
 * \details This is how much bytes we need to buffer so that we don't loose any
 * output.
 */
#define SERIAL_BUFFER_SIZE (12 * 1024)

/** Internal data of the serial driver. */
static struct serial_state
{
    /**
     * Indicates if the driver has completed tranmission of a buffer.
     * 0 means the driver is still trasmitting a buffer,
     * 1 means the driver has completed and buffers can be swapped. */
    unsigned int  output_ready;

    /** Current index in the store buffer. */
    unsigned int  store_index;

    /** Current index in the output buffer. */
    unsigned int  output_index;

    /** How many characters have been lost (not transmitted). */
    unsigned int  lost_characters;

    /** Store buffer. This is where characters are buffered as long as the
     * driver is busy with the output buffer. */
    unsigned char *store_buffer;

    /** Output buffer. This buffer is being transmitted. When the entire buffer
     * has been transmitted, the output buffer and the store buffer are swapped. */
    unsigned char *output_buffer;

    /** First buffer. This is either output or store buffer. */
    unsigned char buffer1[SERIAL_BUFFER_SIZE];

    /** Second buffer. This is either output or store buffer. */
    unsigned char buffer2[SERIAL_BUFFER_SIZE];
} serial_state;

#endif /* USE_REAL_SERIAL_HARDWARE */

/*============[public function definitions]===================================*/


#if (defined USE_REAL_SERIAL_HARDWARE)
/*------ Using real serial hardware, output goes to serial line ASC0 ---------*/

/*------------[initialization routine]----------------------------------------*/

__init void serial_init(void)
{
    /* Initialize internal data */
    serial_state.store_index     = 0;
    serial_state.output_index    = 0;
    serial_state.store_buffer    = serial_state.buffer1;
    serial_state.output_buffer   = serial_state.buffer2;
    serial_state.output_ready    = 1;
    serial_state.lost_characters = 0;

    /* Set pin P15.2 to "output" and "high".
     * This pin is used by ASC0 for output. */

    PORT15_IOCR0 = (PIN_OUT_ALT2 << IOCR0_PC2_OFF);
    PORT15_OMR   |= (1 << OMR_PS2_OFF);

    /* Enable asclin0 by setting the disable request bit to 0.
     * The default value of this register is 3 which means the asc0 is disabled. */
    ASC0_CLC = 0u;
    (void)ASC0_CLC;

    /* Select no clock for ASC0 so we can program it */
    ASC0_CSR = ASC0_CSR_NO_SOURCE;

    /* put ASC0 in initialize mode */
    ASC0_FRAMECON = ASC0_FRAMECON_INITVAL_INIT;

    /* data format: 8 data bits, 1 stop bit (8N1) */
    ASC0_DATCON = ASC0_DATCON_INTIVAL;

    /* configure TX FIFO */
    ASC0_TXFIFOCON = ASC0_TXFIFOCON_INITVAL;

    /* configure prescaler and oversampling */
    ASC0_BITCON = ASC0_BITCON_INITVAL;

    /* configure baud rate */
    ASC0_BRG = ASC0_BRG_INITVAL;

    /* put ASC0 in ASC mode */
    ASC0_FRAMECON = ASC0_FRAMECON_INITVAL_ASC;

    /* Select fCLK as clock input for ASC0.
     * This practically turns on the ASC0. */
    ASC0_CSR = ASC0_CSR_CLK;

    ASC0_FLAGSENABLE = ASC0_FLAGSENABLE_INITVAL;

    /* enable transmission interrupt of ASC0 */
    board_irq_enable(ASC0_IRQ);

    /* Start the transmission fifo or else the tx fifo will never be ready. */
    ASC0_TXFIFO_SETREADY();
}

/*------------[transmit one character]----------------------------------------*/

unsigned int board_putc(int c)
{
    /* Store the new character in the store buffer.
     * If the store buffer is full, increment the number of bytes we lost so
     * far. This is useful when debugging.
     */
    if (serial_state.store_index < SERIAL_BUFFER_SIZE)
    {
        serial_state.store_buffer[serial_state.store_index] = c;
        serial_state.store_index++;
    }
    else
    {
        serial_state.lost_characters++;
    }

    /* If the output buffer has been transmitted, swap the buffers. Now we
     * buffer new characters in the old output buffer and we transmit what we
     * have buffered so far.
     */
    if (serial_state.output_ready == 1)
    {
        serial_swap_buffers();
        serial_state.output_ready = 0;
        /* Tranmit the first character in the output buffer (which was our store
         * buffer so far). When the first character has been transmitted, the
         * machine issues an interrupt and the interrupt handler is called. The
         * interrupt handler will continue transmitting the output buffer until
         * the entire buffer has been transmitted (or until it finds a zero).
         * Then it will set output_ready to 1 and we will swap buffers again. */
        PUTCHAR(serial_state.output_buffer[0]);
    }

    return E_OK;
}

/*------------[interrupt handler]---------------------------------------------*/

/* This interrupt handler is called whenever a character has been transmitted.
 * If the output buffer has more bytes to send (its value is not zero), we send
 * the next byte, otherwise we set output_ready to 1 meaning we are ready. This
 * will make board_putc swap the store and output buffers.
 */
void serial_irq_handler(unsigned int value __attribute__((unused)))
{
    unsigned int output = 0;

    /* Read the current byte to send. If we have reached the end of the output
     * buffer, then we tell we are ready. This will make board_putc swap the
     * buffers. */
    if (serial_state.output_index < SERIAL_BUFFER_SIZE)
    {
        output = serial_state.output_buffer[serial_state.output_index];
        /* Delete the curent byte. This helps us in future runs to detect the
         * end of the output buffer. */
        serial_state.output_buffer[serial_state.output_index] = 0;
        serial_state.output_index++;

        if (output != 0)
        {
            PUTCHAR(output);
        }
        else
        {
            /* The current byte is zero which means that we either have
             * previously deleted it or it was never written. This means we are
             * ready. */
            serial_state.output_ready = 1;
        }
    }
    else
    {
        serial_state.output_ready = 1;
    }
}

#else
/*------ Using fake serial hardware, output goes into arrays -----------------*/

__init void serial_init(void)
{
    serial_buffer_index1 = 0u;
    serial_buffer_index2 = 0u;
    serial_buffer_index3 = 0u;
}

unsigned int board_putc(int c)
{
    unsigned int *index  = NULL;
    char         *buffer = NULL;
    unsigned int  can_put = 0;

    if (serial_buffer_index1 < BUFSIZE)
    {
        index = &serial_buffer_index1;
        buffer = serial_buffer1;
        can_put = 1;
    }
    else if (serial_buffer_index2 < BUFSIZE)
    {
        index = &serial_buffer_index2;
        buffer = serial_buffer2;
        can_put = 1;
    }
    else if (serial_buffer_index3 < BUFSIZE)
    {
        index = &serial_buffer_index3;
        buffer = serial_buffer3;
        can_put = 1;
    }

    if (can_put == 1)
    {
        buffer[*index] = (char) c;
        *index = *index + 1;
    }
    
    return E_OK;
}

void serial_irq_handler(unsigned int value __attribute__((unused)))
{
    /* only for linking purposes when not using real serial hardware */
}

#endif /* USE_REAL_SERIAL_HARDWARE */


/*============[internal function definitions]=================================*/

#if (defined USE_REAL_SERIAL_HARDWARE)
/* Swap the store and the output buffer.
 * This is done when the driver has completed the transmission of the output
 * buffer.
 */
static void serial_swap_buffers(void)
{
    unsigned char *p = serial_state.output_buffer;
    serial_state.output_buffer = serial_state.store_buffer;
    serial_state.store_buffer = p;

    serial_state.store_index  = 0;
    /* board_putc has already transmitted the first byte, so the interrupt
     * handler must continue transmission with the second byte. */
    serial_state.output_index = 1;
}
#endif
/*============[end of file]===================================================*/

