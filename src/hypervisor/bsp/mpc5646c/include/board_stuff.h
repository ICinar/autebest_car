/*
 * board_stuff.h
 *
 * Board specific setting for MPC5646C
 *
 * tjordan, 2014-07-14: initial
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <board.h>

/* ppc_timer.c */
void ppc_timer_handler(void);
void ppc_timer_init(unsigned int freq);

/* io.c */
void serial_init(unsigned int baud);

/* start.S */
void ppc_set_tlb(uint32_t mas0, uint32_t mas1, uint32_t mas2, uint32_t mas3);

/* cache.c */
void board_cache_init(void);

/* board.c */
void __board_halt(void) __noreturn;
void board_init(void) __noreturn;
void board_init_clocks(void);

/* intc.c */
void board_intc_init(void);

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

/* starting index of private TLB entries of this board
 * as we use 4 entries, this must be at least 12 */
#define BOARD_PRIVATE_TLBS	12

/* specific memory layout of the MPC5646C */
#define BOARD_ROM_PHYS		0x00000000
#define BOARD_ROM_SIZE		0x00300000	/* 3 MB */
#define BOARD_ROM_TLBSIZE	TLB_SIZE_4M

#define BOARD_RAM_PHYS		0x40000000
#define BOARD_RAM_SIZE		0x00040000	/* 256 KB */
#define BOARD_RAM_TLBSIZE	TLB_SIZE_256K

#define BOOT_STACK	(BOARD_RAM_PHYS+BOARD_RAM_SIZE)	/* virtual address, grows downwards */

/* peripherals IO region(s):  */
#define BOARD_PER1_PHYS		0xC0000000
#define BOARD_PER1_SIZE		0x04000000
#define BOARD_PER1_TLBSIZE	TLB_SIZE_64M

#define BOARD_PER0_PHYS		0xFFE00000
#define BOARD_PER0_SIZE		0x00200000
#define BOARD_PER0_TLBSIZE	TLB_SIZE_2M

/* PLL setup */
#define BOARD_CLOCK_MHZ		120

#if (BOARD_CLOCK_MHZ == 120)
/* 120 MHz system clock */
/* see Table 6-15. FMPLL lookup table in MPC5646CRM.pdf */
/* these the values are fed directly into the register, logical divisors may differ */
#define BOARD_IDF_VALUE		1u /* i.e. divide by 2 */
#define BOARD_ODF_VALUE		1u /* i.e. divide by 4 */
#define BOARD_NDIV_VALUE	60u
/* input is 16 MHz, output is ( (16 * 60) / (2 * 4) ) -> 120 MHz */

/* clock dividers */
/* z0 core, flash and FEC need a /2 divider if the system clock is faster than 80 MHz */
#define BOARD_HIGHSPEED_DIVIDER	1u /* 1: divider active, 0: divider disabled */
/* divider for peripheral set 1. frequency must be below 32MHz */
#define BOARD_DC0_VALUE		4 /* logical value, i.e. divide by 4 */
/* divider for peripheral set 2. frequency must be below 64MHz */
#define BOARD_DC1_VALUE		2 /* logical value, i.e. divide by 2 */
/* divider for peripheral set 3. frequency must be below 64MHz */
#define BOARD_DC2_VALUE		BOARD_DC1_VALUE

#elif (BOARD_CLOCK_MHZ == 80)

/* 80 MHz system clock */
/* see Table 6-15. FMPLL lookup table in MPC5646CRM.pdf */
/* these the values are fed directly into the register, logical divisors may differ */
#define BOARD_IDF_VALUE		1u /* i.e. divide by 2 */
#define BOARD_ODF_VALUE		1u /* i.e. divide by 4 */
#define BOARD_NDIV_VALUE	40u
/* input is 16 MHz, output is ( (16 * 40) / (2 * 4) ) -> 80 MHz */

/* clock dividers */
/* z0 core, flash and FEC need a /2 divider if the system clock is faster than 80 MHz */
#define BOARD_HIGHSPEED_DIVIDER	0u /* 1: divider active, 0: divider disabled */
/* divider for peripheral set 1. frequency must be below 32MHz */
#define BOARD_DC0_VALUE		4 /* logical value, i.e. divide by 4 */
/* divider for peripheral set 2. frequency must be below 64MHz */
#define BOARD_DC1_VALUE		2 /* logical value, i.e. divide by 2 */
/* divider for peripheral set 3. frequency must be below 64MHz */
#define BOARD_DC2_VALUE		BOARD_DC1_VALUE

#elif (BOARD_CLOCK_MHZ == 32)

/* 32 MHz system clock */
/* see Table 6-15. FMPLL lookup table in MPC5646CRM.pdf */
/* these the values are fed directly into the register, logical divisors may differ */
#define BOARD_IDF_VALUE		1u /* i.e. divide by 2 */
#define BOARD_ODF_VALUE		2u /* i.e. divide by 8 */
#define BOARD_NDIV_VALUE	32u
/* input is 16 MHz, output is ( (16 * 32) / (2 * 8) ) -> 32 MHz */

/* clock dividers */
/* z0 core, flash and FEC need a /2 divider if the system clock is faster than 80 MHz */
#define BOARD_HIGHSPEED_DIVIDER	0u /* 1: divider active, 0: divider disabled */
/* divider for peripheral set 1. frequency must be below 32MHz */
#define BOARD_DC0_VALUE		1 /* logical value, i.e. don't divide */
/* divider for peripheral set 2. frequency must be below 64MHz */
#define BOARD_DC1_VALUE		1 /* logical value, i.e. don't divide */
/* divider for peripheral set 3. frequency must be below 64MHz */
#define BOARD_DC2_VALUE		BOARD_DC1_VALUE

#else

#error "Clock selection in BOARD_CLOCK_MHZ not supported"

#endif /* BOARD_CLOCK_MHZ */

/* ppc timer uses the built-in decrementer clocked with the CPU clock */
#define PPC_TIMER_CLOCK		(BOARD_CLOCK_MHZ * 1000000)
#define PPC_TIMER_IVOR		10

#endif
