/*
 * mpic.c
 *
 * PPC MPIC interrupt controller
 *
 * azuepke, 2013-11-23: initial PPC port
 * azuepke, 2014-05-06: adapted to MPU
 */

#include <kernel.h>
#include <assert.h>
#include <ppc_insn.h>
#include <ppc_io.h>
#include <board.h>
#include <board_stuff.h>
#include <isr.h>
#include <hm.h>


/* we support 80 possible IRQ vectors: 64 internal of the MPIC, 16 external */
#define NUM_IRQS 80

/* special IRQ IDs */
#define IRQ_ID_IPI			0xfffe
#define IRQ_ID_SPURIOUS		0xffff
#define IRQ_ID_MASK			0xffff



/* real number of interrupts */
static unsigned int num_irqs;
static volatile unsigned int stop_request;


/* default interrupt handlers */

__cold void board_unhandled_irq_handler(unsigned int irq)
{
	hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
}

/* CPU private registers */
#define MPIC_IPIDR0			0x0040	/* IPI dispatch register 0..3 */
#define MPIC_IPIDR1			0x0050
#define MPIC_IPIDR2			0x0060
#define MPIC_IPIDR3			0x0070
#define MPIC_CTPR			0x0080	/* current task priority */
#define MPIC_WHOAMI			0x0090	/* who am I */
#define MPIC_IACK			0x00a0	/* interrupt acknowledge */
#define MPIC_EOI			0x00b0	/* end of interrupt */

/* global registers */
#define MPIC_FRR			0x1000	/* feature reporting register */
#define MPIC_GCR			0x1020	/* global configuration register */
#define MPIC_PIR			0x1090	/* processor core initialization register */
#define MPIC_IPIPR0			0x10a0	/* IPI vector/priority register 0..3 */
#define MPIC_IPIPR1			0x10b0
#define MPIC_IPIPR2			0x10c0
#define MPIC_IPIPR3			0x10d0
#define MPIC_SVR			0x10e0	/* spurious vector register */


/* MPIC specific register accessors */
static inline uint32_t mpic_read(unsigned int reg)
{
	return readl((volatile void *)(MPIC_BASE + reg));
}

static inline void mpic_write(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(MPIC_BASE + reg), val);
}


#ifdef SMP

/** determine current CPU via PIR SPR */
static unsigned int mpic_current_cpu(void)
{
	return ppc_get_smp_id();
}

/** send stop request to other cores */
void __cold mpic_send_stop(void)
{
	stop_request = 1;
	/* FIXME: IMPLEMENTME */
	assert(0);
}

/** send rescheduling request to another processor */
static void mpic_send_reschedule(unsigned int cpu)
{
	/* FIXME: IMPLEMENTME */
	(void)cpu;
	assert(0);
}

/** detect the number of processors and initialize SMP stuff */
__init unsigned int mpic_init_smp(void)
{
	unsigned int val;
	unsigned int num_cpus;

	val = mpic_read(MPIC_FRR);
	num_cpus = ((val >> 8) & 0x1f) + 1;

	board.cpus = num_cpus;
	board.current_cpu = mpic_current_cpu;
	board.reschedule = mpic_send_reschedule;

	return num_cpus;
}
#endif

/** initialize the mpic distributed interrupt controller */
__init void mpic_irq_init(void)
{
	unsigned int val;

	val = mpic_read(MPIC_FRR);
	num_irqs = ((val >> 16) & 0x7ff) + 1;
	printf("MPIC supports %u IRQs\n", num_irqs);

#if 0
	/* FIXME: IMPLEMENT! */
#endif
}

/** enable the CPU specific GIC parts */
__init void mpic_enable(void)
{
	/* use 0xffff as spurious vector */
	mpic_write(MPIC_SVR, IRQ_ID_SPURIOUS);


#if 0
	/* FIXME: IMPLEMENT! */
	unsigned int val;

	/* mask and clear all private interrupts */
	dist_write32(DIST_DISABLE, 0xffffffff);
	dist_write32(DIST_PENDING, 0xffffffff);

	/* prio 255 let all interrupts pass through, no preemption */
	gic_write32(GIC_PRIO, 0xff);
	gic_write32(GIC_BINPOINT, 0x7);

	/* set bit 0 to enable the GIC CPU interface */
	val = gic_read32(GIC_CTRL);
	val |= 1;
	gic_write32(GIC_CTRL, val);
#endif
}


/** mask IRQ in distributor */
void board_irq_disable(unsigned int irq)
{
	assert(irq < num_irqs);

	/* FIXME: IMPLEMENTME! */
	(void)irq;
}

/** unmask IRQ in distributor */
void board_irq_enable(unsigned int irq)
{
	assert(irq < num_irqs);

	/* FIXME: IMPLEMENTME! */
	(void)irq;
}

/** dispatch IRQ: mask and ack, call handler */
void board_irq_dispatch(unsigned int vector)
{
	unsigned int irq;

	if (vector == 10) {
		/* decrementer interrupt */
		ppc_timer_handler();
	} else if (vector == 4) {
		/* external interrupt */
		irq = mpic_read(MPIC_IACK) & IRQ_ID_MASK;

		if (irq == IRQ_ID_IPI) {
			/* IPI */
			if (unlikely(stop_request)) {
				__board_halt();
			}

			/* ... rescheduling IPIs do nothing ... */

			mpic_write(MPIC_EOI, 0);
			return;
		} else if (irq == IRQ_ID_SPURIOUS) {
			/* spurious! */
			return;
		}

		if (unlikely(irq >= num_irqs)) {
			hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
		}

		/* acknowledge */
		mpic_write(MPIC_EOI, 0);

		/* call handler, will mask interrupt if necessary */
		isr_cfg[irq].func(isr_cfg[irq].arg0);
	} else {
		hm_system_error(HM_ERROR_UNHANDLED_IRQ, 0x1000 + vector);
	}
}
