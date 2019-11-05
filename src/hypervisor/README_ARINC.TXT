# README_ARINC.TXT -- thoughts on ARINC 653 support
#
# azuepke, 20131205: initial
# azuepke, 20140916: API spec

********************************************************************************

ARINC 653 Part 1 Supplement 3 -- Required Services -- November 2010

* System Limits

  32 partitions
 128 processes per partition
 512 queueing ports
 512 sampling ports
 512 messages
8192 byte per message
 256 semaphores
 256 events
 256 buffers
 256 blackboards



* Naming:
- process = task
- dormant = suspended


* ARINC Stuff

- at partition startup time, only one process runs (the init hook)
  - a partition starts in COLD_START or WARM_START mode
  - partition start finishes by a switch to NORMAL mode
  - the init hook may terminate (we let it terminate) on switch to NORMAL mode

- at partition startup time, all processes and resources are "created"
  - we match the dynamic settings against the configuration
  -> error on mismatch
  - no resource creation in NORMAL mode possible -> checked in user space

- resources
  - processes -> tasks
  - error handler process -> special process -> dedicated hook
  - queuing ports -> SHM + two wait queues between partitions
  - sampling ports -> ?
  - buffers -> private SHM + two wait queues
  - blackboards -> ?
  - semaphores -> one wait queue linked to itself
  - events -> one wait queue linked to itself


* Process States
  ARINC     OSEK
 =======   ======
  DORMANT   SUSPENDED
  READY     READY
  RUNNING   RUNNING
  WAITING   WAITING

- most waiting calls support a timeout or have special constraints to consider
- more state transitions than in OSEK

- no concept for ISRs or HOOKs

- processes are categorized as PERIODIC or APERIODIC
  - PERIODIC processes have a period (multiple of partition period)

- processes can have an optional deadline


* API Calls
- CREATE_PROCESS(attr, &task_id)
  - attr:
    - name
    - entry_point
    - stack_size
    - base_priority
    - period:           activation period (if >= 0)
    - time_capacity:    deadline (if >= 0)
    - deadline_type:    HARD / SOFT
  - process created in DORMANT == SUSPENDED state
  - match attributes in lib internally

- GET_PROCESS_ID(name, &task_id);
  - translate name -> task_id
  - do that in lib

- GET_MY_ID(&task_id);
  - return caller's task ID
  - returns an error for the error handler

- GET_PROCESS_STATUS(task_id, &status);
  - status:
    - attr (see above)
    - state (DORMANT, READY, RUNNING, WAITING)
    - current deadline time
    - current priority

- SET_PRIORITY(task, new_prio)
  - change current priority of a task
  - task already enqueued at the END of its ready queue
  - error in DORMAT state

- LOCK_PREEMPTION(&lock_level)
- UNLOCK_PREEMPTION(&lock_level)
  - increment / decrement "lock level" -> similar to RES_SCHEDULER
  - not by error handler
  - not if partition is in COLD_START or WARM_START
  - can be nested

- DELAYED_START(task_id, delay_time)
- START(task_id) -> (implicit delay_time == 0)
  - transition DORMAT -> (WAITING for ACTIVATION) -> READY
    - reset entry point and stack pointer
    - reset task to base priority
    - if delay > 0: set task to WAITING for ACTIVATION
  - all START activities are delayed until the partition enters NORMAL mode
   - processes enter WAITING_ACT instead
  - on entry to NORMAL mode, or if called in NORMAL mode directly:
    - reference_start_time:
       - set to current_time() for APERIODIC processes
       - set to next_periodic_act-time for PERIODIC processes
    - if delay_time > 0:
      - let process sleep until reference_start_time + delay_time in WAITING_ACT
    - if delay_time == 0 or WAITING_ACT completes:
      - enter READY state
      - set new deadline := now + time_capacity

- STOP_SELF()
  - reset "lock level" to zero (if not error handler)
  - disables any deadline processing
  - terminate calling process, transition CURRENT -> DORMANT

- STOP(task_id)
  - terminate other process (not the caller)
  - transition READY / WAITING -> DORMANT
  - removes the other process from any waiting queues
  - disables any deadline processing

- SUSPEND_SELF(time_out)
  - special "sleep" for APERIODIC processes
  - not for error handler or if "lock_level" > 0
  - time_out == 0:  nothing happens
  - time_out > 0:   wait given duration
  - transition: RUNNING -> WAITING_SUSPENDED -> READY

- SUSPEND(task_id)
  - put other task to sleep, only for APERIODIC processes
  - not if "lock_level" > 0 of the calling process (if called from error handler)
  - cannot target error handler
  - no timeout here
  - transition: READY | WAITING -> WAITING_SUSPENDED
    - NOTE: uses a sticky flag to indicate the transition to SUSPENDED
  - the target task remains enqueued in all synchronization objects! <<<<<<<<<<<

- RESUME(task_id)
  - wake a task in WAITING_SUSPENDED state or clear sticky flag


- GET_TIME(&system_time);
  - get current system time in 64-bit nanoseconds since boot

- TIMED_WAIT(delay_time);
  - lock_level must be 0
  - not called by the error handler
  - sleep for "delay_time" (relative timeout in nanoseconds)
  - yield if delay_time == 0
  - transition CURRENT -> WAIT_SLEEP -> READY

- PERIODIC_WAIT()
  - lock_level must be 0
  - not called by the error handler
  - sleep until next periodic release point
  - task->last_activation += task->cfg->period
  - transition CURRENT -> WAIT_ACT -> READY

- REPLENISH(budget_time)
  - push deadline into the future, but not beyond next periodic activation point
  - deadline = current_time() + budget_time
  - not by error handler
  - not if partition is in COLD_START or WARM_START


* Summary Deadline handling
  - START / DELAYED_START starts a new deadline
  - STOP always resets the deadline
  - PERIODIC_WAIT() resets the deadline (on next wakeup) for PERIODIC processes
  - REPLENISH moves the deadline into the future,
    but not beyond the point of next period activation for periodic tasks


* Summary SUSPEND() / SUSPEND_SELF() / RESUME()
  - only for APERIODIC processes
  - SUSPEND_SELF():
    - set FLAG_SUSPENDED bit on self
    - call TIMED_WAIT() with timeout
  - SUSPEND:
    - if task == READY -> set to WAITING_SUSPENDED state
    - else: set FLAG_SUSPENDED bit on task
  - RESUME:
    - clear FLAG_SUSPENDED if set
    - else if task is WAITING_SUSPENDED -> make it ready
  - prone for deadlocks!


* Health-Monitoring (HM) API

- CREATE_ERROR_HANDLER(entry_point, stack_size);
  - create "nameless" process for error handling
  - we match this to the error hook
  - NOTE: can only call STOP_SELF to terminate itself!

- GET_ERROR_STATUS(&status);
  - error handler fetches next error

- RAISE_APPLICATION_ERROR(error_code, msg, len)
  - pass error with message "one level up"
    - if called from processes -> notify error handler
    - if called from error handler -> notify partition HM

- REPORT_APPLICATION_MESSAGE(msg, len)
  - arbitrary logging: user application or error handler can log a message


* Partitioning API

- GET_PARTITION_STATUS(&status)
  - get status:
    - partition ID
    - partition period (time in nanoseconds)
    - partition duration (time in nanoseconds)
    - lock_level
    - operating_mode
    - start_condition:
      - NORMAL_START
      - PARTITION_RESTART
      - HM_MODULE_RESTART
      - HM_PARTITION_RESTART

- SET_PARTITION_MODE(operating_mode)
  - transitions:
    - COLD_START / WARM_START   ->   NORMAL                     ready
    - COLD_START / WARM_START   ->   IDLE                       halt
    - NORMAL                    ->   COLD_START / WARM_START    reset
    - NORMAL                    ->   IDLE                       halt


* Queuing Ports / Buffers
  - Buffers are partition local queuing ports
  - create once, remain open until partition restart
  - waiting with timeout on both sender and receiver side
  - message queue is kept in SHM


- CREATE_QUEUING_PORT(name, msg_size, num_msg, direction, discipline, &port_id)
  - open a queuing port

- GET_QUEUING_PORT_ID(name, &port_id)
  - translate name to port_id

- GET_QUEUING_PORT_STATUS(port_id, &status)
  - get status ...

- SEND_QUEUING_MESSAGE(port_id, msg, len, timeout);
- RECEIVE_QUEUING_MESSAGE(port_id, msg, *len, timeout);
  - send and receive a message, with timeout

- CLEAR_QUEUING_PORT(port_id)
  - reset all messages on receiver side

- same API for buffers, except CLEAR_QUEUING_PORT()


* Sampling Ports / Blackboards
  - Blackboards are partition local sampling ports
  - create once, remain open until partition restart
  - messages are kept in SHM
  - each message has a validity flag if the message is "old" or "new"
  - after a configured REFRESH_PERIOD expires, a new messages becomes old

- CREATE_SAMPLING_PORT(name size, direction, refresh_period, &port_id);
  - create sampling port

- GET_SAMPLING_PORT_ID(name, &port_id)
  - translate name to port_id

- GET_SAMPLING_PORT_STATUS(port_id, &status)
  - get status ...

- WRITE_SAMPLING_MESSAGE(port_id, msg, len);
- READ_SAMPLING_MESSAGE(port_id, msg, *len, *validity);
  - write or read sampling messages
  - reading returns validity of the message


* Blackboard API exceptions:
- READ_BLACKBOARD(port_id, timeout, addr, *len)
  - read with timeout, no validity here
- CLEAR_BLACKBOARD(port_id)
  - clear blackboard


* Semaphore API
  - only used inside a partition


- CREATE_SEMAPHORE(name, start_value, max_value, discipline, &sem_id);
  - create semaphore

- GET_SEMAPHORE_ID(name, &sem_id)
  - translate name to sem_id

- GET_SEMAPHORE_STATUS(sem_id, &status)
  - get status ...

- WAIT_SEMAPHORE(sem_id, timeout)
  - wait on semaphore // decrement

- SIGNAL_SEMAPHORE(sem_id)
  - signal semaphore // increment



* Event API
  - only used inside a partition
  - an event encodes "UP" and "DOWN"


- CREATE_EVENT(name, &ev_id);
  - create event

- GET_EVENT_ID(name, &ev_id)
  - translate name to ev_id

- GET_EVENT_STATUS(ev_id, &status)
  - get status ...

- SET_EVENT(ev_id);
  - set event to UP

- RESET_EVENT(ev_id);
  - set event to DOWN

- WAIT_EVENT(ev_id, timeout);
  - wait for event to become UP


********************************************************************************

ARINC 653 Part 4 -- Subset Services -- June 2012

Part 4 versus Part 1 restrictions:
- partition scheduling is restricted to only one window
- only two processes: one periodic, one aperiodic (both optional)
- the periodic process has higher priority
- deadline:
  - effective time capacity of the periodic process == partition period
  - no deadline monitoring for the aperiodic process
- no semaphores, no events, no blackboards, no buffers
- only non-blocking communication, no wait queues, queuing discipline ignored
- no locking of preemption
- no process error handler

* API Calls
- CREATE_PROCESS(attr, &task_id)
- START(task_id) -> (implicit delay_time == 0)
- GET_TIME(&system_time);
- PERIODIC_WAIT()

* Partitioning API
- GET_PARTITION_STATUS()
- SET_PARTITION_MODE()

* Health-Monitoring (HM) API
- RAISE_APPLICATION_ERROR(error_code, msg, len)
- REPORT_APPLICATION_MESSAGE(msg, len)

* Queuing Ports
- CREATE_QUEUING_PORT(name, msg_size, num_msg, direction, discipline, &port_id)
- GET_QUEUING_PORT_STATUS(port_id, &status)
- SEND_QUEUING_MESSAGE(port_id, msg, len, timeout);
- RECEIVE_QUEUING_MESSAGE(port_id, msg, *len, timeout);
- CLEAR_QUEUING_PORT(port_id)

* Sampling Ports
- CREATE_SAMPLING_PORT(name size, direction, refresh_period, &port_id);
- WRITE_SAMPLING_MESSAGE(port_id, msg, len);
- READ_SAMPLING_MESSAGE(port_id, msg, *len, *validity);
