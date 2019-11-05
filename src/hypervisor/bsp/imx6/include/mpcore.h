/*
 * mpcore.h
 *
 * ARM Cortex A9 MPCore specific IRQ handling
 *
 * azuepke, 2013-09-11: initial ARM port
 * azuepke, 2013-11-19: refactored from board.h
 * azuepke, 2013-11-20: split into IRQ and timer code
 */

#ifndef __MPCORE_H__
#define __MPCORE_H__

/* special IRQ IDs */
#define IRQ_ID_STOP			0
#define IRQ_ID_IPI			1
#define IRQ_ID_IPI_TIMER	2

#define IRQ_ID_GTIMER		27
#define IRQ_ID_LEGACY_FIQ	28
#define IRQ_ID_PTIMER		29
#define IRQ_ID_WATCHDOG		30
#define IRQ_ID_LEGACY_IRQ	31

#define IRQ_ID_FIRST_SPI	32

void mpcore_send_stop(void);
void mpcore_irq_init(void);
void mpcore_gic_enable(void);
unsigned int mpcore_init_smp(void);
void mpcore_broadcast_timer_ipi(void);

void ipi_irq_handler(unsigned int sender_cpu);
void stop_irq_handler(unsigned int sender_cpu);

#endif
