/*
 * board_stuff.h
 *
 * Board specific setting for TMS570.
 *
 * azuepke, 2013-11-19: initial
 * azuepke, 2014-01-27: adapted from Cortex A15 BSP
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <board.h>

/* sci_uart.c */
void serial_init(unsigned int baud);

/* start.S */
void __board_halt(void) __noreturn;
void __board_idle(void);

/* board.c */
void board_init(void) __noreturn;

/* clocks.c */
void setup_pll(void);
void trim_LPO(void);
void setup_flash(void);
void periph_init(void);
void map_clocks(void);
void set_eclk(void);

/* pinmux.c */
void mux_init(void);

#endif

/* specific memory layout of the TMS570LS31xx board */
#define BOARD_ROM_PHYS		0x00000000
#define BOARD_ROM_SIZE		0x00200000	/* 2 MB */
#define BOARD_RAM_PHYS		0x08000000
#define BOARD_RAM_SIZE		0x00040000	/* 256 KB */

#define LOAD_ADDR	0x00000000	/* physical kernel load address */
/* NOTE: see also mem_detect() in board.c for assumptions on the memory layout */



/** Oscillator clock source */
#define OSC_FREQ		16000000
/** PLL 1 clock source */
#define PLL1_FREQ		180000000
/** LPO Low Freq Oscillator source */
#define LPO_LF_FREQ		80000
/** LPO High Freq Oscillator source */
#define LPO_HF_FREQ		10000000
/** PLL 2 clock source */
#define PLL2_FREQ		180000000


/** GCLK clock domain frequency: lockstep CPU cores */
#define GCLK_FREQ		180000000
/** HCLK clock domain frequency: system peripherals, flash, SRAM */
#define HCLK_FREQ		GCLK_FREQ

/** VCLK1 clock domain frequency: misc devices, SCI, ... */
#define VCLK1_FREQ		(HCLK_FREQ/2)
/** VCLK2 clock domain frequency: HET, HTU */
#define VCLK2_FREQ		(HCLK_FREQ/2)
/** VCLK2 clock domain frequency: EMIF, EMAC */
#define VCLK2_FREQ		(HCLK_FREQ/2)


/** VCLKA1 clock domain frequency: CAN controllers */
#define VCLKA1_FREQ		VCLK1_FREQ
/** VCLKA2 clock domain frequency: FlexRay */
#define VCLKA2_FREQ		VCLK1_FREQ
/** VCLKA3 clock domain frequency: reserved */
#define VCLKA3_FREQ		0
/** VCLKA4 clock domain frequency: EMAC */
#define VCLKA4_FREQ		VCLK1_FREQ

/** RTI clock domain frequency */
#define RTI_FREQ		VCLK1_FREQ
#define RTI_IRQ0		2
#define RTI_ADDR		0xfffffc00

/** VIM interrupt controller */
#define VIM_ADDR		0xfffffdec

/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-bigarm"
#define LD_OUTPUT_ARCH   "arm"

#endif
