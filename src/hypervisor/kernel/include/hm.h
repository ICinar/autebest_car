/*
 * hm.h
 *
 * Health monitoring / error handling.
 *
 * azuepke, 2015-02-28: initial
 */

#ifndef __HM_H__
#define __HM_H__

#include <hm_state.h>
#include <hv_types.h>

/* forward declaration */
struct task_cfg;

/* HM table configuration */
extern const uint8_t num_hm_tables;
extern const struct hm_table hm_table_system_cfg[];
extern const struct hm_table hm_table_part_cfg[];

/** non-fatal asynchronous task error somewhere */
void hm_async_task_error(
	/** faulting task */
	const struct task_cfg *task_cfg,
	/** HM error ID */
	unsigned int hm_error_id,
	/** extra information */
	unsigned long extra);

/** general exception handling */
void hm_exception(
	/** faulting task's registers */
	struct arch_reg_frame *regs,
	/** fatal errors (= system errors) cause a panic */
	int fatal,
	/** HM error ID */
	unsigned int hm_error_id,
	/** vector */
	unsigned long vector,
	/** fault address */
	unsigned long fault_addr,
	/** auxilary information, e.g. fault status register */
	unsigned long aux);

/** non-fatal exception in user space */
void hm_exception_user(
	/** HM error ID */
	unsigned int hm_error_id,
	/** fault address */
	unsigned long fault_addr);

/** raise a partition error */
void hm_part_error(
	/** Related partition */
	const struct part_cfg *part_cfg,
	/** HM error ID */
	unsigned int hm_error_id);

/** raise a system error */
void hm_system_error(
	/** HM error ID */
	unsigned int hm_error_id,
	/** auxilary information, e.g. fault status register */
	unsigned long aux);

/** abort() syscall (for program crashes, etc) */
__tc_fastcall void sys_abort(void);

/** raise an application error */
__tc_fastcall void sys_hm_inject(unsigned int hm_error_id, unsigned long extra);

/** Change system HM table (privileged) */
__tc_fastcall void sys_hm_change(unsigned int table_id);

/** Log application error message */
__tc_fastcall void sys_hm_log(void *err_msg, size_t size);

/** Shutdown or reset the system */
__tc_fastcall void sys_shutdown(haltmode_t mode);

#endif
