/**
 * \file   wdt.h
 * \brief  Interface of the Watchdof driver.
 *
 * \date   25.02.2015
 * \author easycore GmbH, 91058 Erlangen, Germany
 */

#ifndef WDT_H
#define WDT_H

/*==================[macros]==================================================*/

/** Base address of the watchdog system. */
#define WDT_BASE                0xf0036000

/* Offsets of the four different watchdogs */
#define WDT_S_OFFSET            0x0f0 /* Safety Watchdog */
#define WDT_CPU0_OFFSET         0x100 /* CPU0 Watchdog */
#define WDT_CPU1_OFFSET         0x10c /* CPU1 Watchdog */
#define WDT_CPU2_OFFSET         0x118 /* CPU2 Watchdog */
/* watchdog reload value, watchdog counts up and triggers at 0xffff */
#define WDT_RELOAD_VALUE        0x0000


/*==================[external function declarations]==========================*/

void wdt_set_endinit(unsigned long wdt_base, int endinit);
unsigned int wdt_get_endinit(unsigned long wdt_base);
void wdt_service(unsigned long wdt_base);
void wdt_disable(unsigned long wdt_base);

/* Utilities */

#define unlock_watchdog_cpu0() wdt_set_endinit(WDT_BASE + WDT_CPU0_OFFSET, 0)
#define lock_watchdog_cpu0()   wdt_set_endinit(WDT_BASE + WDT_CPU0_OFFSET, 1)

#define watchdog_get_endinit_cpu0() wdt_get_endinit(WDT_BASE + WDT_CPU0_OFFSET);


#endif
/*==================[end of file]=============================================*/

