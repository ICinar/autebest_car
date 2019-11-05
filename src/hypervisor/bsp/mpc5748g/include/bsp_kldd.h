/**
 * \file     bsp_kldd.h
 * \brief    Interface to the kernel level device driver (kldd) of this BSP.
 * \details
 * \date     18.08.2015
 * \author   easycore GmbH, 91058 Erlangen, Germany
 */

#if (!defined BSP_KLDD_H)
#define BSP_KLDD_H

unsigned int bsp_kldd_led_on         (void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);
unsigned int bsp_kldd_led_off        (void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);
unsigned int bsp_kldd_led_toggle     (void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);
unsigned int bsp_kldd_led_display    (void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);

unsigned int bsp_kldd_reg_read8      (void *arg0, unsigned long address, unsigned long output, unsigned long arg3);
unsigned int bsp_kldd_reg_read32     (void *arg0, unsigned long address, unsigned long output, unsigned long arg3);

unsigned int bsp_kldd_reg_write8     (void *arg0, unsigned long address, unsigned long value, unsigned long arg3);
unsigned int bsp_kldd_reg_write16    (void *arg0, unsigned long address, unsigned long value, unsigned long arg3);
unsigned int bsp_kldd_reg_write32    (void *arg0, unsigned long address, unsigned long value, unsigned long arg3);


unsigned int bsp_kldd_reg_bit_clear32(void *arg0, unsigned long address, unsigned long mask, unsigned long arg3);
unsigned int bsp_kldd_reg_bit_set32  (void *arg0, unsigned long address, unsigned long mask, unsigned long arg3);

#endif

