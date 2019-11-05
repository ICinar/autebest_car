/*
 * rti_timer.h
 *
 * TMS570 Real-Time Interrupt (RTI)
 *
 * azuepke, 2014-01-27: initial
 */

#ifndef __RTI_TIMER_H__
#define __RTI_TIMER_H__

void rti_timer_init(unsigned int freq);
void rti_timer_handler(unsigned int irq);

#endif
