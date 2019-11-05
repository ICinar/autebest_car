/*
 * board_stuff.h
 *
 * Board specific setting for NXP LPC1786 with Cortex-M3
 *
 * azuepke, 2015-06-29: initial
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <board.h>

/* pl011_uart.c */
void serial_init(unsigned int baud);

/* start.S */
void __board_halt(void) __noreturn;

/* board.c */
void board_init(void) __noreturn;

#endif

/* specific memory layout of the Cortex M3/M4 */
#define BOARD_ROM_PHYS		0x00000000
#define BOARD_ROM_SIZE		0x00080000	/* 512K */
#define BOARD_RAM_PHYS		0x10000000
#define BOARD_RAM_SIZE		0x00008000	/* 32K */

#define LOAD_ADDR	0x00000000	/* physical kernel load address */

/* IRQs (excluding the 16 exception vectors) */
#define NUM_IRQS	35

/** NVIC */
#define NVIC_BASE	0xe000e000
#define NVIC_TIMER_CLOCK	96*1000*1000

/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-littlearm"
#define LD_OUTPUT_ARCH   "arm"

#endif
