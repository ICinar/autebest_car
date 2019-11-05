/*
 * part.h
 *
 * Kernel partition handling.
 *
 * azuepke, 2013-11-24: initial MPU version
 */

#ifndef __PART_H__
#define __PART_H__

#include <part_state.h>
#include <assert.h>

/** partition configuration -> config.c */
extern const uint8_t num_partitions;

/** initialize idle partitions */
void part_init_idle(void);
/** initialize remaining partitions */
void part_init_rest(unsigned int start_condition);

/** startup all partitions */
void part_start_all(unsigned int cpu_id);

/** get partition configuration */
static inline const struct part_cfg *part_get_part_cfg(unsigned int part_id)
{
	extern const struct part_cfg part_cfg[];

	assert(part_id < num_partitions);
	return &part_cfg[part_id];
}

/** trigger a change of the partition state (delayed until scheduling) */
void part_delayed_state_change(struct part *part, unsigned int new_mode);
/** do a change of the partition state (called from scheduler) */
void part_state_change(struct part *part);


/** Get the caller's partition operating mode */
__tc_fastcall void sys_part_get_operating_mode(void);

/** Get the caller's partition start condition */
__tc_fastcall void sys_part_get_start_condition(void);

/** system call to change the caller's partitions operating mode */
__tc_fastcall void sys_part_set_operating_mode(
	unsigned int new_mode);

/** system call to get other partition's operating mode */
__tc_fastcall void sys_part_get_operating_mode_ex(
	unsigned int part_id);

/** system call to change other partition's operating mode */
__tc_fastcall void sys_part_set_operating_mode_ex(
	unsigned int part_id,
	unsigned int new_mode);

/** system call to get a partition's partition ID */
__tc_fastcall void sys_part_self(void);

#endif
