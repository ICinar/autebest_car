/*
 * Os_Types.h
 *
 * AUTOSAR OS library types
 *
 * This is an internal header for the OS implementation; OS users should include Os.h only.
 *
 * azuepke, 2013-12-02: initial
 */

#ifndef OS_TYPES_H
#define OS_TYPES_H

#include <Std_Types.h>


/** Error Code */
#ifndef STATUSTYPEDEFINED
#define STATUSTYPEDEFINED
#define E_OK        0x00u
typedef unsigned char StatusType;
#endif

/** Task Type */
typedef uint32 TaskType;

/** Task Type Pointer */
typedef uint32 *TaskRefType;

/** Invalid Task ID */
#define INVALID_TASK    -1UL

/** Invalid ISR ID */
#define INVALID_ISR     -1UL

/** ISR Type */
typedef uint32 ISRType;


/** Task State Type */
typedef uint32 TaskStateType;

/** Task State Type Pointer */
typedef uint32 *TaskStateRefType;

/** Task States */
#define RUNNING     0UL
#define WAITING     1UL
#define READY       2UL
#define SUSPENDED   3UL


/** Event Mask */
typedef uint32 EventMaskType;

/** Event Mask Pointer */
typedef EventMaskType *EventMaskRefType;

/** Resource */
typedef uint32 ResourceType;

#define INVALID_RESOURCE  -1UL

/** Application Mode */
typedef uint32 AppModeType;

/** Counter values in ticks */
typedef uint32 TickType;

/** Pointer to counter values in ticks */
typedef uint32 *TickRefType;

/** Counter characteristics */
typedef struct {
	/** The maximum value of a counter in ticks. */
	TickType maxallowedvalue;
	/** Number of ticks of a counter to reach a significant "unit". */
	TickType ticksperbase;
	/** The minimum value for a cycle. */
	TickType mincycle;
} AlarmBaseType;

/** Pointer to counter characteristics */
typedef AlarmBaseType *AlarmBaseRefType;

/** Alarm object */
typedef uint32 AlarmType;

/** Counter object */
typedef uint32 CounterType;

/** Schedule table object */
typedef uint32 ScheduleTableType;

/** Schedule table status constants (match SCHEDTAB_STATE_*) */
#define SCHEDULETABLE_STOPPED		0
#define SCHEDULETABLE_NEXT			1
#define SCHEDULETABLE_WAITING		2
#define SCHEDULETABLE_RUNNING		3
#define SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS	4

/** Schedule table status */
typedef uint32 ScheduleTableStatusType;

/** Pointer to schedule table status */
typedef ScheduleTableStatusType *ScheduleTableStatusRefType;

/** Unique list of service IDs for the ErrorHook() */
typedef enum {
	OSServiceId_INVALID = 0,
	OSServiceId_ActivateTask,
	OSServiceId_CancelAlarm,
	OSServiceId_ChainTask,
	OSServiceId_ClearEvent,
	OSServiceId_DisableAllInterrupts,
	OSServiceId_EnableAllInterrupts,
	OSServiceId_GetActiveApplicationMode,
	OSServiceId_GetAlarmBase,
	OSServiceId_GetAlarm,
	OSServiceId_GetCounterValue,
	OSServiceId_GetElapsedValue,
	OSServiceId_GetEvent,
	OSServiceId_GetISRID,
	OSServiceId_GetResource,
	OSServiceId_GetScheduleTableStatus,
	OSServiceId_GetTaskID,
	OSServiceId_GetTaskState,
	OSServiceId_IncrementCounter,
	OSServiceId_NextScheduleTable,
	OSServiceId_ReleaseResource,
	OSServiceId_ResumeAllInterrupts,
	OSServiceId_ResumeOSInterrupts,
	OSServiceId_Schedule,
	OSServiceId_SetAbsAlarm,
	OSServiceId_SetEvent,
	OSServiceId_SetRelAlarm,
	OSServiceId_SetScheduletableAsync,
	OSServiceId_ShutdownOS,
	OSServiceId_StartOS,
	OSServiceId_StartScheduleTableAbs,
	OSServiceId_StartScheduleTableRel,
	OSServiceId_StartScheduleTableSynchron,
	OSServiceId_StopScheduleTable,
	OSServiceId_SuspendAllInterrupts,
	OSServiceId_SuspendOSInterrupts,
	OSServiceId_SyncScheduleTable,
	OSServiceId_TerminateTask,
	OSServiceId_WaitEvent,
	OSServiceId_WaitGetClearEvent,
} OSServiceIdType;

/** Error Record */
typedef struct {
	uint16 task_id;		/**< faulting Task or ISR */
	uint8 service_id;	/**< OSServiceIdType */
	uint8 error_code;	/**< Error Code */
	unsigned long arg1;
	unsigned long arg2;
	unsigned long arg3;
} OSErrorRecordType;

/** */
typedef enum {
	PRO_IGNORE = 0,
	PRO_TERMINATETASKISR,
	PRO_TERMINATEAPPL,
	PRO_TERMINATEAPPL_RESTART,
	PRO_SHUTDOWN,
} ProtectionReturnType;

#endif
