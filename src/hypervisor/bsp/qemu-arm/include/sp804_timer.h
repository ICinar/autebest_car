/*
 * sp804_timer.h
 *
 * ARM Cortex A9 sp804 specific IRQ handling
 *
 * azuepke, 2013-11-20: initial
 */

#ifndef __SP804_TIMER_H__
#define __SP804_TIMER_H__

void sp804_timer_init(unsigned int freq);
void sp804_timer_handler(unsigned int irq);
void sp804_ipi_timer_handler(unsigned int sender_cpu);

#endif
