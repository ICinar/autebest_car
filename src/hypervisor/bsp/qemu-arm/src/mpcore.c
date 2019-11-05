/*
 * mpcore.c
 *
 * ARM Cortex A9 MPCore specific IRQ handling
 *
 * azuepke, 2013-09-18: initial
 * azuepke, 2013-11-20: split into IRQ and timer code
 * azuepke, 2013-11-24: rework for MPU kernel
 */

#include <kernel.h>
#include <assert.h>
#include <arm_private.h>
#include <arm_insn.h>
#include <arm_io.h>
#include <board.h>
#include <mpcore.h>
#include <board_stuff.h>
#include <board.h>
#include <isr.h>
#include <hm.h>


/* we support 256 possible IRQ vectors (even if GIC supports up to 1024) */
#define NUM_IRQS 256

/* real number of interrupts */
static unsigned int num_irqs;
#ifdef SMP
static unsigned int num_cpus;
#endif

/* default interrupt handlers */

void board_unhandled_irq_handler(unsigned int irq)
{
	hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
}

void ipi_irq_handler(unsigned int sender_cpu __unused)
{
#ifdef SMP
	unsigned int cpu = arch_cpu_id();

	assert(cpu != sender_cpu);

	kernel_ipi_handle(cpu, sender_cpu);
#else
	hm_system_error(HM_ERROR_UNHANDLED_IRQ, IRQ_ID_IPI);
#endif
}

__cold void stop_irq_handler(unsigned int sender_cpu __unused)
{
	__board_halt();
}


/*
 * MPCore GIC and distributor registers.
 *
 * Based on the following TRMs (Technical Reference Manual) by ARM:
 *  DDI0416B -- PrimeCell Generic Interrupt Controller (PL390), Revision: r0p0
 *  IHI0048A -- ARM Generic Interrupt Controller, Architecture version 1.0
 *  DDI0388I -- Cortex-A9, Revision: r4p1
 *  DDI0407I -- Cortex-A9 MPCore, Revision: r4p1
 *
 * Interrupt types:
 *  SGI: software generated interrupts, 0..15
 *  PPI: private peripheral interrupt, 16..31
 *  SPI: shared peripheral interrupt, 32..1023 (max, in multiples of 32)
 */


/* registers in per-CPU area, relative to MPCORE_BASE + 0x0100 */
#define GIC_CTRL		0x000
#define GIC_PRIO		0x004
#define GIC_BINPOINT	0x008
#define GIC_ACK			0x00c
#define GIC_EOI			0x010
#define GIC_PENDING		0x018

#define VALID_IRQ_MASK	0x3ff
#define SENDER_CPU(x)	(((x) >> 10) & 0xf)

/* registers in the distributor area, relative to MPCORE_BASE + 0x1000 */
#define DIST_CTRL		0x000	/* global control register */
#define DIST_TYPE		0x004	/* read-only ID register */

#define DIST_ENABLE		0x100	/* write 1 to enable an interrupt */
#define DIST_DISABLE	0x180	/* write 1 to disable an interrupt */

#define DIST_PENDING	0x280	/* write 1 to clear an interrupt */

#define DIST_PRIO		0x400	/* per interrupt priority, byte accessible */
#define DIST_TARGET		0x800	/* per interrupt target, byte accessible */

#define DIST_IRQ_CONFIG	0xc00	/* per interrupt config */
	/* 2 bits per IRQ:
	 *  0x  edge
	 *  1x  level
	 *  x0  N:N model: all processors handle the interrupt
	 *  x1  1:N model: only one processor handles the interrupt
	 */

#define DIST_IPI		0xf00	/* send IPI register */
	#define IPI_ALL_BUT_SELF	0x01000000
	#define IPI_SELF_ONLY		0x02000000
	/* bits 23..16: target CPUs */
	/* bits 3..0: interrupt ID */


/* MPCore specific register accessors */

/* access to per-CPU specific registers */
static inline uint32_t gic_read32(unsigned int reg)
{
	return readl((volatile void *)(GIC_PERCPU_BASE + reg));
}

static inline void gic_write32(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(GIC_PERCPU_BASE + reg), val);
}

/* access to distributor registers */
static inline uint32_t dist_read32(unsigned int reg)
{
	return readl((volatile void *)(GIC_DIST_BASE + reg));
}

static inline void dist_write32(unsigned int reg, uint32_t val)
{
	writel((volatile void *)(GIC_DIST_BASE + reg), val);
}

static inline uint8_t dist_read8(unsigned int reg)
{
	return readb((volatile void *)(GIC_DIST_BASE + reg));
}

static inline void dist_write8(unsigned int reg, uint8_t val)
{
	writeb((volatile void *)(GIC_DIST_BASE + reg), val);
}



#ifdef SMP
/** send stop request to other cores */
void __cold mpcore_send_stop(void)
{
	dist_write32(DIST_IPI, IPI_ALL_BUT_SELF | IRQ_ID_STOP);
}

/** send timer IPI request to other processors from CPU #0 */
void mpcore_broadcast_timer_ipi(void)
{
	dist_write32(DIST_IPI, IPI_ALL_BUT_SELF | IRQ_ID_IPI_TIMER);
}

/** send IPI request to other processors */
void board_ipi_broadcast(unsigned long cpu_mask)
{
	dist_write32(DIST_IPI, cpu_mask << 16 | IRQ_ID_IPI);
}

/** detect the number of processors and initialize SMP stuff */
__init unsigned int mpcore_init_smp(void)
{
	unsigned int val;

	val = dist_read32(DIST_TYPE);
	num_cpus = (val >> 5) + 1;

	return num_cpus;
}
#endif

/** initialize the MPCore distributed interrupt controller */
__init void mpcore_irq_init(void)
{
	unsigned int val;
	unsigned int i;

	val = dist_read32(DIST_TYPE);
	num_irqs = ((val & 0x1f) + 1) * 32;
	printf("GIC supports %u IRQs\n", num_irqs);

	/* mask all interrupts and clear pending bits */
	for (i = 0; i < num_irqs / 32; i++) {
		dist_write32(DIST_DISABLE + i * 4, 0xffffffff);
		dist_write32(DIST_PENDING + i * 4, 0xffffffff);
	}

	/* set all interrupts to max prio */
	for (i = 0; i < num_irqs; i++) {
		/* highest possible priority */
		dist_write8(DIST_PRIO + i, 0x00);
		/* default to first CPU */
		dist_write8(DIST_TARGET + i, 0x01);
	}

	/* FIXME: need to load a valid edge/level configuration for each interrupt! */
	/* SGIs: level triggered, N:N model */
	dist_write32(DIST_IRQ_CONFIG + 0, 0xaaaaaaaa);
	/* PPIs: level triggered, 1:N model */
	dist_write32(DIST_IRQ_CONFIG + 4, 0x7dc00000);
	/* SPIs: edge triggered, 1:N model */
	for (i = 2; i < num_irqs / 16; i++) {
		dist_write32(DIST_IRQ_CONFIG + i * 4, 0x55555555);
	}

	/* set bit 0 to globally enable the GIC */
	val = dist_read32(DIST_CTRL);
	val |= 1;
	dist_write32(DIST_CTRL, val);
}

/** enable the CPU specific GIC parts */
__init void mpcore_gic_enable(void)
{
	unsigned int val;

	/* mask and clear all private interrupts, then enable them */
	dist_write32(DIST_DISABLE, 0xffffffff);
	dist_write32(DIST_PENDING, 0xffffffff);
	dist_write32(DIST_ENABLE, 0xffffffff);

	/* prio 255 let all interrupts pass through, no preemption */
	gic_write32(GIC_PRIO, 0xff);
	gic_write32(GIC_BINPOINT, 0x7);

	/* set bit 0 to enable the GIC CPU interface */
	val = gic_read32(GIC_CTRL);
	val |= 1;
	gic_write32(GIC_CTRL, val);
}


/** mask IRQ in distributor */
void board_irq_disable(unsigned int irq)
{
	unsigned int word, bit;

	assert(irq < num_irqs);

	word = irq / 32;
	bit = irq % 32;
	dist_write32(DIST_DISABLE + 4 * word, 1u << bit);
}

/** unmask IRQ in distributor */
void board_irq_enable(unsigned int irq)
{
	unsigned int word, bit;
#ifdef SMP
	unsigned cpu = arch_cpu_id();	// FIXME: ALEX: bind IRQ to calling core
#endif

	assert(irq < num_irqs);

#ifdef SMP
	/* set target processor */
	dist_write8(DIST_TARGET + 1 * irq, 1u << cpu);
#endif

	word = irq / 32;
	bit = irq % 32;
	dist_write32(DIST_ENABLE + 4 * word, 1u << bit);
}

/** dispatch IRQ: mask and ack, call handler */
/* NOTE: vector is an IDT entry number, but IRQs start from vector 0x20 on! */
void board_irq_dispatch(unsigned int vector __unused)
{
	unsigned int val;
	unsigned int irq;

	do {
		val = gic_read32(GIC_ACK);

		irq = val & VALID_IRQ_MASK;
		if (irq < 32) {
			/* per CPU interrupts, never masked, directly call handler */
			isr_cfg[irq].func((void*)SENDER_CPU(val));
		} else if (irq == VALID_IRQ_MASK) {
			/* spurious! */
			break;
		} else {
			/* device interrupt: call handler, will mask interrupt if necessary */
			assert(irq < num_irqs);
			isr_cfg[irq].func(isr_cfg[irq].arg0);
		}

		gic_write32(GIC_EOI, val);

		/* check for further pending interrupts */
		val = gic_read32(GIC_PENDING);
	} while ((val & VALID_IRQ_MASK) != VALID_IRQ_MASK);
}


/* "magic marker" for KLDD function */
unsigned int board_irq_kldd_magic;

/** trigger a software interrupt
 * used as KLDD - may be used by tests or as a way of inter-partition communication
 * parameters: arg0 - "magic marker" for KLDD
 *             arg1 - logical interrupt number, 0 to 3
 * returns non-null if interrupt was triggered
 *
 * qemu-arm implementation: uses IPI for originating CPU, IPI numbers from 8 to 11
 */
unsigned int board_irq_trigger_kldd(void *arg0, unsigned long arg1,
                                    unsigned long arg2, unsigned long arg3)
{
	unsigned int num = arg1;
	assert(arg0 == &board_irq_kldd_magic);
	(void) (arg0);
	(void) (arg1 | arg2 | arg3);
	if (num < 4) {
		/* FIXME: no SMP support - target is own CPU */
		dist_write32(DIST_IPI, IPI_SELF_ONLY | 8 | num);
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
	assert(arg0 == &board_irq_kldd_magic);
	(void) (arg0);
	(void) (arg1 | arg2 | arg3);
	/* nothing to do */
	return 1;
}
