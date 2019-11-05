
#include "bsp_kldd.h"
#include "leds.h"

unsigned int bsp_kldd_led_on
(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
    (void) arg0;
    (void) arg2;
    (void) arg3;

    led_on(arg1);

    return 0;
}

unsigned int bsp_kldd_led_off
(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
    (void) arg0;
    (void) arg2;
    (void) arg3;

    led_off(arg1);

    return 0;
}

unsigned int bsp_kldd_led_toggle
(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
    (void) arg0;
    (void) arg2;
    (void) arg3;

    led_toggle(arg1);

    return 0;
}
