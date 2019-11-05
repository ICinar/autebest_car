/*
 * dmtimer.c
 *
 * TI AM335x DMTimer driver
 *
 * azuepke, 2013-09-18: initial
 * azuepke, 2013-11-20: split into IRQ and timer code
 * azuepke, 2013-12-23: cloned from ARM MPCore Timer
 * azuepke, 2014-05-06: adapted to MPU
 */

/*
 * See chapter 20.1 "DMTimer" of the TI manual:
 * "AM335x ARM Cortex-A8 Microprocessors (MPUs) Technical Reference Manual"
 * October 2011 -- Revised December 2013
 *
 * Specs:
 * - free running upward counter with auto reload on overflow
 * - compare logic with interrupt on match
 * - PRCM input #2 selects bus clock
 * - an overflow is generated when reaching ffff'ffff
 * - the timer rate is: (ffff'ffff - TLDR + 1) * clock_period * clock_divider
 * - we use a divider of 1 (2^n, from 1 to 256 possible)
 * - "posted" mode to handle coupling of clock domains
 *   - TCLR, TLDR, TCRR, TTGR and TMAR are affected for writing
 *   - TCRR, TCAR1 and TCAR2 are affected for reading
 */


#include <kernel.h>
#include <assert.h>
#include <arm_io.h>
#include <board.h>
#include <board_stuff.h>


/* Timer registers */
#define TIDR			0x00	/* identification register */
#define TIOCP_CFG		0x10	/* OCP configuration register */
#define IRQ_EOI			0x20	/* IRQ end-of-interrupt register */
#define IRQSTATUS_RAW	0x24	/* IRQ status raw register */
#define IRQSTATUS		0x28	/* IRQ status register */
#define IRQENABLE_SET	0x2c	/* interrupt enable set register */
#define IRQENABLE_CLR	0x30	/* interrupt enable clear register */
#define IRQWAKEEN		0x34	/* IRQ wakeup enable register */

#define TCLR			0x38	/* timer control register */
#define TCRR			0x3c	/* counter running register */
#define TLDR			0x40	/* (re-)load register */
#define TTGR			0x44	/* trigger register */
#define TWPS			0x48	/* timer write posting bits register */
#define TMAR			0x4c	/* timer match register */
#define TCAR1			0x50	/* timer capture register */
#define TSICR			0x54	/* timer synchronous interface control register */
#define TCAR2			0x58	/* timer capture register */

/* Register accessors */
static inline uint32_t dmtimer_read(unsigned int reg)
{
	return readl((volatile void *)(DMTIMER_BASE + reg));
}

static inline void dmtimer_write(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(DMTIMER_BASE + reg), val);
}

/* Posted mode: wait until previous write have finished */
static inline void dmtimer_flush_posted(void)
{
	while ((dmtimer_read(TWPS) & 0x1f) != 0)
		;
}

/* interrupts in IRQSTATUS and IRQENABLE */
#define IRQ_MAT			0x0001	/* Match */
#define IRQ_OVF			0x0002	/* Overflow */
#define IRQ_TCAR		0x0004	/* TCAR */

/* bits in TCLR */
#define TCLR_ST			0x0001	/* start counter */
#define TCLR_AR			0x0002	/* auto reload */
#define TCLR_PTV0		0x0004	/* prescaler bit 0 */
#define TCLR_PTV1		0x0008	/* prescaler bit 1 */
#define TCLR_PTV2		0x0010	/* prescaler bit 2 */
#define TCLR_PRE		0x0020	/* enable prescaler */
#define TCLR_CE			0x0040	/* compare mode */
#define TCLR_SCPWM		0x0080	/* pulse mode negative */
#define TCLR_TCM0		0x0100	/* transition capture mode bit 0 */
#define TCLR_TCM1		0x0200	/* transition capture mode bit 1 */
#define TCLR_TRG0		0x0400	/* trigger output mode bit 0 */
#define TCLR_TRG1		0x0800	/* trigger output mode bit 1 */
#define TCLR_PT			0x1000	/* toggle mode */
#define TCLR_CAPT_MODE	0x2000	/* capture on second event */
#define TCLR_GPO_CFG	0x4000	/* drive timer pin as input */

/* bits in TSICR */
#define TSICR_SFT		0x0002	/* reset functional parts */
#define TSICR_POSTED	0x0004	/* posted mode */


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

/** interrupt handler */
void dmtimer_handler(unsigned int irq __unused)
{
	dmtimer_flush_posted();
	dmtimer_write(IRQSTATUS, IRQ_OVF);

	time_last_tick_ns += clock_ns;

	/* notify kernel on timer interrupt */
	kernel_timer(time_last_tick_ns);
}

/** timer implementation -- uses SP804 private timer on core #0 */
void __init dmtimer_init(unsigned int freq)
{
	unsigned int reload;

	reload = DMTIMER_CLOCK / freq;
	clock_ns = 1000000000 / freq;
	board_timer_resolution = clock_ns;

	/* disable and initialize the timer, but do not start it yet */
	dmtimer_write(TSICR, TSICR_POSTED);
	dmtimer_write(TCLR, 0);
	dmtimer_flush_posted();

	/* program interrupts */
	dmtimer_write(IRQENABLE_CLR, IRQ_MAT | IRQ_OVF | IRQ_TCAR);

	/* set start and reload value */
	dmtimer_write(TCRR, 0xffffffff - reload);
	dmtimer_write(TLDR, 0xffffffff - reload);
	dmtimer_write(TTGR, 0);

	/* unmask timer interrupt */
	board_irq_enable(DMTIMER_IRQ);

	/* enable timer in 32-bit periodic mode with interrupts */
	dmtimer_flush_posted();
	dmtimer_write(IRQENABLE_SET, IRQ_OVF);
	dmtimer_write(TCLR, TCLR_ST | TCLR_AR);
	dmtimer_flush_posted();
}
