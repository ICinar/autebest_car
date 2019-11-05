/*
 * board_stuff.h
 *
 * Board specific setting for QEMU PowerPC
 *
 * azuepke, 2014-06-03: initial
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <board.h>

/* ppc_timer.c */
void ppc_timer_handler(void);
void ppc_timer_init(unsigned int freq);

/* 8250_serial.c */
void serial_init(unsigned int baud);

/* start.S */
void ppc_set_tlb(uint32_t mas0, uint32_t mas1, uint32_t mas2, uint32_t mas3);

/* board.c */
void __board_halt(void) __noreturn;
void board_init(void) __noreturn;

/* mpic.c */
void mpic_send_stop(void);
void mpic_irq_init(void);
void mpic_enable(void);
unsigned int mpic_init_smp(void);

#endif

/* starting index of private TLB entries of this board
 * as we use (up to) 4 entries, this must be at least 12 */
#define BOARD_PRIVATE_TLBS	12

/* specific memory layout of the QEMU e500 board */
/* RAM starts at 0, but we leave the first 1 MB of RAM unused */
#define BOARD_ROM_PHYS		0x00100000
#define BOARD_ROM_SIZE		0x00100000	/* 1 MB */
#define BOARD_RAM_PHYS		0x01000000
#define BOARD_RAM_SIZE		0x00100000	/* 1 MB */

/* like Linux, we use a load offset of 32K relative to an 1M aligned region */
#define LOAD_ADDR	0x00100000	/* physical kernel load address */

#define BOOT_STACK	(BOARD_RAM_PHYS+BOARD_RAM_SIZE)	/* virtual address, grows downwards */
/* NOTE: see also mem_detect() in board.c for assumptions on the memory layout */

/* CCSRBAR IO region */
#define BOARD_CCSRBAR_PHYS	0xe0000000
#define BOARD_CCSRBAR_SIZE	0x00100000	/* 1 MB */
#define BOARD_CCSRBAR_VIRT	0xffe00000

/* addresses of the interrupt controller */
#define MPIC_BASE			BOARD_CCSRBAR_VIRT + 0x40000

#define PPC_TIMER_CLOCK		400000000	/* 400 MHz */
#define PPC_TIMER_IRQ		10

/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-powerpc"
#define LD_OUTPUT_ARCH   "powerpc"

#endif
