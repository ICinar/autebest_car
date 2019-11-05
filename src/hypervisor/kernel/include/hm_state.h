/*
 * hm_state.h
 *
 * ARINC compatible health monitoring.
 *
 * azuepke, 2015-05-13: initial
 * azuepke, 2015-05-22: reworked understanding of tables
 */

#ifndef __HM_STATE_H__
#define __HM_STATE_H__

#include <stdint.h>

/*
 * ARINC HM handling in a nutshell:
 *
 * An error is classified by a high level "error id", which describes an error
 * like "instruction fetch outside MPU region" or "partition initialization
 * error".
 * The error IDs are not standardized and defined by the operating system (us).
 * These errors are then fed into a multi-stage error handling system:
 *
 * At first, the HM system checks if the error is a "partition error" that can
 * be reliably accounted to the current partition , e.g. an exception is
 * synchronous to execution of code in the partition's user space, or if the
 * error is a "module error", e.g. if the error happened in the kernel
 * or trusted driver.
 *
 * For module errors or "system errors", as we call them, the HM system checks
 * the currently active "module HM table".
 * The module (i.e., the system, in ARINC speak) can have multiple operation
 * modes/phases, for example "module initialization" or "module function",
 * which relate to (at least two) different module error tables.
 * The module HM table decides what to do when a critical error at module level
 * happens, whether to IGNORE, SHUTDOWN, or RESET the system.
 *
 * For partition errors, the "multi partition HM table" is consulted next.
 * This table is shared between multiple partitions. ARINC shows an example
 * with two tables for all system partitions and all application partitions.
 * This table has the same levels and attributes the module HM table, e.g.,
 * reset/shutdown the system or forward the error to the third table,
 * the partition's HM table.
 *
 * The third and last table is the partition's HM table. Unlike for former two,
 * this table handles errors at partition or task/process level. For partition
 * level errors, the actions comprise IGNORE, IDLE, COLD_START, and
 * WARM_RESTART to change the partition state. For task level errors, the
 * error ID is translated into an appropiate error code and the HM process
 * is notified. If no HM process is enabled or the error happens in the
 * HM process itself, the error is promoted to become a partition error again.
 * So, in either case, a partition level recovery action must be present.
 */

/*
 * We compress these three tables of ARINC into just two tables:
 *
 * For "system errors", the current "system HM table" is consulted.
 * This table directly relates to the ARINC module HM table.
 * We provide a privileged system call to switch between these module HM tables.
 * 
 * Similarly, "partition errors" are handled by a dedicated "partition HM table"
 * for each partition. Unlike ARINC, the partition HM table allows to define
 * module level actions as well to allow folding "multi partition HM table" in.
 * 
 */

/*
 * Error IDs: We support up to NUM_HM_ERROR_IDS different errors listed below,
 * so the tools need to emit entries for all possible errors.
 */

#define HM_ERROR_ILLEGAL_INSTRUCTION	0	/* CPU executed an illegal instruction. */
#define HM_ERROR_PRIVILEGED_INSTRUCTION	1	/* CPU executed a privileged instruction */
#define HM_ERROR_TRAP					2	/* trap or debug exception */
#define HM_ERROR_ARITHMETIC_OVERFLOW	3	/* Integer arithmetic error, e.g. overflow */

#define HM_ERROR_BOOT_ERROR				4	/* various errors at boot time */
#define HM_ERROR_HARDWARE_ERROR			5	/* hardware error */
#define HM_ERROR_FPU_ACCESS				6	/* FPU access error (FPU is currently disabled) */
#define HM_ERROR_FPU_ERROR				7	/* FPU arithmetic error */

#define HM_ERROR_NMI					8	/* unhandled NMI */
#define HM_ERROR_MCE					9	/* unhandled machine check exception */
#define HM_ERROR_UNHANDLED_IRQ			10	/* unhandled IRQ */
#define HM_ERROR_POWER_FAIL				11	/* power failure */

#define HM_ERROR_UNALIGNED_CODE			12	/* unaligned code execution */
#define HM_ERROR_UNALIGNED_DATA			13	/* unaligned data access */
#define HM_ERROR_SYNC_BUS_ERROR			14	/* synchronous bus error */
#define HM_ERROR_ASYNC_BUS_ERROR		15	/* asynchronous bus error */

#define HM_ERROR_ICACHE_ERROR			16	/* instruction cache error */
#define HM_ERROR_DCACHE_ERROR			17	/* data cache error */
#define HM_ERROR_CODE_MEMORY_ERROR		18	/* code memory error */
#define HM_ERROR_DATA_MEMORY_ERROR		19	/* data memory error */

#define HM_ERROR_MPU_ERROR_READ			20	/* MPU permission error on data read */
#define HM_ERROR_MPU_ERROR_WRITE		21	/* MPU permission error on data write */
#define HM_ERROR_MPU_ERROR_CODE			22	/* MPU permission error on instruction fetch */
#define HM_ERROR_MPU_ERROR_DEVICE		23	/* MPU permission error on hardware devices / peripheral access */

#define HM_ERROR_CONTEXT_UNDERFLOW		24	/* TriCore: register context or call stack underflow */
#define HM_ERROR_CONTEXT_OVERFLOW		25	/* TriCore: register context or call stack overflow */
#define HM_ERROR_CONTEXT_ERROR			26	/* TriCore: critical context error */
#define HM_ERROR_STACK_OVERFLOW			27	/* stack overflow */

#define HM_ERROR_DEADLINE_MISSED		28	/* task deadline missed */
#define HM_ERROR_TASK_ACTIVATION_ERROR	29	/* task activation error (multiple activation) */
#define HM_ERROR_TASK_STATE_ERROR		30	/* task state error (wrong state in event set) */
/*      not yet assigned                31 */

/* NOTE: errors from here on can be raised by the partition via sys_hm_inject() */
#define HM_ERROR_ABORT					32	/* user called sys_abort() */
#define HM_ERROR_USER_CONFIG_ERROR		33	/* user specific configuration error */
#define HM_ERROR_USER_APPLICATION_ERROR	34	/* user specific application error */
/*      not yet assigned                35 */

#define FIRST_HM_USER_ERROR_ID			32
#define NUM_HM_ERROR_IDS				36


/* default SYSTEM level actions for above errors (all shutdown) */
#define HM_ACTION_DEFAULT_SYSTEM_0		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_1		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_2		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_3		HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_SYSTEM_4		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_5		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_6		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_7		HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_SYSTEM_8		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_9		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_10		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_11		HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_SYSTEM_12		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_13		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_14		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_15		HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_SYSTEM_16		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_17		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_18		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_19		HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_SYSTEM_20		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_21		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_22		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_23		HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_SYSTEM_24		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_25		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_26		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_27		HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_SYSTEM_28		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_29		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_30		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_31		HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_SYSTEM_32		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_33		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_34		HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_SYSTEM_35		HM_ACTION_SYSTEM_SHUTDOWN


/* default PARTITION level actions for above errors */
#define HM_ACTION_DEFAULT_PARTITION_0	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_EXCEPTION)
#define HM_ACTION_DEFAULT_PARTITION_1	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_EXCEPTION)
#define HM_ACTION_DEFAULT_PARTITION_2	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_EXCEPTION)
#define HM_ACTION_DEFAULT_PARTITION_3	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_EXCEPTION)

#define HM_ACTION_DEFAULT_PARTITION_4	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_5	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_6	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_EXCEPTION)
#define HM_ACTION_DEFAULT_PARTITION_7	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_EXCEPTION)

#define HM_ACTION_DEFAULT_PARTITION_8	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_9	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_10	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_11	HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_PARTITION_12	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_EXCEPTION)
#define HM_ACTION_DEFAULT_PARTITION_13	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_EXCEPTION)
#define HM_ACTION_DEFAULT_PARTITION_14	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_15	HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_PARTITION_16	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_17	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_18	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_19	HM_ACTION_SYSTEM_SHUTDOWN

#define HM_ACTION_DEFAULT_PARTITION_20	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_MEMORY)
#define HM_ACTION_DEFAULT_PARTITION_21	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_MEMORY)
#define HM_ACTION_DEFAULT_PARTITION_22	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_MEMORY)
#define HM_ACTION_DEFAULT_PARTITION_23	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_MEMORY)

#define HM_ACTION_DEFAULT_PARTITION_24	(HM_ACTION_TASK_IDLE | E_OS_STACKFAULT)	/* we treat CSA under/overflows like stack errors */
#define HM_ACTION_DEFAULT_PARTITION_25	(HM_ACTION_TASK_IDLE | E_OS_STACKFAULT)
#define HM_ACTION_DEFAULT_PARTITION_26	HM_ACTION_SYSTEM_SHUTDOWN
#define HM_ACTION_DEFAULT_PARTITION_27	(HM_ACTION_TASK_IDLE | E_OS_STACKFAULT)

#define HM_ACTION_DEFAULT_PARTITION_28	(HM_ACTION_TASK_IDLE | E_OS_PROTECTION_TIME)
#define HM_ACTION_DEFAULT_PARTITION_29	(HM_ACTION_TASK_IDLE | E_OS_LIMIT)
#define HM_ACTION_DEFAULT_PARTITION_30	(HM_ACTION_TASK_IDLE | E_OS_STATE)
#define HM_ACTION_DEFAULT_PARTITION_31	HM_ACTION_TASK_IDLE /* not assigned yet */

#define HM_ACTION_DEFAULT_PARTITION_32	HM_ACTION_PARTITION_IDLE
#define HM_ACTION_DEFAULT_PARTITION_33	HM_ACTION_PARTITION_IDLE
#define HM_ACTION_DEFAULT_PARTITION_34	(HM_ACTION_TASK_IDLE | E_OS_USER_ERROR)
#define HM_ACTION_DEFAULT_PARTITION_35	HM_ACTION_TASK_IDLE /* not assigned yet */

/*
 * HM tables
 * For each error ID, we compress level, action and error code into an 8-bit
 * value with the following encoding:
 *   bit 7:     system level (set) or partition level (clear)
 *   bit 5..6:  action
 *   bit 0..4:  error code, 0 meaning no task error
 *
 * In the system HM table, the error code is ignored and only SYSTEM level
 * errors and their according actions are allowed.
 *
 * In the partition HM table, additional both SYSTEM and PARTITION levels
 * are allowed. Additionally, TASK actions are encoded as PARTITION
 * actions with a non-zero error code. If TASK level HM handling fails, the
 * kernel falls back to partition level error handling.
 */

/* ARINC module error actions: INGORE/RESET/SHUTDOWN */
#define HM_ACTION_SYSTEM_IGNORE			0x80
#define HM_ACTION_SYSTEM_SHUTDOWN		0xa0	/* system level default */
#define HM_ACTION_SYSTEM_RESET			0xc0
/*      HM_ACTION_SYSTEM_UNUSED			0xe0 */

/* ARINC partition error actions: IGNORE/IDLE/WARM_START/COLD_START */
#define HM_ACTION_PARTITION_IGNORE		0x00
#define HM_ACTION_PARTITION_IDLE		0x20
#define HM_ACTION_PARTITION_WARM_START	0x40
#define HM_ACTION_PARTITION_COLD_START	0x60	/* partition level default */

/* ARINC task error actions (map 1:1 to partition error actions) */
#define HM_ACTION_TASK_IGNORE			HM_ACTION_PARTITION_IGNORE
#define HM_ACTION_TASK_IDLE				HM_ACTION_PARTITION_IDLE
#define HM_ACTION_TASK_WARM_START		HM_ACTION_PARTITION_WARM_START
#define HM_ACTION_TASK_COLD_START		HM_ACTION_PARTITION_COLD_START

/* masks to get SYSTEM and PARTITION action and ERROR_CODE */
#define HM_ACTION_MASK					0xe0
#define HM_ERROR_CODE_MASK				0x1f

/* testers */
#define HM_IS_SYSTEM_LEVEL(x)			(((x) & 0x80) != 0)
#define HM_IS_PARTITION_LEVEL(x)		(((x) & 0x80) == 0)
#define HM_IS_TASK_LEVEL(x)				(((x) & HM_ERROR_CODE_MASK) != 0)

/** HM table for system level error handling */
struct hm_table {
	/** HM action */
	uint8_t level_action_error_code[NUM_HM_ERROR_IDS];
};

#endif
