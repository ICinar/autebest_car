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

/* io.c */
void serial_init(unsigned int baud);

/* cache.c */
void board_cache_init(void);

/* board.c */
void board_halt(void);
void board_init(void);
void board_init_clocks(void);

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

#endif
