/*
 * ipev_state.h
 *
 * Inter Partition Event (IPEV) state representation
 *
 * azuepke, 2014-04-08: initial
 */

#ifndef __IPEV_STATE_H__
#define __IPEV_STATE_H__

#include <stdint.h>

/** upper limit of IPEVs in the system (so we can use 16-bit indices) */
#define MAX_IPEVS	65536

/** IPEV type */
struct ipev_cfg {
	uint16_t global_task_id;
	uint8_t mask_bit;
	uint8_t padding;
};

#endif
