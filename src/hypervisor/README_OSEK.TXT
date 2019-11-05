# README_OSEK.TXT
#
# tjordan, 2014-09-09: initial

Building OSEK/AUTOSAR partitions
=================================

The hypervisor kernel provides basic functionality to an OSEK partition,
e.g. task scheduling conforming to OSEK requirements, events, alarms,
AUTOSAR schedule tables and so on.
However, some functionality found in a typical OSEK system must be
implemented in user space. This is covered by the OSEK library (libos).
As AUTOSAR and OSEK systems are highly configurable, the OSEK library
needs configuration that must be provided by the partition.

Workflow
=========

Inputs:
- hypervisor configuration (gen_config.xml) with a partition for which
  objects will be replaced (initially, this partition may be empty)
- AUTOSAR OS configuration (e.g. as AUTOSAR EPC)

1. Generate hypervisor/OSEK library configuration out of AUTOSAR configuration
   - for all AUTOSAR OS objects, generate matching hypervisor configuration
   - map priorities from AUTOSAR configuration to partition-specific ones
   - do stack optimization, generate symbols for stacks
   -> Output: hypervisor configuration (gen_config.xml) including configuration
      of OSEK partition

2. Generate OSEK library .c/.h configuration files
   - using code generator, transform the OSEK library specific parts of the
     hypervisor configuration to C code
   -> Intermediate Output: C Code for OSEK library

3. Compile and link application (including OSEK library configuration objects)
   -> Output: application binary

Outputs:
- hypervisor configuration (gen_config.xml) including configuration of OSEK
  partition
- application binary

OSEK library configuration in XML
==================================

The OSEK library shares some configuration with the hypervisor (e.g. task
names, stack slots used for tasks). For these parts of the configuration, the
code generator directly uses the corresponding hypervisor configuration parts.
Additional configuration items (e.g. stack sizes, autostart configuration) are
placed in <osek> tags within the elements that define them.

The following description is loosely based on the order of the AUTOSAR OS SWS.

Alarm configuration
--------------------
Example:
    <alarm name="bench_alarm1" counter="bench_counter2">
      <action_hook partition="part1" hook="bench4"/>
      <osek>
        <autostart appmode="OSDEFAULTAPPMODE" alarmtime="20" cycletime="100" relative="yes"/>
      </osek>
    </alarm>
TODO

Appmode configuration
----------------------
Example:
    <appmode name="OSDEFAULTAPPMODE"/>

Requirements:
* an appmode tag must be present for all configured appmodes
* appmode/@name must be set to the name of the appmode

Counter configuration
----------------------
Example:
(global)
    <counter name="c1" type="sw" maxallowedvalue="255" ticksperbase="10" mincycle="1"/>
(local)
    <counter_access counter="c1"/>

Requirements:
* a counter tag must be present on global level for each configured counter
* inside the OSEK partition, a counter_access tag must be present for each
  counter that is used by the partition
* counter_access/@counter must be set to the name of the counter

Event configuration
--------------------
Example:
    <event name="e1"/>
    <event name="e2" mask="4"/>

Requirements:
* an event tag must be present for every configured event
* event/@name must be set to the name of the event
* event/@mask must be set to the decimal integer representation of the event's
  mask if a event mask is exlicitly specified in the OSEK configuration
* event/@mask must not be present if the OSEK configuration omits the event's
  mask or if the mask in the OSEK configuration is set to AUTOSAR

Hook configuration
-------------------

Requirements:
* hook/invoke/@entry must be set to "__os_hook_invoke" for Hooks.

TODO

ISR configuration
------------------

Requirements:
* isr/invoke/@entry must be set to "__os_isr_invoke" for ISRs.

TODO

Generic OS configuration
-------------------------
TODO, currently none - hooks?

Resource configuration
-----------------------
Example:
    <resource name="r1" prio="102"/>

Requirements:
* a resource tag must be present for all LINKED and STANDARD resources
* resource/@name must be set to the name in the OSEK configuration
* resource/@prio must be set to the maximum priority of all tasks and ISRs
  sharing the resource for STANDARD resources
* resource/@prio must be set to the prio attribute of the linked STANDARD
  resource for LINKED resources
* if the AUTOSAR configuration parameter OsUseResScheduler is set to true, the
  generation tool may decide to automatically generate a resource named
  RES_SCHEDULER. this resource must have the maximum priority of all tasks, and
  every task must have a task/osek/resourceaddress tag referencing this
  resource. See also chapters 7.1.2.1 and 12.8 of AUTOSAR_SWS_OS.pdf, v5.0.0,
  which include contradicting requirements about this behaviour.

Schedule table configuration
-----------------------------
TODO

Task configuration
-------------------
Example:
    <task name="TestControlTask" cpu="0" prio="100" eprio="198" regs="yes" fpu="no" blocking="no">
      <invoke entry="__os_task_invoke" arg="_Task_TestControlTask" stack="/__stack1 - 16"/>
      <osek stacksize="150">
        <eventaccess event="e1"/>
        <resourceaccess resource="r1"/> <!-- currently unused -->
        <autostart appmode="OSDEFAULTAPPMODE"/>
      </osek>
    </task>

Requirements:
* a task tag must be present for all OSEK tasks
* task/@name must be set to the name of the task in the OSEK configuration
* task/invoke/@arg must be set to the name of the task in the OSEK
  configuration, prefixed with "_Task_"
* task/@prio must be set to the queueing priority of the task. this correlates
  directly with the priority in the OSEK configuration (see below about mapping
  priorities)
* task/@eprio must be set if the task is configured to use an internal
  resource. it must be set to the maximum priority of all tasks and ISRs
  sharing this internal resource.
* task/@eprio must be set if the task is configured to be non-preemptive. it
  must be set to the maximum priority of all tasks.
* task/@regs must be set to "yes" // FIXME: WIP - see stack sharing?
* task/@fpu must be set to "yes" if the task accesses the FPU. as there is no
  AUTOSAR parameter to determine this, a vendor specific parameter should be
  used.
* task/@blocking must be set to "yes" if the task is an EXTENDED task in the
  OSEK configuration
* task/@blocking must be set to "no" if the task is an BASIC task in the OSEK
  configuration
* task/invoke/@entry must be set to "__os_task_invoke" for tasks.
* task/invoke/@stack must be set to the start of the task's stack (see below
  about allocating and optimizing stacks).
* task/osek must be present for all OSEK tasks.
* task/osek/@stacksize must be set to the stack size required by the task. as
  there is no AUTOSAR parameter to determine this, a vendor specific parameter
  should be used.
* task/osek/eventaccess must be present for every event that's used by the task
* task/osek/eventaccess/@event must reference an event
* task/osek/resourceaccess must be present for every resource that is accessed
  by the task
* task/osek/resourceaccess/@resource must reference a resource
* task/osek/autostart must be present for every application mode the task is
  started in.
* task/osek/autostart/@appmode must reference an application mode

// FIXME: WIP startup task / init hook?

Mapping/allocating AUTOSAR priorities
--------------------------------------
The hypervisor provides an upper limit for a partition's priorities by
specifying the 'max_prio' attribute of a partition. To ease migration of
existing configurations, it may be needed to transform the priority range
used in the AUTOSAR configuration to the range used in the hypervisor's
partition configuration.

In order to fulfil OSEK semantics, priorities should be allocated in the
following order, highest to lowest:
* prio_max must be reserved for ErrorHook/ProtectionHook // FIXME: WIP
* cat1 ISRs // FIXME: do we support these? see ISR configuration above
* cat2 ISRs
* tasks

Allocating/optimizing AUTOSAR stack slots
------------------------------------------
// FIXME: the following is also valid for the register context - add these

To reduce runtime resource consumption, stacks of OSEK objects may be
optimized. This means that tasks/ISRs that can't interrupt each other may
share the same stack. As a first start, only tasks are considered for
optimization.
Apart from the benefit of reducing RAM usage, stack optimization has some
downsides:
- stack usage can't be determined exactly for a specific task any more if its
  stack is shared with another task.
- security: a task may read stack data of another task if their stack is
  shared.

Consequently, stack optimization should be configurable. As there is no
AUTOSAR parameter to configure this, a vendor specific parameter should be
used.

Two tasks can't interrupt each other if all of the following conditions are
true:
* both have the same running priority ("eprio" attribute)
* neither of them calls WaitEvent() - i.e. neither of them is an extended
  task
* neither of them calls Schedule() - this can't be determined from the AUTOSAR
  configuration, so a vendor specific parameter should be used here.


Open points
============
- Is it a use case to pinpoint task/ISR priorities manually?
  - If yes, per-partition or inter-partition?
  - How do we cope with this in our tools?
- Is it a use case to have non-OSEK tasks/ISRs/objects in an OSEK partition?
  - If yes, how do we ensure these extras don't get overwritten by a new import
    of the AUTOSAR configuration?
- Do we want to be able to export AUTOSAR configurations?
  - If yes, how do we store mapping information?

