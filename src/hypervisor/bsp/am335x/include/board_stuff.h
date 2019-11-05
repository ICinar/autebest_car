/*
 * board_stuff.h
 *
 * Board specific setting for Beaglebone Black with ARM Cortex A8.
 *
 * azuepke, 2013-12-23: Beaglebone Black port
 * azuepke, 2014-05-06: adapted to MPU
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <board.h>

/* ti_uart.c */
void serial_init(unsigned int baud);

/* dmtimer.c */
void dmtimer_init(unsigned int freq);
void dmtimer_handler(unsigned int irq);

/* start.S */
void __board_halt(void) __noreturn;

unsigned int armv7_flush_dcache_all(void);

/* board.c */
void board_init(void) __noreturn;

/* intc.c */
void intc_init(void);

#endif

/* specific memory layout of the BBB board */
#define BOARD_ROM_PHYS		0x80000000
#define BOARD_ROM_SIZE		0x00100000	/* 1 MB */
/* external DRAM */
#define BOARD_RAM_PHYS		0x81000000
#define BOARD_RAM_SIZE		0x00100000	/* 1 MB */
/* internal SRAM */
//#define BOARD_RAM_PHYS		0x402f0400
//#define BOARD_RAM_SIZE		0x0000fc00	/* 63K */
/* OCM SRAM */
//#define BOARD_RAM_PHYS		0x40300000
//#define BOARD_RAM_SIZE		0x00010000	/* 64K */

/* like Linux, we use a load offset of 32K relative to an 1M aligned region */
#define LOAD_ADDR	0x80008000	/* physical kernel load address */
/* NOTE: see also mem_detect() in board.c for assumptions on the memory layout */


/* 1st IO region, peripherals, e.g. serial */
#define BOARD_IO1_PHYS		0x44e00000
#define BOARD_IO1_SIZE		0x00100000	/* 1 MB */

/* 2nd IO region, L4 peripherals, DMTIMER2, UART1 */
#define BOARD_IO2_PHYS		0x48000000
#define BOARD_IO2_SIZE		0x00100000	/* 1 MB */

/* 3rd IO region, interrupt controller */
#define BOARD_IO3_PHYS		0x48200000
#define BOARD_IO3_SIZE		0x00100000	/* 1 MB */

/* region 0xfff00000 must be left out for the mapping of the vector pages! */

/* SoC specific addresses */
/* UART0 */
#define UART_BASE			(BOARD_IO1_PHYS + 0x9000)
#define UART_CLOCK			48000000	/* 48 MHz */

/* DMTIMER0 */
#define DMTIMER_BASE		(BOARD_IO1_PHYS + 0x5000)
#define DMTIMER_IRQ			66
#define DMTIMER_CLOCK		32768	/* 32 kHz */
//FIXME: better use DMTimer2? -- 0x48040000, IRQ 68, 25 MHz(?) clock

/* INTC */
#define INTC_BASE			(BOARD_IO3_PHYS + 0x0000)


/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-littlearm"
#define LD_OUTPUT_ARCH   "arm"

#endif
