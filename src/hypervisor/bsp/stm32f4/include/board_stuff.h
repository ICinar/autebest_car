/*
 * board_stuff.h
 *
 * Board specific setting for STM32F4 with Cortex-M4
 *
 * azuepke, 2015-07-91: initial
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <board.h>

/* usart.c */
void serial_init(unsigned int baud);

/* start.S */
void __board_halt(void) __noreturn;

/* board.c */
void board_init(void) __noreturn;

/* clocks.c */
void init_clocks(void);

#endif

/* specific memory layout of the Cortex M3/M4 */
#define BOARD_ROM_PHYS		0x08000000
#define BOARD_ROM_SIZE		0x00100000	/* 1M */
#define BOARD_RAM_PHYS		0x20000000
#define BOARD_RAM_SIZE		0x0001c000	/* 112K */

#define LOAD_ADDR	0x08000000	/* physical kernel load address */

/* IRQs (excluding the 16 exception vectors) */
#define NUM_IRQS	82

/** NVIC */
#define NVIC_BASE	0xe000e000
#define NVIC_TIMER_CLOCK	168*1000*1000

/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-littlearm"
#define LD_OUTPUT_ARCH   "arm"

#endif
