#include <kernel.h>
#include <gpio.h>


void gpio_init(void){
    return;
}

void gpio_write(uint16_t nr, unsigned short value){
    GPIO[nr].odr = value;
}

int gpio_read(uint16_t nr){
    return GPIO[nr].idr;
}

void gpio_pin_toggle(uint16_t nb, unsigned int nr){
    if (nr >= NR_PINS){
        return;
    }
    if(GPIO[nb].odr & (1 << nr)){
        GPIO[nb].br = (1 << nr);
    }
    else{
        GPIO[nb].bs = ( 1<< nr);
    }
}

void gpio_set(uint16_t nr, unsigned short pin){
    GPIO[nr].bs = (1 << pin);
}

void gpio_reset(uint16_t nr, unsigned short pin){
    GPIO[nr].br = (1<<pin);
}
