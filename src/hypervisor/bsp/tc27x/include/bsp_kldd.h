/**
 * \file     bsp_kldd.h
 * \brief    Interface to the kernel level device driver (kldd) of this BSP.
 * \details
 * \date     22.04.2015
 * \author   easycore GmbH, 91058 Erlangen, Germany
 */

#if (!defined BSP_KLDD_H)
#define BSP_KLDD_H

unsigned int bsp_kldd_led_on     (void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);
unsigned int bsp_kldd_led_off    (void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);
unsigned int bsp_kldd_led_toggle (void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);


#endif

