/*
 * mpcore_timer.c
 *
 * ARM Cortex A9 MPCore specific per-CPU timer
 *
 * azuepke, 2013-09-18: initial
 * azuepke, 2013-11-20: split into IRQ and timer code
 * azuepke, 2016-04-18: adapted to MPU
 */

#include <kernel.h>
#include <assert.h>
#include <arm_io.h>
#include <board.h>
#include <mpcore.h>
#include <mpcore_timer.h>
#include <board_stuff.h>


/* registers in per-CPU area, relative to MPCORE_BASE + 0x0100 */

/* the private timer is a count-down timer that fires on the transition to 0 */
#define PTIMER_RELOAD	0x000
#define PTIMER_COUNTER	0x004
#define PTIMER_CTRL		0x008
	/*
	 * Control bits
	 *  bit 15-8:  PRESCALER
	 *  bit 2:     IRQ_ENABLE
	 *  bit 1:     AUTO_RELOAD
	 *  bit 0:     ENABLE
	 */

#define PTIMER_ACK		0x00c


/** time in nanoseconds on last interrupt */
static time_t time_last_tick_ns;
/** ticker time in nanoseconds */
static unsigned int clock_ns;
/** timer resolution in nanoseconds */
unsigned int board_timer_resolution;

/* access to per-CPU specific registers */
static inline uint32_t ptimer_read32(unsigned int reg)
{
	return readl((volatile void *)(MPCORE_PTIMER_BASE + reg));
}

static inline void ptimer_write32(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(MPCORE_PTIMER_BASE + reg), val);
}


/** acknowledge private timer */
static inline void ptimer_ack(void)
{
	ptimer_write32(PTIMER_ACK, 0x1);
}

/** initialize private timer */
static __init void ptimer_init(unsigned int reload)
{
	/* disable and initialize the timer, but do not start it yet */
	ptimer_write32(PTIMER_CTRL, 0);
	ptimer_write32(PTIMER_RELOAD, reload);
	ptimer_write32(PTIMER_COUNTER, reload);
	ptimer_ack();

	/* start the timer (enable timer, IRQ, auto reload, prescaler == 0) */
	ptimer_write32(PTIMER_CTRL, 0x7);
}

/** get current time in nanoseconds */
time_t board_get_time(void)
{
	/* FIXME: implement sub-nanosecond timer resolution! */
	return time_last_tick_ns;
}

/** interrupt handler */
void mpcore_timer_handler(unsigned int irq __unused)
{
	ptimer_ack();

	time_last_tick_ns += clock_ns;

	/* notify kernel on timer interrupt */
	kernel_timer(time_last_tick_ns);
}

//#ifdef SMP
// NOTE: we compile this one all the time to use the same config on UP and SMP
/** IPI-based timer handler on other cores, just notify the kernel */
void mpcore_ipi_timer_handler(unsigned int sender_cpu __unused)
{
	assert(arch_cpu_id() > 0);
	assert(sender_cpu == 0);

	kernel_timer(time_last_tick_ns);
}
//#endif

/** timer implementation -- uses MPCore private timer on core #0 */
void mpcore_timer_init(unsigned int freq)
{
	unsigned int reload;

	reload = MPCORE_TIMER_CLOCK / freq;
	clock_ns = 1000000000 / freq;
	board_timer_resolution = clock_ns;

	ptimer_init(reload);

	board_irq_enable(IRQ_ID_PTIMER);

	/* enable timer interrupt */
	board_irq_enable(IRQ_ID_PTIMER);
}
