/*
 * hm.c
 *
 * Health monitoring / error handling.
 *
 * azuepke, 2013-03-22: initial (panic.c)
 * azuepke, 2015-02-28: initial (hm.c
 * azuepke, 2015-03-05: merged panic
 * azuepke, 2015-05-27: use HM tables
 */

#include <kernel.h>
#include <assert.h>
#include <hm.h>
#include <hv_error.h>
#include <board.h>
#include <arch.h>
#include <sched.h>
#include <core.h>
#include <part.h>
#include <task.h>

static uint8_t current_system_hm_table = 0;	/* only changed by CPU #0 */

#ifndef NDEBUG
static const char *hm_strerror(unsigned int hm_error_id)
{
	switch (hm_error_id) {
	case HM_ERROR_ILLEGAL_INSTRUCTION:		return "ILLEGAL_INSTRUCTION";
	case HM_ERROR_PRIVILEGED_INSTRUCTION:	return "PRIVILEGED_INSTRUCTION";
	case HM_ERROR_TRAP:						return "TRAP";
	case HM_ERROR_ARITHMETIC_OVERFLOW:		return "ARITHMETIC_OVERFLOW";
	case HM_ERROR_BOOT_ERROR:				return "BOOT_ERROR";
	case HM_ERROR_HARDWARE_ERROR:			return "HARDWARE_ERROR";
	case HM_ERROR_FPU_ACCESS:				return "FPU_ACCESS";
	case HM_ERROR_FPU_ERROR:				return "FPU_ERROR";
	case HM_ERROR_NMI:						return "NMI";
	case HM_ERROR_MCE:						return "MCE";
	case HM_ERROR_UNHANDLED_IRQ:			return "UNHANDLED_IRQ";
	case HM_ERROR_POWER_FAIL:				return "POWER_FAIL";
	case HM_ERROR_UNALIGNED_CODE:			return "UNALIGNED_CODE";
	case HM_ERROR_UNALIGNED_DATA:			return "UNALIGNED_DATA";
	case HM_ERROR_SYNC_BUS_ERROR:			return "SYNC_BUS_ERROR";
	case HM_ERROR_ASYNC_BUS_ERROR:			return "ASYNC_BUS_ERROR";
	case HM_ERROR_ICACHE_ERROR:				return "ICACHE_ERROR";
	case HM_ERROR_DCACHE_ERROR:				return "DCACHE_ERROR";
	case HM_ERROR_CODE_MEMORY_ERROR:		return "CODE_MEMORY_ERROR";
	case HM_ERROR_DATA_MEMORY_ERROR:		return "DATA_MEMORY_ERROR";
	case HM_ERROR_MPU_ERROR_READ:			return "MPU_ERROR_READ";
	case HM_ERROR_MPU_ERROR_WRITE:			return "MPU_ERROR_WRITE";
	case HM_ERROR_MPU_ERROR_CODE:			return "MPU_ERROR_CODE";
	case HM_ERROR_MPU_ERROR_DEVICE:			return "MPU_ERROR_DEVICE";
	case HM_ERROR_CONTEXT_UNDERFLOW:		return "CONTEXT_UNDERFLOW";
	case HM_ERROR_CONTEXT_OVERFLOW:			return "CONTEXT_OVERFLOW";
	case HM_ERROR_CONTEXT_ERROR:			return "CONTEXT_ERROR";
	case HM_ERROR_STACK_OVERFLOW:			return "STACK_OVERFLOW";
	case HM_ERROR_DEADLINE_MISSED:			return "DEADLINE_MISSED";
	case HM_ERROR_TASK_ACTIVATION_ERROR:	return "TASK_ACTIVATION_ERROR";
	case HM_ERROR_TASK_STATE_ERROR:			return "TASK_STATE_ERROR";
	case HM_ERROR_ABORT:					return "ABORT";
	case HM_ERROR_USER_CONFIG_ERROR:		return "USER_CONFIG_ERROR";
	case HM_ERROR_USER_APPLICATION_ERROR:	return "USER_APPLICATION_ERROR";
	default:								return "???";
	}
}
#endif

void hm_async_task_error(
	const struct task_cfg *task_cfg,
	unsigned int hm_error_id,
	unsigned long extra)
{
	const struct part_cfg *part_cfg;
	const struct hm_table *hm_table;
	unsigned int error_code;
	unsigned int curr_pos;
	unsigned int next_pos;
	struct task *task;
	struct part *part;
	unsigned int err;

	assert(task_cfg != NULL);
	assert(hm_error_id < NUM_HM_ERROR_IDS);

	/* task ID is global, get associated partition */
	assert(task_cfg != NULL);
	assert(task_cfg->cpu_id == arch_cpu_id());
	part_cfg = task_cfg->part_cfg;
	part = part_cfg->part;

	/* check partition state */
	if (part->operating_mode != PART_OPERATING_MODE_NORMAL) {
		goto part_error;
	}

	/* check HM table */
	hm_table = &hm_table_part_cfg[part_cfg->part_id];
	if (!HM_IS_TASK_LEVEL(hm_table->level_action_error_code[hm_error_id])) {
		goto part_error;
	}
	error_code = hm_table->level_action_error_code[hm_error_id] & HM_ERROR_CODE_MASK;

	if (unlikely(part_cfg->error_hook_id == 0xffff)) {
		/* partition has no error hook, elevate to partition error */
		goto part_error;
	}

	/* NOTE: tools must check this */
	assert(part_cfg->user_error_state != NULL);
	assert(part_cfg->num_error_states > 0);
	assert(part_cfg->num_error_states <= 256);

	/* activate error hook */
	task = &part_cfg->tasks[part_cfg->error_hook_id];
	assert(task != NULL);
	assert(task->cfg->max_activations == part_cfg->num_error_states);

	err = task_check_activate(task);
	if (err != E_OK) {
		assert(err == E_OS_LIMIT);
		/* error overflow! */
		goto part_error;
	}

	curr_pos = part->error_write_pos;
	next_pos = curr_pos + 1;
	if (next_pos == part_cfg->num_error_states) {
		next_pos = 0;
	}
	part->error_write_pos = next_pos;

	/* prepare error record */
	part_cfg->user_error_state[curr_pos].task_id = task_cfg->task_id;
	part_cfg->user_error_state[curr_pos].error_code = error_code;
	part_cfg->user_error_state[curr_pos].extra = extra;

	/* activate hook */
	task_do_activate(task);
	return;

part_error:
	hm_part_error(part_cfg, hm_error_id);
}


/** general exception handling */
void hm_exception(
	struct arch_reg_frame *regs,
	int fatal,
	unsigned int hm_error_id,
	unsigned long vector,
	unsigned long fault_addr,
	unsigned long aux)
{
	assert(regs != NULL);
	assert(hm_error_id < NUM_HM_ERROR_IDS);

	/* first, pass exception to board layer to handle it */
	if (board_hm_exception(regs, fatal, hm_error_id, vector, fault_addr, aux)) {
		/* board layer handled the exception, return and retry */
		return;
	}

	if (unlikely(fatal)) {
#ifndef NDEBUG
		printf("Fatal exception in partition '%s' task '%s': %d [%s]\n",
		       current_part_cfg()->name, current_task()->cfg->name,
		       hm_error_id, hm_strerror(hm_error_id));
		arch_dump_registers(regs, vector, fault_addr, aux);
#endif

		hm_system_error(hm_error_id, fault_addr);
	}

#ifndef NDEBUG
	printf("User exception in partition '%s' task '%s': %d [%s]\n",
	       current_part_cfg()->name, current_task()->cfg->name,
	       hm_error_id, hm_strerror(hm_error_id));
	arch_dump_registers(regs, vector, fault_addr, aux);
#endif

	/* non fatal exception: notify user */
	hm_exception_user(hm_error_id, fault_addr);
}

/** non-fatal exception in user space */
void hm_exception_user(
	unsigned int hm_error_id,
	unsigned long fault_addr)
{
	const struct task_cfg *curr_task_cfg;
	const struct part_cfg *part_cfg;
	const struct hm_table *hm_table;
	unsigned int error_code;
	struct task *task;
	struct part *part;

	assert(hm_error_id < NUM_HM_ERROR_IDS);

	part_cfg = current_part_cfg();
	assert(part_cfg->cpu_id == arch_cpu_id());
	part = part_cfg->part;

	/* check partition state */
	if (part->operating_mode != PART_OPERATING_MODE_NORMAL) {
		goto part_error;
	}

	/* check HM table */
	hm_table = &hm_table_part_cfg[part_cfg->part_id];
	if (!HM_IS_TASK_LEVEL(hm_table->level_action_error_code[hm_error_id])) {
		goto part_error;
	}
	error_code = hm_table->level_action_error_code[hm_error_id] & HM_ERROR_CODE_MASK;

	if (unlikely(part_cfg->exception_hook_id == 0xffff)) {
		/* partition has no exception hook, elevate to partition error */
		goto part_error;
	}

	/* NOTE: tools must check this */
	assert(part_cfg->user_exception_state != NULL);

	/* handle it ... */
	task = &part_cfg->tasks[part_cfg->exception_hook_id];
	assert(task != NULL);

	if (unlikely(!TASK_STATE_IS_SUSPENDED(task->flags_state))) {
		/* recursive exception, elevate to partition error */
		goto part_error;
	}

	/* prepare exception record */
	curr_task_cfg = current_task()->cfg;
	part_cfg->user_exception_state->task_id = curr_task_cfg->task_id;
	part_cfg->user_exception_state->error_code = error_code;
	part_cfg->user_exception_state->fault_addr = fault_addr;

	/* activate hook -- at the HEAD of the ready queue! */
	task_prepare(task);
	sched_readyq_insert_head(task);
	return;

part_error:
	hm_part_error(part_cfg, hm_error_id);
}

/** raise a partition error */
void hm_part_error(
	const struct part_cfg *part_cfg,
	unsigned int hm_error_id)
{
	const struct hm_table *hm_table;
	unsigned int new_mode;
	unsigned int action;
	struct part *part;

	assert(part_cfg != NULL);
	assert(hm_error_id < NUM_HM_ERROR_IDS);

	assert(part_cfg->cpu_id == arch_cpu_id());

	/* check HM table */
	hm_table = &hm_table_part_cfg[part_cfg->part_id];
	action = hm_table->level_action_error_code[hm_error_id] & HM_ACTION_MASK;
	if (HM_IS_SYSTEM_LEVEL(hm_table->level_action_error_code[hm_error_id])) {
		goto system_error;
	}

#ifndef NDEBUG
	printf("Partition '%s' error %d [%s]: %s\n",
	       part_cfg->name, hm_error_id, hm_strerror(hm_error_id),
	       (action == HM_ACTION_PARTITION_IGNORE) ? "ignored" :
	       (action == HM_ACTION_PARTITION_IDLE) ? "partition shutdown" : "partition restart");
#endif

	if (action == HM_ACTION_PARTITION_IGNORE) {
		return;
	}

	if (action == HM_ACTION_PARTITION_COLD_START) {
		new_mode = PART_OPERATING_MODE_COLD_START;
	} else if (action == HM_ACTION_PARTITION_WARM_START) {
		new_mode = PART_OPERATING_MODE_WARM_START;
	} else {
		assert(action == HM_ACTION_PARTITION_IDLE);
		new_mode = PART_OPERATING_MODE_IDLE;
	}

	/* request shut down or partition restart */
	/* the change will be handled on the next call to the scheduler */
	part = part_cfg->part;
	part_delayed_state_change(part, new_mode);

	/* return without setting a return code */
	return;

system_error:
#ifndef NDEBUG
	printf("Partition '%s' error %d [%s]: %s\n",
	       part_cfg->name, hm_error_id, hm_strerror(hm_error_id),
	       (action == HM_ACTION_SYSTEM_IGNORE) ? "ignored" :
	       (action == HM_ACTION_SYSTEM_SHUTDOWN) ? "system shutdown" : "system reset");
#endif

	if (action == HM_ACTION_SYSTEM_IGNORE) {
		return;
	} else if (action == HM_ACTION_SYSTEM_SHUTDOWN) {
		board_halt(BOARD_HM_SHUTDOWN);
	} else {
		board_halt(BOARD_HM_RESET);
	}
}

/** abort() syscall (for program crashes, etc) */
void sys_abort(void)
{
#ifndef NDEBUG
	const struct task_cfg *cfg;
	struct task *task;

	task = current_task();
	cfg = task->cfg;

	printf("Abort in task '%s' part '%s'\n", cfg->name, cfg->part_cfg->name);
#endif

	hm_part_error(current_part_cfg(), HM_ERROR_ABORT);
}

/** raise an application error */
void sys_hm_inject(unsigned int hm_error_id, unsigned long extra)
{
	if ((hm_error_id < FIRST_HM_USER_ERROR_ID) ||
	    (hm_error_id >= NUM_HM_ERROR_IDS)) {
		SET_RET(E_OS_ID);	/* ERRNO: invalid error ID */
		return;
	}

	SET_RET(E_OK);
	hm_async_task_error(current_task()->cfg, hm_error_id, extra);
}

/** raise a system error */
void hm_system_error(
	unsigned int hm_error_id,
	/** auxilary information, e.g. fault status register */
	unsigned long aux __unused)
{
	const struct hm_table *hm_table;
	unsigned int action;

	/* check system HM table */
	assert(current_system_hm_table < num_hm_tables);
	hm_table = &hm_table_system_cfg[current_system_hm_table];
	assert(hm_error_id < NUM_HM_ERROR_IDS);
	action = hm_table->level_action_error_code[hm_error_id] & HM_ACTION_MASK;

#ifndef NDEBUG
	printf("System error %d [%s] (aux:0x%08lx) -> %s\n",
	       hm_error_id, hm_strerror(hm_error_id), aux,
	       (action == HM_ACTION_SYSTEM_IGNORE) ? "ignored" :
	       (action == HM_ACTION_SYSTEM_SHUTDOWN) ? "system shutdown" : "system reset");
#endif

	if (action == HM_ACTION_SYSTEM_IGNORE) {
		return;
	} else if (action == HM_ACTION_SYSTEM_SHUTDOWN) {
		board_halt(BOARD_HM_SHUTDOWN);
	} else {
		board_halt(BOARD_HM_RESET);
	}
}

/** Log application error message */
__tc_fastcall void sys_hm_log(void *err_msg __unused, size_t size __unused)
{
	// FIXME: IMPLEMENTME!
	SET_RET(E_OS_NOFUNC);
}

/** Change system HM table (privileged) */
void sys_hm_change(unsigned int table_id)
{
	if (!(current_part_cfg()->flags & PART_FLAG_PRIVILEGED)) {
		SET_RET(E_OS_ACCESS);	/* ERRNO: partition privilege error */
		return;
	}

	if (arch_cpu_id() != 0) {
		SET_RET(E_OS_CORE);	/* ERRNO: restricted to first core */
		return;
	}

	if (table_id >= num_hm_tables) {
		SET_RET(E_OS_ID);
		return;
	}

	current_system_hm_table = table_id;
	SET_RET(E_OK);
}

/** Reboot system call */
void sys_shutdown(haltmode_t mode)
{
	if (!(current_part_cfg()->flags & PART_FLAG_PRIVILEGED)) {
		SET_RET(E_OS_ACCESS);	/* ERRNO: partition privilege error */
		return;
	}

	board_halt(mode);
	/* DOES NOT RETURN */
}

#ifndef NDEBUG
/** kernel assertion */
void __assert(const char *file, int line, const char *func, const char *cond)
{
	struct sched_state *sched;
	struct task *task;
	unsigned int cpu;
	unsigned int i;

	cpu = arch_cpu_id();

	for (i = 0; i < num_cpus; i++) {
		if (core_cfg[cpu].core_state->hm_panic_in_progress) {
			__board_halt();
			/* DOES NOT RETURN */
		}
	}

	core_cfg[cpu].core_state->hm_panic_in_progress = 1;

	printf("Kernel panic");
#ifdef SMP
	printf(" on CPU %d", cpu);
#endif

	/* can't use current_sched_state() here (may not be usable yet) */
	sched = core_cfg[cpu].sched;
	task = sched->current_task;
	if (task != NULL) {
		const struct task_cfg *cfg = task->cfg;

		if (cfg != NULL) {
			printf(" in part '%s' task '%s'",
			       cfg->part_cfg->name,
			       cfg->name);
		}
	}
	printf(", system halted.\n");
	printf("Reason: %s:%d: %s: assertion '%s' failed.\n", file, line, func, cond);

	board_halt(BOARD_HM_ASSERT);
}
#endif
