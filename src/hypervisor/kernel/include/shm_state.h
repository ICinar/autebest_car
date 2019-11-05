/*
 * shm_state.h
 *
 * Shared memory (SHM) state representation
 *
 * azuepke, 2014-09-28: initial
 */

#ifndef __SHM_STATE_H__
#define __SHM_STATE_H__

#include <stdint.h>

/** upper limit of SHMs in the system (so we can use 8-bit indices) */
#define MAX_SHMS	256

/** SHM configuration */
struct shm_cfg {
	/** base address */
	addr_t base;
	/** size */
	size_t size;
};

/** static SHM access configuration of the partitions */
struct shm_access {
	/** Related SHM */
	const struct shm_cfg *shm_cfg;
};

#endif
