/*
 * board_stuff.h
 *
 * Board specific setting for QEMU Tricore
 *
 * azuepke, 2014-10-24: initial
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <board.h>

/* board.c */
void __board_halt(void) __noreturn __tc_fastcall;
void board_init(void) __noreturn __tc_fastcall;
void board_setup_boot_csa(void *base, unsigned int num_contexts) __tc_fastcall;

/* stm_timer.c */
void stm_timer_handler(unsigned int irq);
void stm_timer_init_core(unsigned int core_id);
void stm_timer_init(unsigned int freq);

/* wdt.c */
void wdt_set_endinit(unsigned long wdt_base, int endinit);
void wdt_service(unsigned long wdt_base);
void wdt_disable(unsigned long wdt_base);

#endif

/* specific memory layout of the FIXME board */

/* I/O */
#define BOARD_IO_BASE			0xf0000000
#define BOARD_IO_SIZE			0x0fffff00	/* 256 MB */

/* shared flash */
#define BOARD_FLASH_BASE		0x80000000
#define BOARD_FLASH_SIZE		0x00200000	/* 2 MB */
#define BOARD_FLASH_UC_BASE		0xa0000000	/* uncached flash */
#define BOARD_FLASH_UC_SIZE		BOARD_FLASH_SIZE

/* shared SRAM */
#define BOARD_SRAM_BASE			0x90000000
#define BOARD_SRAM_SIZE			0x00008000	/* 32 KB */

/* per-core program scratch pad RAM */
#define BOARD_LOCAL_CRAM_BASE	0xc0000000	/* local view on CRAM */
#define BOARD_LOCAL_CRAM_SIZE	0x00008000	/* 32 KB */

#define BOARD_CPU0_CRAM_BASE	0x70100000
#define BOARD_CPU0_CRAM_SIZE	0x00006000	/* 24 KB */

#define BOARD_CPU1_CRAM_BASE	0x60100000
#define BOARD_CPU1_CRAM_SIZE	0x00008000	/* 32 KB */

#define BOARD_CPU2_CRAM_BASE	0x50100000
#define BOARD_CPU2_CRAM_SIZE	0x00008000	/* 32 KB */

/* per-core data scratch pad RAM */
#define BOARD_CPU0_DRAM_BASE	0x70000000
#define BOARD_CPU0_DRAM_SIZE	0x0001c000	/* 112 KB */

#define BOARD_CPU1_DRAM_BASE	0x60000000
#define BOARD_CPU1_DRAM_SIZE	0x0001e000	/* 120 KB */

#define BOARD_CPU2_DRAM_BASE	0x50000000
#define BOARD_CPU2_DRAM_SIZE	0x0001e000	/* 120 KB */

/* stack grows downwards */
#define BOOT_STACK_CPU0			(BOARD_CPU0_DRAM_BASE+BOARD_CPU0_DRAM_SIZE)
#define BOOT_CSA_ENTRIES		8
#define BOOT_CSA_CPU0			(BOARD_CPU0_DRAM_BASE+BOARD_CPU0_DRAM_SIZE-0x1000)

/* Timer on TSIM */
#define STM_TIMER_BASE			0xf0000000
#define STM_TIMER_COREOFFSET	0x1000
#define STM_TIMER_CLOCK			(1*1000*1000)
#define STM_TIMER_IRQ			2	/* see IRQ table in tc_irq_conf_tsim.c! */

/* Watchdogs on TSIM */
#define WDT_BASE				0xf0036000
/* Offsets of the four different watchdogs on TSIM */
#define WDT_S_OFFSET			0x0f0
#define WDT_CPU0_OFFSET			0x100
#define WDT_CPU1_OFFSET			0x10c
#define WDT_CPU2_OFFSET			0x118
/* watchdog reload value, watchdog counts up and triggers at 0xffff */
#define WDT_RELOAD_VALUE		1234


/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-tricore"
#define LD_OUTPUT_ARCH   "tricore"

#endif
