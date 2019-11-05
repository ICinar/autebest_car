#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>

/* GPIOs A..K, each 0x400 bytes apart, starting at 4002'0000 */
struct gpio_regs {
	uint32_t moder;		/* mode: 00:input, 01:output, 10:alternate, 11:analog */
	uint32_t otyper;	/* type: 0:push-pull, 1:open-drain (1 bit per pin) */
	uint32_t ospeedr;	/* output speed: 00:low, 01:medium, 10:fast, 11:high */
	uint32_t pupdr;		/* pull-up/pull-down: 00: no, 01:pull-up, 10:pull-down */
	uint32_t idr;		/* input data (read-only, 1 bit per pin) */
	uint32_t odr;		/* output data (1 bit per pin) */
	uint16_t bs;		/* bit set (1 bit per pin) */
	uint16_t br;		/* bit reset (1 bit per pin) */
	uint32_t lckr;		/* lock register (1 bit per pin, write once) */
	uint32_t afr[2];	/* alternate function: 4 bit per pin */
	uint8_t reserved[0x400-0x28];
};
#define GPIO	((volatile struct gpio_regs *)(0x40020000))


#define OUTPORT GPIO[3]

#define NR_PINS 16

void gpio_init(void);

void gpio_write(uint16_t nr , unsigned short value);

int gpio_read(uint16_t nr);

void gpio_pin_toggle(uint16_t nb ,unsigned int nr);

void gpio_set(uint16_t nr,unsigned short pin);

void gpio_reset(uint16_t nr,unsigned short pin);

#endif
