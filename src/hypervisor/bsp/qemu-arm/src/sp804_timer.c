/*
 * sp804_timer.c
 *
 * ARM SP804 timer driver
 * http://infocenter.arm.com/help/topic/com.arm.doc.ddi0271d/DDI0271.pdf
 *
 * The SP804 hardware has two identical programmable "Free Running Counters".
 * The timers decrement down and fire at the 1 -> 0 transition.
 * We program them in 32-bit mode as periodic timers.
 *
 * azuepke, 2013-11-20: initial
 */

#include <kernel.h>
#include <assert.h>
#include <arm_io.h>
#include <board.h>
#include <sp804_timer.h>
#include <board_stuff.h>
#include <mpcore.h>


/** Registers of the first timer. */
#define SP804_LOAD			0x000	/* load value */
#define SP804_VALUE			0x004	/* current timer value (read) */
#define SP804_CTRL			0x008	/* control register */
#define SP804_ACK			0x00c	/* interrupt clear register (write) */
#define SP804_RIS			0x010	/* raw interrupt status register */
#define SP804_MIS			0x014	/* masked interrupt status register */
#define SP804_BGLOAD		0x018	/* background load value */

/* Timer 2 starts at offset +0x20 to the first timer. */

/** Bits in control register */
#define SP804_CTRL_EN		0x80	/* enable timer */
#define SP804_CTRL_PERIODIC	0x40	/* periodic mode */
#define SP804_CTRL_INT		0x20	/* interrupt enable */
#define SP804_CTRL_PRE1		0x08	/* prescaler bit 1 */
#define SP804_CTRL_PRE0		0x04	/* prescaler bit 0 */
#define SP804_CTRL_32BIT	0x02	/* 32-bit mode */
#define SP804_CTRL_ONESHOT	0x01	/* oneshot mode */

/* prescaler bits:
 * 00: divide clock by 1
 * 01: divide clock by 16
 * 10: divide clock by 256
 */

/** Bits in ACK, RIS, MIS */
#define SP804_IRQ_BIT		0x01	/* IRQ bit */


/** time in nanoseconds on last interrupt */
static time_t time_last_tick_ns;
/** ticker time in nanoseconds */
static unsigned int clock_ns;
/** timer resolution in nanoseconds */
unsigned int board_timer_resolution;

/** get current time in nanoseconds */
time_t board_get_time(void)
{
	/* FIXME: implement sub-nanosecond timer resolution! */
	return time_last_tick_ns;
}

/* access to per-CPU specific registers */
static inline uint32_t sp804_read32(unsigned int reg)
{
	return readl((volatile void *)(SP804_TIMER_BASE + reg));
}

static inline void sp804_write32(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(SP804_TIMER_BASE + reg), val);
}

/** interrupt handler */
void sp804_timer_handler(unsigned int irq __unused)
{
	sp804_write32(SP804_ACK, SP804_IRQ_BIT);

	time_last_tick_ns += clock_ns;

#ifdef SMP
	/* notify on other cores via IPI */
	assert(arch_cpu_id() == 0);
	mpcore_broadcast_timer_ipi();
#endif

	/* notify kernel on timer interrupt */
	kernel_timer(time_last_tick_ns);
}

//#ifdef SMP
// NOTE: we compile this one all the time to use the same config on UP and SMP
/** IPI-based timer handler on other cores, just notify the kernel */
void sp804_ipi_timer_handler(unsigned int sender_cpu __unused)
{
	assert(arch_cpu_id() > 0);
	assert(sender_cpu == 0);

	kernel_timer(time_last_tick_ns);
}
//#endif

/** timer implementation -- uses SP804 private timer on core #0 */
void __init sp804_timer_init(unsigned int freq)
{
	unsigned int reload;

	reload = SP804_TIMER_CLOCK / freq;
	clock_ns = 1000000000 / freq;
	board_timer_resolution = clock_ns;

	/* disable and initialize the timer, but do not start it yet */
	sp804_write32(SP804_CTRL, SP804_CTRL_32BIT);
	sp804_write32(SP804_LOAD, reload);

	/* enable timer in 32-bit periodic mode with interrupts */
	sp804_write32(SP804_CTRL, SP804_CTRL_EN | SP804_CTRL_PERIODIC |
	                          SP804_CTRL_INT | SP804_CTRL_32BIT);

	/* unmask timer interrupt */
	sp804_write32(SP804_MIS, SP804_IRQ_BIT);
	board_irq_enable(SP804_TIMER_IRQ);
}
