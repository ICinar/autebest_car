/*
 * board_stuff.h
 *
 * Board specific setting for tc27x
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
void board_init(void)   __noreturn __tc_fastcall;
void board_setup_boot_csa(void *base, unsigned int num_contexts) __tc_fastcall;

#endif

/* specific memory layout of the tc27x board */

/* I/O */
#define BOARD_IO_BASE           0xf0000000
#define BOARD_IO_SIZE           0x0fffff00  /* 256 MB */

/* shared flash */
#define BOARD_FLASH_BASE        0x80000000
#define BOARD_FLASH_SIZE        0x00200000  /* 2 MB */
#define BOARD_FLASH_UC_BASE     0xa0000000  /* uncached flash */
#define BOARD_FLASH_UC_SIZE     BOARD_FLASH_SIZE

/* shared SRAM */
#define BOARD_SRAM_BASE         0x90000000
#define BOARD_SRAM_SIZE         0x00008000  /* 32 KB */

/* per-core program scratch pad RAM */
#define BOARD_LOCAL_CRAM_BASE   0xc0000000  /* local view on CRAM */
#define BOARD_LOCAL_CRAM_SIZE   0x00008000  /* 32 KB */

#define BOARD_CPU0_CRAM_BASE    0x70100000
#define BOARD_CPU0_CRAM_SIZE    0x00006000  /* 24 KB */

#define BOARD_CPU1_CRAM_BASE    0x60100000
#define BOARD_CPU1_CRAM_SIZE    0x00008000  /* 32 KB */

#define BOARD_CPU2_CRAM_BASE    0x50100000
#define BOARD_CPU2_CRAM_SIZE    0x00008000  /* 32 KB */

/* per-core data scratch pad RAM */
#define BOARD_CPU0_DRAM_BASE    0x70000000
#define BOARD_CPU0_DRAM_SIZE    0x0001c000  /* 112 KB */

#define BOARD_CPU1_DRAM_BASE    0x60000000
#define BOARD_CPU1_DRAM_SIZE    0x0001e000  /* 120 KB */

#define BOARD_CPU2_DRAM_BASE    0x50000000
#define BOARD_CPU2_DRAM_SIZE    0x0001e000  /* 120 KB */

/* Base addresses of the CSFR registers for CPU0, CPU1 and CPU2 */
#define CPU0_CSFR_BASE          0xF8810000u
#define CPU1_CSFR_BASE          0xF8830000u
#define CPU2_CSFR_BASE          0xF8850000u

#define BOOT_CSA_ENTRIES        8

/* stacks grow downwards */
#define BOOT_STACK_CPU0         (BOARD_CPU0_DRAM_BASE+BOARD_CPU0_DRAM_SIZE)
#define BOOT_CSA_CPU0           (BOARD_CPU0_DRAM_BASE+BOARD_CPU0_DRAM_SIZE-0x1000)

#define BOOT_STACK_CPU1         (BOARD_CPU1_DRAM_BASE+BOARD_CPU1_DRAM_SIZE)
#define BOOT_CSA_CPU1           (BOARD_CPU1_DRAM_BASE+BOARD_CPU1_DRAM_SIZE-0x1000)

#define BOOT_STACK_CPU2         (BOARD_CPU2_DRAM_BASE+BOARD_CPU2_DRAM_SIZE)
#define BOOT_CSA_CPU2           (BOARD_CPU2_DRAM_BASE+BOARD_CPU2_DRAM_SIZE-0x1000)

/* linker defines */
#define LD_OUTPUT_FORMAT "elf32-tricore"
#define LD_OUTPUT_ARCH   "tricore"

#endif
