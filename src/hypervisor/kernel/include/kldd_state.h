/*
 * kldd_state.h
 *
 * Kernel level device driver (KLDD) state representation
 *
 * azuepke, 2014-03-09: initial
 */

#ifndef __KLDD_STATE_H__
#define __KLDD_STATE_H__

/** upper limit of KLDDs in the system (so we can use 8-bit indices) */
#define MAX_KLDDS	256

/** internal function call type for a KLDD */
typedef unsigned int (*kldd_func_t)(void *arg0, unsigned long arg1,
                                    unsigned long arg2, unsigned long arg3);

/** first internal parameter of a KLDD */
struct kldd_cfg {
	kldd_func_t func;
	void *arg0;
};

#endif
