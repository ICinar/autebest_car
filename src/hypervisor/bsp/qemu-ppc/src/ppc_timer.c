/*
 * ppc_timer.c
 *
 * PowerPC Book-E decrementer timer
 *
 * We program the PowerPC Book-E decrementer in periodic mode.
 *
 * azuepke, 2013-11-22: initial
 * azuepke, 2014-06-03: adapted to MPU
 */

#include <kernel.h>
#include <assert.h>
#include <board.h>
#include <ppc_spr.h>
#include <board_stuff.h>


/** TCR bits */
#define TCR_WRC1		0x20000000	/* watchdog timer reset control bit 1 */
#define TCR_WRC0		0x10000000	/* watchdog timer reset control bit 0 */
#define TCR_WIE			0x08000000	/* watchdog interrupt enable */
#define TCR_DIE			0x04000000	/* decrementer interrupt enable */
#define TCR_FP1			0x02000000	/* fixed interval timer period bit 1 */
#define TCR_FP0			0x01000000	/* fixed interval timer period bit 0 */
#define TCR_FIE			0x00800000	/* fixed interval interrupt enable */
#define TCR_ARE			0x00400000	/* auto reload enable */

/** TSR bits */
#define TSR_ENW			0x80000000	/* enable next watchdog timer */
#define TSR_WIS			0x40000000	/* watchdog interrupt status */
#define TSR_DIS			0x08000000	/* decrementer interrupt status */
#define TSR_FIS			0x04000000	/* fixed interval interrupt status */


/** time in nanoseconds on last interrupt */
static time_t time_last_tick_ns;
/** current time in nanoseconds */
static unsigned int clock_ns;
/** timer resolution in nanoseconds */
unsigned int board_timer_resolution;

/** get current time in nanoseconds */
time_t board_get_time(void)
{
	/* FIXME: implement sub-nanosecond timer resolution! */
	return time_last_tick_ns;
}

/** interrupt handler */
void ppc_timer_handler(void)
{
	/* ACK */
	ppc_set_spr(SPR_TSR, TSR_DIS);

	time_last_tick_ns += clock_ns;

	/* notify kernel on timer interrupt */
	kernel_timer(time_last_tick_ns);
}

/** timer implementation -- uses PPC private timer on core #0 */
void __init ppc_timer_init(unsigned int freq)
{
	unsigned int reload;

	reload = PPC_TIMER_CLOCK / freq;
	clock_ns = 1000000000 / freq;
	board_timer_resolution = clock_ns;

	/* disable and initialize the timer, but do not start it yet */
	ppc_set_spr(SPR_TCR, 0);
	ppc_set_spr(SPR_DECAR, reload);
	ppc_set_spr(SPR_DEC, reload);

	/* clear previous interrupts */
	ppc_set_spr(SPR_TSR, TSR_DIS);

	/* enable timer in periodic mode with interrupts */
	ppc_set_spr(SPR_TCR, TCR_DIE | TCR_ARE);
}
