/*
 * board_stuff.h
 *
 * Board specific setting for MPC5748G Calypso.
 *
 * tjordan, 2014-07-14: initial
 */

#ifndef __BOARD_STUFF_H__
#define __BOARD_STUFF_H__

#ifndef __ASSEMBLER__

#include <board.h>

/* serial.c */
void serial_init(unsigned int baud);
unsigned int board_putc(int c);

/* cache.c */
void board_cache_init(void);

/* board.c */
void board_halt(haltmode_t mode);
void board_init(void);
void board_init_clocks(void);
void __board_reset(void);
#ifdef HSM
extern unsigned int hsm_version;
#endif

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

/* CPU IDs as returned by arch_cpu_id */
#define CPU0 0u
#define CPU1 1u
#define CPU2 2u


#define BOARD_ROM_PHYS      0x00fc0000
#define BOARD_ROM_SIZE      0x0003C000  /* 6 MB */

#define BOARD_RAM_PHYS      0x40000000
#define BOARD_RAM_SIZE      0x000C0000 /* 768 KB System RAM */

#define BOOT_STACK  ((BOARD_RAM_PHYS+BOARD_RAM_SIZE)-16)    /* virtual address, grows downwards, 400BFFF0 */

/* PLL setup */
#define BOARD_CLOCK_MHZ     160 // 160 Calypso

#if (BOARD_CLOCK_MHZ == 160)
/* 160 MHz system clock */


#elif (BOARD_CLOCK_MHZ == 120)
/* 120 MHz system clock */
/* see Table 6-15. FMPLL lookup table in MPC5646CRM.pdf */
/* these the values are fed directly into the register, logical divisors may differ */
#define BOARD_IDF_VALUE     1u /* i.e. divide by 2 */
#define BOARD_ODF_VALUE     1u /* i.e. divide by 4 */
#define BOARD_NDIV_VALUE    60u
/* input is 16 MHz, output is ( (16 * 60) / (2 * 4) ) -> 120 MHz */

/* clock dividers */
/* z0 core, flash and FEC need a /2 divider if the system clock is faster than 80 MHz */
#define BOARD_HIGHSPEED_DIVIDER 1u /* 1: divider active, 0: divider disabled */
/* divider for peripheral set 1. frequency must be below 32MHz */
#define BOARD_DC0_VALUE     4 /* logical value, i.e. divide by 4 */
/* divider for peripheral set 2. frequency must be below 64MHz */
#define BOARD_DC1_VALUE     2 /* logical value, i.e. divide by 2 */
/* divider for peripheral set 3. frequency must be below 64MHz */
#define BOARD_DC2_VALUE     BOARD_DC1_VALUE

#elif (BOARD_CLOCK_MHZ == 80)

/* 80 MHz system clock */
/* see Table 6-15. FMPLL lookup table in MPC5646CRM.pdf */
/* these the values are fed directly into the register, logical divisors may differ */
#define BOARD_IDF_VALUE     1u /* i.e. divide by 2 */
#define BOARD_ODF_VALUE     1u /* i.e. divide by 4 */
#define BOARD_NDIV_VALUE    40u
/* input is 16 MHz, output is ( (16 * 40) / (2 * 4) ) -> 80 MHz */

/* clock dividers */
/* z0 core, flash and FEC need a /2 divider if the system clock is faster than 80 MHz */
#define BOARD_HIGHSPEED_DIVIDER 0u /* 1: divider active, 0: divider disabled */
/* divider for peripheral set 1. frequency must be below 32MHz */
#define BOARD_DC0_VALUE     4 /* logical value, i.e. divide by 4 */
/* divider for peripheral set 2. frequency must be below 64MHz */
#define BOARD_DC1_VALUE     2 /* logical value, i.e. divide by 2 */
/* divider for peripheral set 3. frequency must be below 64MHz */
#define BOARD_DC2_VALUE     BOARD_DC1_VALUE

#elif (BOARD_CLOCK_MHZ == 32)

/* 32 MHz system clock */
/* see Table 6-15. FMPLL lookup table in MPC5646CRM.pdf */
/* these the values are fed directly into the register, logical divisors may differ */
#define BOARD_IDF_VALUE     1u /* i.e. divide by 2 */
#define BOARD_ODF_VALUE     2u /* i.e. divide by 8 */
#define BOARD_NDIV_VALUE    32u
/* input is 16 MHz, output is ( (16 * 32) / (2 * 8) ) -> 32 MHz */

/* clock dividers */
/* z0 core, flash and FEC need a /2 divider if the system clock is faster than 80 MHz */
#define BOARD_HIGHSPEED_DIVIDER 0u /* 1: divider active, 0: divider disabled */
/* divider for peripheral set 1. frequency must be below 32MHz */
#define BOARD_DC0_VALUE     1 /* logical value, i.e. don't divide */
/* divider for peripheral set 2. frequency must be below 64MHz */
#define BOARD_DC1_VALUE     1 /* logical value, i.e. don't divide */
/* divider for peripheral set 3. frequency must be below 64MHz */
#define BOARD_DC2_VALUE     BOARD_DC1_VALUE

#else

#error "Clock selection in BOARD_CLOCK_MHZ not supported"

#endif /* BOARD_CLOCK_MHZ */

/* ppc timer uses the built-in decrementer clocked with the CPU clock */
#define PPC_TIMER_CLOCK     (BOARD_CLOCK_MHZ * 1000000)

/* Utilities */
#define MEMORY_WORD(address) (*((volatile uint32_t *) (address)))
#define MEMORY_HALF(address) (*((volatile uint16_t *) (address)))
#define MEMORY_BYTE(address) (*((volatile uint8_t *)  (address)))

#define WRITE32(value,address) (MEMORY_WORD((address))=(value))
#define WRITE16(value,address) (MEMORY_HALF((address))=(value))
#define WRITE8(value,address)  (MEMORY_BYTE((address))=(value))

#endif
