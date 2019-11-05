/*
 * shm.h
 *
 * Shared memory (SHM)
 *
 * azuepke, 2014-09-28: initial
 */

#ifndef __SHM_H__
#define __SHM_H__

#include <shm_state.h>

/** SHM configuration -> config.c */
extern const struct shm_cfg shm_cfg[];

/** SHM access configuration -> config.c */
extern const struct shm_access shm_access[];

/** retrieve SHM details */
__tc_fastcall void sys_shm_iterate(unsigned int shm_id);

#endif
