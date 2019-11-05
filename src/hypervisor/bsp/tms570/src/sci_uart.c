/*
 * sci_uart.c
 *
 * TMS570 SCI serial UART driver.
 *
 * azuepke, 2013-11-15: initial
 * azuepke, 2014-01-27: use sciregs.h
 */

#include <board.h>
#include <board_stuff.h>
#include <sciregs.h>
#include <hv_error.h>


#define SCIREGS SCIREG1
#define CLOCK   VCLK1_FREQ


unsigned int board_putc(int c)
{
    /* poll until TX FIFO has space for a character */
    while ((SCIREGS->FLR & SCI_INT_TX) == 0) {
        return E_OS_NOFUNC;
    }

    SCIREGS->TD = c;
    return E_OK;
}

__init void serial_init(unsigned int baudrate)
{
    /* bring SCI out of reset */
    SCIREGS->GCR0 = 0U;
    SCIREGS->GCR0 = 1U;

    /* Disable all interrupts */
    SCIREGS->CLEARINT    = 0xFFFFFFFFU;
    SCIREGS->CLEARINTLVL = 0xFFFFFFFFU;

    /* global control 1 */
    SCIREGS->GCR1 = (1U << 25U)  /* enable transmit */
                  | (1U << 24U)  /* enable receive */
                  | (1U << 5U)   /* internal clock (device has no clock pin) */
                  | ((1U-1U) << 4U)  /* number of stop bits */
                  | (0U << 3U)  /* even parity, otherwise odd */
                  | (0U << 2U)  /* enable parity */
                  | (1U << 1U);  /* asynchronous timing mode */



    /* set baudrate */
    SCIREGS->BRS = CLOCK / (16 * baudrate + 1);  /* baudrate */

    /* transmission length */
    SCIREGS->FORMAT = 8U - 1U;  /* length */

    /* set SCI pins functional mode */
    SCIREGS->PIO0 = (1U << 2U)  /* tx pin */
                 | (1U << 1U)  /* rx pin */
                 | (0U);  /* clk pin */

    /* set SCI pins default output value */
    SCIREGS->PIO3 = (0U << 2U)  /* tx pin */
                  | (0U << 1U)  /* rx pin */
                  | (0U);  /* clk pin */

    /* set SCI pins output direction */
    SCIREGS->PIO1 = (0U << 2U)  /* tx pin */
                 | (0U << 1U)  /* rx pin */
                 | (0U);  /* clk pin */

    /* set SCI pins open drain enable */
    SCIREGS->PIO6 = (0U << 2U)  /* tx pin */
                 | (0U << 1U)  /* rx pin */
                 | (0U);  /* clk pin */

    /* set SCI pins pullup/pulldown enable */
    SCIREGS->PIO7 = (0U << 2U)  /* tx pin */
                | (0U << 1U)  /* rx pin */
                | (0U);  /* clk pin */

    /* set SCI pins pullup/pulldown select */
    SCIREGS->PIO8 = (1U << 2U)  /* tx pin */
                 | (1U << 1U)  /* rx pin */
                 | (1U);  /* clk pin */

    /* set interrupt level */
    SCIREGS->SETINTLVL = (0U << 26U)  /* Framing error */
                       | (0U << 25U)  /* Overrun error */
                       | (0U << 24U)  /* Parity error */
                       | (0U << 9U)  /* Receive */
                       | (0U << 8U)  /* Transmit */
                       | (0U << 1U)  /* Wakeup */
                       | (0U);  /* Break detect */

    /* set interrupt enable */
    SCIREGS->SETINT = (0U << 26U)  /* Framing error */
                    | (0U << 25U)  /* Overrun error */
                    | (0U << 24U)  /* Parity error */
                    | (0U << 9U)  /* Receive */
                    | (0U << 1U)  /* Wakeup */
                    | (0U);  /* Break detect */

    /* Finaly start SCILIN */
    SCIREGS->GCR1 |= (1U << 7U);
}

