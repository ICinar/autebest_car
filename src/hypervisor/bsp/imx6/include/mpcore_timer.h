/*
 * mpcore_timer.h
 *
 * ARM Cortex A9 MPCore specific IRQ handling
 *
 * azuepke, 2013-09-11: initial ARM port
 * azuepke, 2013-11-19: refactored from board.h
 * azuepke, 2013-11-20: split into IRQ and timer code
 * azuepke, 2016-04-18: adapted to MPU
 */

#ifndef __MPCORE_TIMER_H__
#define __MPCORE_TIMER_H__

void mpcore_timer_init(unsigned int freq);
void mpcore_timer_handler(unsigned int irq);
void mpcore_ipi_timer_handler(unsigned int sender_cpu);

#endif
