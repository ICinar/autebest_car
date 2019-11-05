/*
 * intc.c
 *
 * TI AM335x Interrupt controller (Beaglebone Black)
 *
 * azuepke, 2014-01-03: initial Beaglebone Black port
 * azuepke, 2014-05-06: adapted to MPU
 */

/*
 * See chapters 6 "Interrupts" of the TI manual:
 * "AM335x ARM Cortex-A8 Microprocessors (MPUs) Technical Reference Manual"
 * October 2011 -- Revised December 2013
 *
 * Specs:
 * - 128 interrupt sources
 * - level-sensitive only
 * - all interrupts can also be raised by software
 * - 128 interrupt priorities
 *   - 0 highest / 0x7f lowest priority
 *   - all interrupts lower or equal a threshold are masked
 *   - 0xff disables prio mechanism
 */

#include <kernel.h>
#include <assert.h>
#include <arm_private.h>
#include <arm_insn.h>
#include <arm_io.h>
#include <board.h>
#include <board_stuff.h>
#include <board.h>
#include <isr.h>
#include <hm.h>


/* we support 128 possible IRQ vectors */
#define NUM_IRQS		128
#define SPURIOUS_BIT	0x80


/** default interrupt handler */
__cold void board_unhandled_irq_handler(unsigned int irq)
{
	hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
}


/** INTC registers */
#define INTC_REVISION			0x00	/* revision */
#define INTC_SYSCONFIG			0x10	/* config (reset, auto-idle) */
#define INTC_SYSSTATUS			0x14	/* status (reset complete) */
#define INTC_SIR_IRQ			0x40	/* currently active IRQ */
#define INTC_SIR_FIQ			0x44	/* currently active FIQ */
#define INTC_CONTROL			0x48	/* ack (w1c, bit 0: IRQ, bit 1: FIQ) */
#define INTC_PROTECTION			0x4c	/* user mode access protection */
#define INTC_IDLE				0x50	/* idle mode settings */
#define INTC_IRQ_PRIORITY		0x60	/* currently active IRQ priority */
#define INTC_FIQ_PRIORITY		0x64	/* currently active FIQ priority */
#define INTC_THRESHOLD			0x68	/* priority threshold */

#define INTC_ITR(x)				(0x80+0x20*(x))	/* raw interrupt status */
#define INTC_MIR(x)				(0x84+0x20*(x))	/* interrupt mask */
#define INTC_MIR_CLEAR(x)		(0x88+0x20*(x))	/* clear bit in mask */
#define INTC_MIR_SET(x)			(0x8c+0x20*(x))	/* set bit in mask */
#define INTC_ISR_SET(x)			(0x90+0x20*(x))	/* set software interrupts */
#define INTC_ISR_CLEAR(x)		(0x94+0x20*(x))	/* clear software interrupts */
#define INTC_PENDING_IRQ(x)		(0x98+0x20*(x))	/* pending IRQs */
#define INTC_PENDING_FIQ(x)		(0x9c+0x20*(x))	/* pending FIQs */

#define INTC_ILR(x)				(0x100+4*(x))	/* bit 0: FIQ, bit 2..7: prio */


/* access to INTC registers */
static inline uint32_t intc_read(unsigned int reg)
{
	return readl((volatile void *)(INTC_BASE + reg));
}

static inline void intc_write(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(INTC_BASE + reg), val);
}

/** init: setup default handlers, etc */
__init void intc_init(void)
{
	unsigned int irq;

	/* configure all interrupts as IRQs, prio 15 */
	for (irq = 0; irq < NUM_IRQS; irq++) {
		intc_write(INTC_ILR(irq), (15 << 2) | 0);
	}
	/* raise DMTimer0 IRQ to prio 0 */
	intc_write(INTC_ILR(DMTIMER_IRQ), (0 << 2) | 0);

	/* prevent user accessing the INTC */
	intc_write(INTC_PROTECTION, 0x1);

	/* mask all interrupts */
	intc_write(INTC_MIR_SET(0), 0xffffffff);
	intc_write(INTC_MIR_SET(1), 0xffffffff);
	intc_write(INTC_MIR_SET(2), 0xffffffff);
	intc_write(INTC_MIR_SET(3), 0xffffffff);

	/* allow all interrupts */
	intc_write(INTC_THRESHOLD, 0x7f);
}

/** mask IRQ in distributor */
void board_irq_disable(unsigned int irq)
{
	unsigned int word, bit;

	assert(irq < NUM_IRQS);

	word = irq / 32;
	bit = irq % 32;

	intc_write(INTC_MIR_SET(word), 1u << bit);
}

/** unmask IRQ in distributor */
void board_irq_enable(unsigned int irq)
{
	unsigned int word, bit;

	assert(irq < NUM_IRQS);

	word = irq / 32;
	bit = irq % 32;

	intc_write(INTC_MIR_CLEAR(word), 1u << bit);
}

/** dispatch IRQ: mask and ack, call handler */
void board_irq_dispatch(unsigned int vector __unused)
{
	unsigned int irq;

	irq = intc_read(INTC_SIR_IRQ);

	if (irq < NUM_IRQS) {
		/* device interrupt: call handler, will mask interrupt if necessary */
		isr_cfg[irq].func(isr_cfg[irq].arg0);
	} else {
		/* spurious! */
	}

	/* ack IRQ */
	intc_write(INTC_CONTROL, 0x1);
	arm_dsb();
}
