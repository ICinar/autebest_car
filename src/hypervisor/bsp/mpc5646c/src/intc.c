/*
 * intc.c
 *
 * PPC INTC interrupt controller
 *
 * tjordan, 2014-07-14: initial MPC5646C port
 */

#include <kernel.h>
#include <assert.h>
#include <ppc_insn.h>
#include <ppc_io.h>
#include <board.h>
#include <board_stuff.h>
#include <isr.h>
#include <hm.h>


/* we support 278 interrupt vectors */
#define NUM_IRQS 278

/* special IRQ IDs */
#define IRQ_ID_IPI			0xfffe
#define IRQ_ID_SPURIOUS		0xffff
#define IRQ_ID_MASK			0xffff

/* register definitions for INTC */
#define IRQ_INTC_MCR		(*((volatile unsigned int *) 0xFFF48000u))
#define IRQ_INTC_CPR_PRC0	(*((volatile unsigned int *) 0xFFF48008u))
#define IRQ_INTC_CPR_PRC1	(*((volatile unsigned int *) 0xFFF4800Cu))
#define IRQ_INTC_IACKR_PRC0	(*((volatile unsigned int *) 0xFFF48010u))
#define IRQ_INTC_IACKR_PRC1	(*((volatile unsigned int *) 0xFFF48014u))
#define IRQ_INTC_EOIR_PRC0	(*((volatile unsigned int *) 0xFFF48018u))
#define IRQ_INTC_EOIR_PRC1	(*((volatile unsigned int *) 0xFFF4801Cu))
#define IRQ_INTC_SSCIR(x)	(*((volatile unsigned char *) (0xFFF48020u + (x))))
#define IRQ_INTC_PSR(x)		(*((volatile unsigned char *) (0xFFF48040u + (x))))

#define IRQ_IACKR_INTVEC(n)	((n) >> 2)
#define IRQ_SSCIR_SET		0x02u
#define IRQ_SSCIR_CLR		0x01u

#define IRQ_PSR_PRC_SEL(x)	(((x) & 0x03u) << 6)

#define IRQ_PSR_PRC_0		0x00u
#define IRQ_PSR_PRC_BOTH	0x01u
#define IRQ_PSR_PRC_1		0x03u

/* INTC has 16 priorities, but internally, we only use two oft them:
 * priority 0 means that the source is masked, priority 1 is unmasked.
 * consequently, we always set CPR to 0 to enable all unmasked interrupts
 */
#define IRQ_PSR_PRI_MASK	0u
#define IRQ_PSR_PRI_UNMASK	1u

static volatile unsigned int stop_request;


static void irq_INTC_dispatch(void)
{
	/* FIXME: no SMP support for now */
	unsigned int irq;

	/* reading the source number also acknowledges the interrupt */
	irq = IRQ_IACKR_INTVEC(IRQ_INTC_IACKR_PRC0);

	/* call handler, CPR is still 1 - no other interrupts allowed */
	isr_cfg[irq].func(isr_cfg[irq].arg0);

	/* handler finished - either the interrupt was handled or it has been masked.
	 * now we can lower CPR again */
	IRQ_INTC_EOIR_PRC0 = 0; /* clear LIFO */
	IRQ_INTC_CPR_PRC0 = 0; /* reset by hand */

}

/* default interrupt handlers */

__cold void board_unhandled_irq_handler(unsigned int irq)
{
	hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
}

/** initialize the INTC interrupt controller */
__init void board_intc_init(void)
{
	printf("INTC supports %u IRQs\n", NUM_IRQS);
	IRQ_INTC_CPR_PRC0 = 0;
}

/** mask IRQ in distributor */
void board_irq_disable(unsigned int irq)
{
	assert(irq < NUM_IRQS);

	IRQ_INTC_PSR(irq) = IRQ_PSR_PRC_0 | IRQ_PSR_PRI_MASK;
}

/** unmask IRQ in distributor */
void board_irq_enable(unsigned int irq)
{
	assert(irq < NUM_IRQS);

	/* FIXME: no SMP support */
	IRQ_INTC_PSR(irq) = IRQ_PSR_PRC_0 | IRQ_PSR_PRI_UNMASK;
}

/** dispatch IRQ: mask and ack, call handler */
void board_irq_dispatch(unsigned int vector)
{
	/* vector contains the IVOR number */
	if (vector == PPC_TIMER_IVOR) {
		/* decrementer interrupt */
		ppc_timer_handler();
	} else if (vector == 11) {
		/* fixed-interval timer interrupt */
		hm_system_error(HM_ERROR_UNHANDLED_IRQ, 0x1000 + vector);
	} else if (vector == 4) {
		/* INTC interrupt */
		irq_INTC_dispatch();
	} else {
		hm_system_error(HM_ERROR_UNHANDLED_IRQ, 0x1000 + vector);
	}
}

/* "magic marker" for KLDD functions */
unsigned int board_irq_kldd_magic;

/** trigger a software interrupt
 * used as KLDD - may be used by tests or as a way of inter-partition communication
 * parameters: arg0 - "magic marker" for KLDD
 *             arg1 - logical interrupt number, 0 to 3
 * returns non-null if interrupt was triggered
 *
 * mpc5646c implementation: uses software interrupts 4 to 7
 */
unsigned int board_irq_trigger_kldd(void *arg0, unsigned long arg1,
                                    unsigned long arg2, unsigned long arg3)
{
	unsigned int num = arg1;
	assert(arg0 == &board_irq_kldd_magic);
	(void) arg0;
	(void) (arg2 | arg3);
	if (num < 4) {
		IRQ_INTC_SSCIR(num | 4) = IRQ_SSCIR_SET;
	} else {
		return 0;
	}
	return 1;
}

/** clear a software interrupt after it has been reported
 * used as KLDD - may be used by tests or as a way of inter-partition communication
 * parameters: arg0 - "magic marker" for KLDD
 *             arg1 - logical interrupt number, 0 to 3
 * returns non-null if interrupt has been cleared
 */
unsigned int board_irq_clear_kldd(void *arg0, unsigned long arg1,
                                    unsigned long arg2, unsigned long arg3)
{
	unsigned int num = arg1;
	assert(arg0 == &board_irq_kldd_magic);
	(void) arg0;
	(void) (arg2 | arg3);
	if (num < 4) {
		IRQ_INTC_SSCIR(num | 4) = IRQ_SSCIR_CLR;
	} else {
		return 0;
	}
	return 1;
}
