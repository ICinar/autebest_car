/*
 * rti_timer.c
 *
 * TMS570 Real-Time Interrupt (RTI)
 *
 * azuepke, 2014-01-27: initial
 */

#include <kernel.h>
#include <assert.h>
#include <arm_io.h>
#include <board.h>
#include <rti_timer.h>
#include <board_stuff.h>

struct rti {
	/* 0x00 */
	uint32_t gctrl;		/* enable counters */
	uint32_t tbctrl;	/* clock source for RTIUC0 */
	uint32_t capctrl;
	uint32_t compctrl;	/* compare control */

	/* 0x10 */
	uint32_t frc0;		/* free running counter */
	uint32_t uc0;		/* prescaled counter value */
	uint32_t cpuc0;		/* prescaler */
	uint32_t   unused_1c;

	/* 0x20 */
	uint32_t cafrc0;
	uint32_t cauc0;
	uint32_t   unused_28;
	uint32_t   unused_2c;

	/* 0x30 */
	uint32_t frc1;
	uint32_t uc1;
	uint32_t cpuc1;
	uint32_t   unused_3c;

	/* 0x40 */
	uint32_t cafrc1;
	uint32_t cauc1;
	uint32_t   unused_48;
	uint32_t   unused_4c;

	/* 0x50 */
	uint32_t comp0;		/* compare register */
	uint32_t udcp0;		/* update compare register (added to compare reg on match) */
	uint32_t comp1;
	uint32_t udcp1;
	uint32_t comp2;
	uint32_t udcp2;
	uint32_t comp3;
	uint32_t udcp3;

	/* 0x70 */
	uint32_t tblcomp;
	uint32_t tbhcomp;
	uint32_t   unused_78;
	uint32_t   unused_7c;

	/* 0x80 */
	uint32_t setintena;		/* write 0x1 to enable INT0 */
	uint32_t clearintena;	/* write 0x1 to disable INT0 */
	uint32_t intflag;		/* write 0x1 to ack INT0 */
	uint32_t   unused_8c;

	/* 0x90 */
	uint32_t dwdctrl;
	uint32_t dwdprld;
	uint32_t wdstatus;
	uint32_t wdkey;
	uint32_t dwdcntr;
	uint32_t wwdrxnctrl;
	uint32_t wwdsizectrl;

	/* 0xac */
	uint32_t intclrenable;	/* auto clear features */
	uint32_t cimp0clr;		/* compare clear register */
	uint32_t cimp1clr;
	uint32_t cimp2clr;
	uint32_t cimp3clr;
};

/*
 * we use ...
 * - block 0 using the RTICLK clock source
 * - a prescaler of 2 (e.g. CPUC0 set to 1)
 * - event 0 for interrupt generation (VIM interrupt #2)
 */
volatile struct rti * const rti = (volatile struct rti *)RTI_ADDR;


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
void rti_timer_handler(unsigned int irq __unused)
{
	rti->intflag = 1;		/* ack INT0 */

	time_last_tick_ns += clock_ns;

	/* notify kernel on timer interrupt */
	kernel_timer(time_last_tick_ns);
}

/** timer implementation -- uses SP804 private timer on core #0 */
void __init rti_timer_init(unsigned int freq)
{
	unsigned int reload;

	reload = (RTI_FREQ / 2) / freq;	/* /2 due to prescaler of 2 */
	clock_ns = 1000000000 / freq;
	board_timer_resolution = clock_ns;

	rti->gctrl = 0;			/* disable RTI timers */
	rti->intclrenable = 0x05050505 ;	/* disable auto clearing */
	rti->tbctrl = 0;		/* use RTIUC0 as clock */
	rti->compctrl = 0;		/* compare control -> all use RTIFRC0 */
	rti->cpuc0 = 1;			/* use a prescaler of 2 */

	rti->frc0 = 0;			/* start value */
	rti->uc0 = 0;			/* start value */

	rti->comp0 = reload;	/* set start comparator + increment */
	rti->udcp0 = reload;

	rti->setintena = 1;		/* enable INT0 */
	board_irq_enable(RTI_IRQ0);

	rti->gctrl = 1;			/* enable counter 0 -- FIXME: must come last */
}
