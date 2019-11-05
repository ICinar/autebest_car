#ifndef BSP_KLDD_H
#define BSP_KLDD_H

unsigned int bsp_kldd_gpio_write     (void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3);
unsigned int bsp_kldd_gpio_read     (void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3);
unsigned int bsp_kldd_gpio_pin_toggle (void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3);
unsigned int bsp_kldd_gpio_set     (void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3);
unsigned int bsp_kldd_gpio_reset     (void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3);

#endif
