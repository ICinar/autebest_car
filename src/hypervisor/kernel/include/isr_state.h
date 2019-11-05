/*
 * isr_state.h
 *
 * Kernel ISR configuration.
 *
 * azuepke, 2014-03-09: initial
 */

#ifndef __ISR_STATE_H__
#define __ISR_STATE_H__

/** upper limit of ISRs in the system (so we can use 16-bit indices) */
#define MAX_ISRS	1024

/** an entry in the ISR table */
struct isr_cfg {
	void (*func)(const void *arg0);	/* registered function */
	const void *arg0;				/* regsitered argument */
};

#endif
