/*
 * board_stuff.h
 *
 * Board specific setting for QEMU Versatile Express Cortex A15.
 *
 * azuepke, 2013-11-19: initial
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
unsigned int armv7_flush_dcache_all(void);
#ifdef SMP
extern unsigned int __boot_release;
#endif

/* board.c */
void board_init(void) __noreturn;

/* KLDD convenience functions */

/** "magic marker" for KLDD function */
extern unsigned int board_irq_kldd_magic;

/** trigger a software interrupt
 * used as KLDD - may be used by tests or as a way of inter-partition communication
 * parameters: arg0 - "magic marker" for KLDD
 *             arg1 - logical interrupt number, 0 to 3
 * returns non-null if interrupt was triggered
 */
unsigned int board_irq_trigger_kldd(void *arg0, unsigned long arg1,
                                    unsigned long arg2, unsigned long arg3);

/** clear a software interrupt after it has been reported
 * used as KLDD - may be used by tests or as a way of inter-partition communication
 * parameters: arg0 - "magic marker" for KLDD
 *             arg1 - logical interrupt number, 0 to 3
 * returns non-null if interrupt has been cleared
 */
unsigned int board_irq_clear_kldd(void *arg0, unsigned long arg1,
                                  unsigned long arg2, unsigned long arg3);

#endif

/* specific memory layout of the QEMU Cortex A15 board */
#define BOARD_ROM_PHYS		0x80000000
#define BOARD_ROM_SIZE		0x00100000	/* 1 MB */
#define BOARD_RAM_PHYS		0x81000000
#define BOARD_RAM_SIZE		0x00100000	/* 1 MB */
#define BOARD_RAM2_PHYS		0x82000000
#define BOARD_RAM2_SIZE		0x00100000	/* 1 MB */

/* like Linux, we use a load offset of 32K relative to an 1M aligned region */
#define LOAD_ADDR	0x80000000	/* physical kernel load address */
/* NOTE: see also mem_detect() in board.c for assumptions on the memory layout */

/** MPCore specific addresses */
#define GIC_DIST_BASE			0x2c001000
#define GIC_PERCPU_BASE			0x2c002000

#define SP804_TIMER_BASE		0x1c110000
#define SP804_TIMER_CLOCK		1000000	/* 1 MHz */
#define SP804_TIMER_IRQ			34

/** enforce QEMU SMP startup workaround */
#define QEMU_SMP_STARTUP 1

/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-littlearm"
#define LD_OUTPUT_ARCH   "arm"

#endif
