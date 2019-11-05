/*
 * board_stuff.h
 *
 * Board specific setting for ARM i.MX6 SABRELITE board with Cortex A9.
 *
 * azuepke, 2013-09-11: initial ARM port
 * azuepke, 2013-11-19: refactored board.h
 * azuepke, 2016-04-14: ported from ZYNQ
 * azuepke, 2016-04-18: adapted to MPU
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <board.h>

/* imx6_uart.c */
void serial_init(unsigned int baud);

/* start.S */
void __board_halt(void) __noreturn;

unsigned int armv7_flush_dcache_all(void);

/* board.c */
void board_init(void) __noreturn;

#endif

/* specific memory layout of the i.MX6 SABRELITE board */
#define BOARD_ROM_PHYS		0x10008000
#define BOARD_ROM_SIZE		0x00100000	/* 1 MB */
/* external DRAM */
#define BOARD_RAM_PHYS		0x11000000
#define BOARD_RAM_SIZE		0x00100000	/* 1 MB */

/* 1st IO region, peripherals, e.g. serial */
#define BOARD_IO1_PHYS		0x02000000
#define BOARD_IO1_SIZE		0x00100000	/* 1 MB */

/* 2nd IO region, more peripherals */
#define BOARD_IO2_PHYS		0x02100000
#define BOARD_IO2_SIZE		0x00100000	/* 1 MB */

/* 3rd IO region, MPCore region, L2 cache controller */
#define BOARD_IO3_PHYS		0x00a00000
#define BOARD_IO3_SIZE		0x00100000	/* 1 MB */

/* region 0xfff00000 must be left out for the mapping of the vector pages! */

/** MPCore specific addresses */
#define MPCORE_SCU_BASE			0x00a00000
#define GIC_PERCPU_BASE			0x00a00100
#define MPCORE_GTIMER_BASE		0x00a00200
#define MPCORE_PTIMER_BASE		0x00a00600
#define GIC_DIST_BASE			0x00a01000

#define MPCORE_TIMER_CLOCK		396000000	/* 396 MHz */

/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-littlearm"
#define LD_OUTPUT_ARCH   "arm"

#endif
