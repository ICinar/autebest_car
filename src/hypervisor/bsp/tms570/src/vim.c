/*
 * vim.c
 *
 * TMS570 vectored interrupt manager (VIM)
 *
 * azuepke, 2014-01-27: initial
 */

#include <kernel.h>
#include <assert.h>
#include <arm_private.h>
#include <arm_insn.h>
#include <arm_io.h>
#include <board.h>
#include <board_stuff.h>
#include <board.h>
#include <vim.h>
#include <isr.h>
#include <hm.h>


/* we support 95 possible IRQ vectors */
#define NUM_IRQS 95

struct vim {
	/* 0xdec -- SRAM parity management */
	uint32_t parflg;
	uint32_t parctl;
	uint32_t adderr;
	uint32_t fbparerr;
	uint32_t   unused_fc;

	/* 0xe00 -- IRQ and FIQ index: 0 = no interrupt, 1..96 -> channel 0..95 */
	uint32_t irqindex;
	uint32_t fiqindex;
	uint32_t   unused_08;
	uint32_t   unused_0c;

	/* 0xe10 */
	uint32_t firqpr[4];		/* bit field: 0: IRQ, 1: FIQ */

	/* 0xe20 */
	uint32_t intreq[4];		/* bit field: pending interrupts */

	/* 0xe30 */
	uint32_t reqenaset[4];	/* bit field: enable interrupt channel */

	/* 0xe40 */
	uint32_t reqenaclr[4];	/* bit field: disable interrupt channel */

	/* 0xe50 */
	uint32_t wakeenaset[4];	/* bit field: enable wake-up for channel */

	/* 0xe60 */
	uint32_t wakeenaclr[4];	/* bit field: disable wake-up for channel */

	/* 0xe70 */
	uint32_t irqvector;		/* interrupt vector address from VIC SRAM */
	uint32_t fiqvector;
	uint32_t capevt;		/* capture event */
	uint32_t   unused_7c;

	/* 0xe60 */
	uint8_t chanctrl[96];	/* channel control -- defaults to 1:1 mapping */
};

volatile struct vim * const vim = (volatile struct vim *)VIM_ADDR;


/* default interrupt handlers */

__cold void board_unhandled_irq_handler(unsigned int irq)
{
	hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
}


/** mask IRQ in distributor */
void board_irq_disable(unsigned int irq)
{
	unsigned int word, bit;

	assert(irq < NUM_IRQS);

	word = irq / 32;
	bit = irq % 32;

	vim->reqenaclr[word] = 1u << bit;
}

/** unmask IRQ in distributor */
void board_irq_enable(unsigned int irq)
{
	unsigned int word, bit;

	assert(irq < NUM_IRQS);

	word = irq / 32;
	bit = irq % 32;

	vim->reqenaset[word] = 1u << bit;
}

/** dispatch IRQ: mask and ack, call handler */
/* NOTE: vector is an IDT entry number, but IRQs start from vector 0x20 on! */
void board_irq_dispatch(unsigned int vector __unused)
{
	unsigned int irq;

	/* acknowledges the interrupt in the VIM */
	irq = vim->irqindex;
	if (irq == 0) {
		/* spurious or no other pending interrupt */
		/* FIXME: better use a "count and ignore" strategy */
		hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
	}

	/* adjust channel number */
	irq--;
	assert(irq < NUM_IRQS);

	/* call handler */
	isr_cfg[irq].func(isr_cfg[irq].arg0);
}

__init void vim_irq_init(void)
{
	/* empty */
}
