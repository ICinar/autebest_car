/**
 * \file     serial.c
 * \brief    Implementation of the serial interface.
 * \details  This module implements a simulated serial interface. The output is
 *           sent into internal buffers which can be read and dumped using the
 *           debugger.
 *
 *           Each CPU has its own output buffer as shown in the following table.
 *           This is the buffer you want to dump in order to read the output of
 *           that CPU. You can change the buffer size by changing the value of
 *           the respective symbol.
 *
 *           \code
 *           +-------------------------------------+
 *           | CPU  | buffer name    | buffer size |
 *           +-------------------------------------+
 *           | CPU0 | serial_buffer0 | BUFSIZE0    |
 *           | CPU1 | serial_buffer1 | BUFSIZE1    |
 *           | CPU2 | serial_buffer2 | BUFSIZE2    |
 *           +-------------------------------------+
 *           \endcode
 *
 * \date     14.07.2014
 * \author   Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author   easycore GmbH, 91058 Erlangen, Germany
 * \version
 * \par      License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * \par      File history
 * - tjordan, 2014-07-14: initial
 * - lberaru, 2015-10-07: rewritten to support multicore.
 * 
 * \copyright Copyright 2015 easycore GmbH, 91058 Erlangen, Germany.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include <ppc_io.h>
#include <board.h>
#include <board_stuff.h>
#include <hv_error.h>
#include <kernel.h>

/*==================[macros]==================================================*/

/* Buffer sizes for each CPU */

#if (defined SMP)
#define      BUFSIZE0  (12*1024)
#define      BUFSIZE1  (12*1024)
#define      BUFSIZE2  (12*1024)
#else
#define      BUFSIZE0  (32*1024)
#endif

/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

/* Output buffers for each CPU and the corresponding buffer indexes. */

static char         serial_buffer0[BUFSIZE0];
static unsigned int serial_buffer0_index;

#if (defined SMP)
static char         serial_buffer1[BUFSIZE1] __section_bss_core(1);
static unsigned int serial_buffer1_index     __section_bss_core(1);
static char         serial_buffer2[BUFSIZE2] __section_bss_core(2);
static unsigned int serial_buffer2_index     __section_bss_core(2);
#endif


/*==================[external function definitions]===========================*/

/*------------------[Initialize the serial driver]----------------------------*/

__init void serial_init(unsigned int baudrate __unused)
{
    serial_buffer0_index = 0;
#if (defined SMP)
    serial_buffer1_index = 0;
    serial_buffer2_index = 0;
#endif
}

/*------------------[Character output]----------------------------------------*/

unsigned int board_putc(int c)
{
    /* using a function instead of a macro at this place
     * has strange effects on the timers, which is why
     * we use ppc_get_spr instead of arch_cpu_id */

    /* every CPU writes into its own buffer */
    switch (ppc_get_spr(SPR_PIR))
    {
        case CPU0:
            if (serial_buffer0_index < BUFSIZE0)
            {
                serial_buffer0[serial_buffer0_index] = (char) c;
                serial_buffer0_index++;
            }
            break;

#if (defined SMP)
        case CPU1:
            if (serial_buffer1_index < BUFSIZE1)
            {
                serial_buffer1[serial_buffer1_index] = (char) c;
                serial_buffer1_index++;
            }
            break;

        case CPU2:
            if (serial_buffer2_index < BUFSIZE2)
            {
                serial_buffer2[serial_buffer2_index] = (char) c;
                serial_buffer2_index++;
            }
            break;
#endif
        default:
            break;
    }

    return E_OK;
}


/*==================[internal function definitions]===========================*/
/*==================[end of file]=============================================*/

