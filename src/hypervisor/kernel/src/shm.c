/*
 * shm.c
 *
 * Shared memory (SHM)
 *
 * azuepke, 2014-09-28: initial
 */

#include <kernel.h>
#include <assert.h>
#include <shm.h>
#include <part.h>
#include <task.h>
#include <sched.h>
#include <hv_error.h>

/** retrieve SHM details */
void sys_shm_iterate(
	unsigned int shm_id)
{
	const struct part_cfg *part_cfg;
	const struct shm_cfg *cfg;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (shm_id >= part_cfg->num_shm_accs) {
		SET_RET(E_OS_ID);
		return;
	}
	cfg = part_cfg->shm_accs[shm_id].shm_cfg;

	SET_OUT1(cfg->base);
	SET_OUT2(cfg->size);
	SET_RET(E_OK);
}
