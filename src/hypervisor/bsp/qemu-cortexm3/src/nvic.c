/*
 * nvic.c
 *
 * ARM Cortex-M3/M4 NVIC interrupt controller and timer
 *
 * azuepke, 2015-06-26: initial
 */

#include <kernel.h>
#include <assert.h>
#include <arm_private.h>
#include <arm_insn.h>
#include <arm_io.h>
#include <board.h>
#include <nvic.h>
#include <board_stuff.h>
#include <board.h>
#include <isr.h>
#include <hm.h>


/* NVIC registers */
/* interrupt set-enable registers, one bit per interrupt */
#define NVIC_ISER(x)	((&_REG32(0xe000e100))[x])
/* interrupt clear-enable registers, one bit per interrupt */
#define NVIC_ICER(x)	((&_REG32(0xe000e180))[x])
/* interrupt set-pending registers, one bit per interrupt */
#define NVIC_ISPR(x)	((&_REG32(0xe000e200))[x])
/* interrupt clear-pending registers, one bit per interrupt */
#define NVIC_ICPR(x)	((&_REG32(0xe000e280))[x])
/* interrupt active bit registers, one bit per interrupt */
#define NVIC_IABR(x)	((&_REG32(0xe000e300))[x])
/* interrupt priority registers, one byte per interrupt */
#define NVIC_IPR(x)		((&_REG8(0xe000e400))[x])
/* softweare trigger interrupt register, write-only */
#define NVIC_STIR		_REG32(0xe000ef00)

/* bitmap of edge-triggered interrupts, starting from IRQ 0 */
static const unsigned int edge_triggered[(NUM_IRQS + 31)/32] = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
};

__init void nvic_irq_init(void)
{
	unsigned int i;

	/* interrupts have lowest priority */
	for (i = 0; i < NUM_IRQS; i++) {
		NVIC_IPR(i) = 0xf0;
	}

	/* from left to right: reserved | usage fault | bus fault | memory error */
	SHPR1 = 0x00000000;
	/* from left to right: svc call | reserved | reserved | reserved */
	SHPR2 = 0x10000000;
	/* from left to right: systick | pendsv | reserved | reserved */
	SHPR3 = 0xf0f00000;

	/* priority levels 0x00 and 0x10 are reserved for the kernel */
	/* FIXME: all interrupts must use the same priority! */
}

/** mask IRQ in distributor */
void board_irq_disable(unsigned int irq)
{
	unsigned int word, bit;

	assert(irq < NUM_IRQS);

	word = irq / 32;
	bit = irq % 32;
	NVIC_ICER(word) = 1u << bit;
}

/** unmask IRQ in distributor */
void board_irq_enable(unsigned int irq)
{
	unsigned int word, bit;

	assert(irq < NUM_IRQS);

	word = irq / 32;
	bit = irq % 32;

	/* clear any pending level-triggered interrupt */
	if ((edge_triggered[word] & (1u << bit)) == 0) {
		NVIC_ICPR(word) = 1u << bit;
	}
	NVIC_ISER(word) = 1u << bit;
}

/** dispatch IRQ: mask and ack, call handler */
void board_irq_dispatch(unsigned int vector __unused)
{
	unsigned int irq;

	if (vector == 15) {
		nvic_timer_handler(vector);
		return;
	}
	assert(vector >= 16);
	irq = vector - 16;

	assert(irq < NUM_IRQS);

	isr_cfg[irq].func(isr_cfg[irq].arg0);
}

void board_unhandled_irq_handler(unsigned int irq)
{
	hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
}


/* SysTick (24-bit count-down timer) */
#define STK_CTRL	_REG32(0xe000e010)
#define STK_CTRL_COUNTFLAG	0x00010000
#define STK_CTRL_CLKSOURCE	0x00000004
#define STK_CTRL_TICKINT	0x00000002
#define STK_CTRL_ENABLE		0x00000001

#define STK_LOAD	_REG32(0xe000e014)	/* Load with period-1 cycles */

#define STK_VAL		_REG32(0xe000e018)

#define STK_CALIB	_REG32(0xe000e01c)
#define STK_CALIB_NOREF		0x80000000
#define STK_CALIB_SKEW		0x40000000

/** time in nanoseconds on last interrupt */
static time_t time_last_tick_ns;
/** ticker time in nanoseconds */
static unsigned int clock_ns;
/** timer resolution in nanoseconds */
unsigned int board_timer_resolution;

/** get current time in nanoseconds */
time_t board_get_time(void)
{
	/* The NVIC SysTick does not allow to determine if the timer value
	 * is read before or after an underflow/reload condition, thus we cannot
	 * reliably get sub-tick timer resolutions.
	 */
	return time_last_tick_ns;
}

/** interrupt handler */
void nvic_timer_handler(unsigned int irq __unused)
{
	time_last_tick_ns += clock_ns;

	/* notify kernel on timer interrupt */
	kernel_timer(time_last_tick_ns);
}

/** timer implementation */
void __init nvic_timer_init(unsigned int freq)
{
	unsigned int reload;

	reload = NVIC_TIMER_CLOCK / freq;
	clock_ns = 1000000000 / freq;
	board_timer_resolution = clock_ns;

	STK_LOAD = reload - 1;
	barrier();
	STK_VAL = 0;
	barrier();
	STK_CTRL = STK_CTRL_CLKSOURCE | STK_CTRL_TICKINT | STK_CTRL_ENABLE;
}
