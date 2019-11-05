/*
 * Os_Api.h
 *
 * AUTOSAR OS library API declarations
 *
 * This is an internal header for the OS implementation; OS users should include Os.h only.
 *
 * azuepke, 2013-12-02: initial
 */

#ifndef OS_API_H
#define OS_API_H

#include <Os_Types.h>

/* pull in kernel error codes */
#include <hv_error.h>

#define DeclareTask(TaskName)   extern void _Task_ ## TaskName(void)

/* don't need these, so let them expand to something that doesn't result in a compiler error */
#define DeclareResource(ResourceIdentifier)  extern StatusType ActivateTask(TaskType TaskID)
#define DeclareEvent(EventId)  extern StatusType ActivateTask(TaskType TaskID)
#define DeclareAlarm(AlarmIdentifier)  extern StatusType ActivateTask(TaskType TaskID)

#define TASK(TaskName)  void _Task_ ## TaskName(void)
#define ISR(IsrName)    void _Isr_ ## IsrName(void)

StatusType ActivateTask(TaskType TaskID);
StatusType TerminateTask(void);
StatusType ChainTask(TaskType TaskID);
StatusType Schedule(void);
StatusType GetTaskID(TaskRefType TaskIDRef);
StatusType GetTaskState(TaskType TaskID, TaskStateRefType StateRef);

void DisableAllInterrupts(void);
void EnableAllInterrupts(void);
void SuspendAllInterrupts(void);
void ResumeAllInterrupts(void);
void SuspendOSInterrupts(void);
void ResumeOSInterrupts(void);

StatusType SetEvent(TaskType TaskID, EventMaskType Mask);
StatusType GetEvent(TaskType TaskID, EventMaskRefType MaskRef);
StatusType WaitEvent(EventMaskType Mask);
StatusType ClearEvent(EventMaskType Mask);
StatusType WaitGetClearEvent(EventMaskType WaitMask, EventMaskType ClearMask, EventMaskRefType MaskRef);

StatusType GetResource(ResourceType ResID);
StatusType ReleaseResource(ResourceType ResID);

void StartOS(AppModeType appmode);
void ShutdownOS(StatusType error);
AppModeType GetActiveApplicationMode(void);

StatusType GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType InfoRef);
StatusType GetAlarm(AlarmType AlarmID, TickRefType TickRef);
StatusType SetRelAlarm(AlarmType AlarmID, TickType Increment, TickType Cycle);
StatusType SetAbsAlarm(AlarmType AlarmID, TickType Start, TickType Cycle);
StatusType CancelAlarm(AlarmType AlarmID);

StatusType IncrementCounter(CounterType CounterID);
StatusType GetCounterValue(CounterType CounterID, TickRefType ValueRef);
StatusType GetElapsedValue(CounterType CounterID, TickRefType ValueRef, TickRefType ElapsedValueRef);

StatusType StartScheduleTableRel(ScheduleTableType ScheduleTableID, TickType Offset);
StatusType StartScheduleTableAbs(ScheduleTableType ScheduleTableID, TickType Start);
StatusType StopScheduleTable(ScheduleTableType ScheduleTableID);
StatusType NextScheduleTable(ScheduleTableType ScheduleTableID_From, ScheduleTableType ScheduleTableID_To);
StatusType StartScheduleTableSynchron(ScheduleTableType ScheduleTableID);
StatusType SyncScheduleTable(ScheduleTableType ScheduleTableID, TickType Value);
StatusType SetScheduletableAsync(ScheduleTableType ScheduleTableID);
StatusType GetScheduleTableStatus(ScheduleTableType ScheduleTableID, ScheduleTableStatusRefType ScheduleStatusRef);

ISRType GetISRID(void);

int main(void);

/* user supplied */
void ErrorHook(StatusType error);
ProtectionReturnType ProtectionHook(StatusType Fatalerror);
void ShutdownHook(StatusType error);

/** Pointer to the currently active error record.
 *  If not NULL, the ErrorHook is currently active.
 */
extern OSErrorRecordType *__OsErrorRecord;

/* _OsErrorGet*.c */
/** Get Service ID of last error */
static inline OSServiceIdType _OSErrorGetServiceId(void)
{
	return __OsErrorRecord->service_id;
}

/** Get Argument 1 of last error */
static inline unsigned long _OSErrorGetArg1(void)
{
	return __OsErrorRecord->arg1;
}

/** Get Argument 2 of last error */
static inline unsigned long _OSErrorGetArg2(void)
{
	return __OsErrorRecord->arg2;
}

/** Get Argument 3 of last error */
static inline unsigned long _OSErrorGetArg3(void)
{
	return __OsErrorRecord->arg3;
}

/* wrappers to service call ID */
#define OSErrorGetServiceId()           _OSErrorGetServiceId()

/* wrappers to service call arguments */
#define OSError_ActivateTask_TaskID()   ((TaskType)_OSErrorGetArg1())

#define OSError_ChainTask_TaskID()      ((TaskType)_OSErrorGetArg1())

#define OSError_GetTaskID_TaskIDRef()   ((TaskRefType)_OSErrorGetArg1())

#define OSError_GetTaskState_TaskID()   ((TaskType)_OSErrorGetArg1())
#define OSError_GetTaskState_StateRef() ((TaskStateRefType)_OSErrorGetArg2())

#define OSError_SetEvent_TaskID()       ((TaskType)_OSErrorGetArg1())
#define OSError_SetEvent_Mask()         ((EventMaskType)_OSErrorGetArg2())

#define OSError_GetEvent_TaskID()       ((TaskType)_OSErrorGetArg1())
#define OSError_GetEvent_MaskRef()      ((EventMaskRefType)_OSErrorGetArg2())

#define OSError_WaitEvent_Mask()        ((EventMaskType)_OSErrorGetArg1())

#define OSError_ClearEvent_Mask()       ((EventMaskType)_OSErrorGetArg1())

#define OSError_WaitGetClearEvent_WaitMask()    ((EventMaskType)_OSErrorGetArg1())
#define OSError_WaitGetClearEvent_ClearMask()   ((EventMaskType)_OSErrorGetArg2())
#define OSError_WaitGetClearEvent_MaskRef()     ((EventMaskRefType)_OSErrorGetArg3())

#define OSError_GetResource_ResID()     ((ResourceType)_OSErrorGetArg1())

#define OSError_ReleaseResource_ResID() ((ResourceType)_OSErrorGetArg1())

#define OSError_GetAlarmBase_AlarmID()  ((AlarmType)_OSErrorGetArg1())
#define OSError_GetAlarmBase_InfoRef()  ((AlarmBaseRefType)_OSErrorGetArg2())

#define OSError_GetAlarm_AlarmID()      ((AlarmType)_OSErrorGetArg1())
#define OSError_GetAlarm_TickRef()      ((TickRefType)_OSErrorGetArg2())

#define OSError_SetRelAlarm_AlarmID()   ((AlarmType)_OSErrorGetArg1())
#define OSError_SetRelAlarm_Increment() ((TickType)_OSErrorGetArg2())
#define OSError_SetRelAlarm_Cycle()     ((TickType)_OSErrorGetArg3())

#define OSError_SetAbsAlarm_AlarmID()   ((AlarmType)_OSErrorGetArg1())
#define OSError_SetAbsAlarm_Start()     ((TickType)_OSErrorGetArg2())
#define OSError_SetAbsAlarm_Cycle()     ((TickType)_OSErrorGetArg3())

#define OSError_CancelAlarm_AlarmID()   ((AlarmType)_OSErrorGetArg1())

#define OSError_IncrementCounter_CounterID()   ((CounterType)_OSErrorGetArg1())

#define OSError_GetCounterValue_CounterID()    ((CounterType)_OSErrorGetArg1())
#define OSError_GetCounterValue_ValueRef()     ((TickRefType)_OSErrorGetArg2())

#define OSError_GetElapsedValue_CounterID()         ((CounterType)_OSErrorGetArg1())
#define OSError_GetElapsedValue_ValueRef()          ((TickRefType)_OSErrorGetArg2())
#define OSError_GetElapsedValue_ElapsedValueRef()   ((TickRefType)_OSErrorGetArg3())


#define OSError_StartScheduleTableRel_ScheduleTableID() ((ScheduleTableType)_OSErrorGetArg1())
#define OSError_StartScheduleTableRel_Offset()          ((TickType)_OSErrorGetArg2())

#define OSError_StartScheduleTableAbs_ScheduleTableID() ((ScheduleTableType)_OSErrorGetArg1())
#define OSError_StartScheduleTableAbs_Start()           ((TickType)_OSErrorGetArg2())

#define OSError_StopScheduleTable_ScheduleTableID()     ((ScheduleTableType)_OSErrorGetArg1())

#define OSError_NextScheduleTable_ScheduleTableID_From()    ((ScheduleTableType)_OSErrorGetArg1())
#define OSError_NextScheduleTable_ScheduleTableID_To()      ((ScheduleTableType)_OSErrorGetArg2())

#define OSError_StartScheduleTable_ScheduleTableID()    ((ScheduleTableType)_OSErrorGetArg1())

#define OSError_SyncScheduleTable_ScheduleTableID()     ((ScheduleTableType)_OSErrorGetArg1())
#define OSError_SyncScheduleTable_Value()               ((TickType)_OSErrorGetArg2())

#define OSError_SetScheduletableAsync_ScheduleTableID() ((ScheduleTableType)_OSErrorGetArg1())

#define OSError_GetScheduletableAsync_ScheduleTableID()     ((ScheduleTableType)_OSErrorGetArg1())
#define OSError_GetScheduletableAsync_ScheduleStatusRef()   ((ScheduleTableStatusRefType)_OSErrorGetArg2())

#endif
