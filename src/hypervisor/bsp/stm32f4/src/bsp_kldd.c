//#include <kernel.h>
#include "bsp_kldd.h"
#include "gpio.h"

unsigned int bsp_kldd_gpio_write
(void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3)
{
    (void) arg0;
    (void) arg3;

    gpio_write(arg2, arg1);

    return 0;
}

unsigned int bsp_kldd_gpio_read
(void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3)
{
    (void) arg0;
    (void) arg1;
    (void) arg3;

    return gpio_read(arg2);
}

unsigned int bsp_kldd_gpio_pin_toggle
(void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3)
{
    (void) arg0;
    (void) arg3;
	
    gpio_pin_toggle(arg2, arg1);

    return 0;
}


unsigned int bsp_kldd_gpio_set
(void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3)
{
    (void) arg0;
    (void) arg3;

    gpio_set(arg2, arg1);

    return 0;
}

unsigned int bsp_kldd_gpio_reset
(void *arg0, unsigned long arg1, unsigned int arg2, unsigned long arg3)
{
    (void) arg0;
    (void) arg3;

    gpio_reset(arg2, arg1);
    return 0;
}
