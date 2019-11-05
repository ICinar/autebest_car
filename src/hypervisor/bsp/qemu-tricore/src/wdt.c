/*
 * wdt.c
 *
 * TriCore Watchdog handling.
 *
 * azuepke, 2014-12-31: initial
 */

/*
 * This driver supports two versions of the watchdog (WDT) hardware:
 * the much simpler ones used in TC1796, TC1797, and TC1798, and
 * the more complex ones used in TC27x and newer.
 *
 * As TSIM implements the old version, we implement both variants.
 * The preprocessor define AURIX_WDT switches modes.
 *
 * Bits only available for the old watchdog are tagged (old), and
 * flags for the new one are tagged with (new).
 * But we drive the watchdog in a kind of compatibility mode and only
 * use features already available in the old versions.
 *
 * Depending on the board, multiple independent watchdogs need to be served.
 */

#include <kernel.h>
#include <assert.h>
#include <tc_io.h>
#include <board_stuff.h>
#include <sched.h>	/* num_cpus */


/** Offsets of watchdog registers */
#define WDT_CON0			0x000
#define WDT_CON1			0x004
#define WDT_SR				0x008

/* Register accessors. */
#define WDT_READ(wdt, reg)			readl((volatile void *)((wdt) + (reg)))
#define WDT_WRITE(wdt, reg, val)	writel((volatile void *)((wdt) + (reg)), (val))

/** Bits in CON0: */
#define CON0_ENDINIT		0x0001		/* ENDINIT bit,  */
#define CON0_LCK			0x0002		/* CON0 register locked */
#define CON0_REL(x)			((x)<<16)	/* watchdog reload value */

/** Bits in CON1: */
#define CON1_CLRIRF			0x0001		/* clear internal request flag ("S" only) */
#define CON1_IR0			0x0004		/* input frequency, bit 0 */
#define CON1_DR				0x0008		/* disable request */
#define CON1_IR1			0x0020		/* input frequency, bit 1 (new) */
#define CON1_UR				0x0040		/* unlock restriction request (new) */
#define CON1_PAR			0x0080		/* password auto-sequence request (new) */
#define CON1_TCR			0x0100		/* counter check request (new) */
#define CON1_TCTR(x)		((x)<<9)	/* timer check tolerance request (new) */

/** Bits in SR: */
#define SR_AE				0x0001		/* watchdog access error */
#define SR_OE				0x0002		/* watchdog overflow error */
#define SR_IS0				0x0004		/* input frequency, bit 0 */
#define SR_DS				0x0008		/* disable status */
#define SR_TO				0x0010		/* watchdog time-out mode */
#define SR_PR				0x0020		/* prewarning mode (old) */
#define SR_IS1				0x0020		/* input frequency, bit 1 (new) */
#define SR_US				0x0040		/* unlock restriction status (new) */
#define SR_PAS				0x0080		/* password auto-sequence status (new) */
#define SR_TCS				0x0100		/* counter check status (new) */
#define SR_TCT(x)			((x)<<9)	/* timer check tolerance status (new) */
#define SR_TIM(x)			((x)<<16)	/* current watchdog timer value */


/** Unlock access to watchdog, set/clear ENDINIT, lock watchdog */
__init void wdt_set_endinit(unsigned long wdt_base, int endinit)
{
	uint32_t con0, pass;

	con0 = WDT_READ(wdt_base, WDT_CON0);
	assert((con0 & CON0_LCK) != 0);

	/* first write: password */
#ifdef AURIX_WDT
	pass = con0 ^ 0xfc;
	pass = (pass & 0xfffffffc) | CON0_ENDINIT;
#else
	// NOTE: Scheme for TC1798, TSIM is different again
	//pass = (con0 & 0xffffff01) | 0xf0 | (con1 & 0x0c);
	pass = (con0 & 0xffff0000) | 0xf0 | 0x01;
#endif
	WDT_WRITE(wdt_base, WDT_CON0, pass);

	/* second write: set reload value + new pardword, set new ENDINIT */
#ifdef AURIX_WDT
	pass = CON0_REL(WDT_RELOAD_VALUE) | (con0 & 0xfffc) | CON0_LCK;
#else
	pass = CON0_REL(WDT_RELOAD_VALUE) | (con0 & 0xff00) | 0xf0 | CON0_LCK;
#endif
	if (endinit) {
		pass |= CON0_ENDINIT;
	}
	WDT_WRITE(wdt_base, WDT_CON0, pass);

	/* ENDINIT clear: read back CON0 to complete the operation */
	if (!endinit) {
		con0 = WDT_READ(wdt_base, WDT_CON0);
	}
}

/** Service Watchdog */
void wdt_service(unsigned long wdt_base)
{
	uint32_t con0, pass;

	/* unlock watchdog */
	con0 = WDT_READ(wdt_base, WDT_CON0);
	assert((con0 & CON0_LCK) != 0);

	/* first write: password */
#ifdef AURIX_WDT
	pass = con0 ^ 0xfc;
	pass = (pass & 0xfffffffc) | CON0_ENDINIT;
#else
	// NOTE: Scheme for TC1798, TSIM is different again
	//pass = (con0 & 0xffffff01) | 0xf0 | (con1 & 0x0c);
	pass = (con0 & 0xffff0000) | 0xf0 | 0x01;
#endif
	WDT_WRITE(wdt_base, WDT_CON0, pass);

	/* second write: set reload value, keep current ENDINIT */
#ifdef AURIX_WDT
	pass = CON0_REL(WDT_RELOAD_VALUE) | (con0 & 0xfffd) | CON0_LCK;
#else
	pass = CON0_REL(WDT_RELOAD_VALUE) | (con0 & 0xff01) | 0xf0 | CON0_LCK;
#endif
	WDT_WRITE(wdt_base, WDT_CON0, pass);
}

/** Disable watchdog (must be called with ENDINIT clear) */
__init void wdt_disable(unsigned long wdt_base)
{
	uint32_t con1;

	assert((WDT_READ(wdt_base, WDT_CON0) & CON0_ENDINIT) == 0);

	con1 = WDT_READ(wdt_base, WDT_CON1);
	con1 |= CON1_DR;
	WDT_WRITE(wdt_base, WDT_CON1, con1);
}
