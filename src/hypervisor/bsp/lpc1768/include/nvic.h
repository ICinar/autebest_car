/*
 * nvic.h
 *
 * ARM Cortex-M3/M4 NVIC interrupt controller and timer
 *
 * azuepke, 2015-06-26: initial
 */

#ifndef __NVIC_H__
#define __NVIC_H__

void nvic_irq_init(void);
void nvic_timer_init(unsigned int freq);
void nvic_timer_handler(unsigned int irq);

#endif
