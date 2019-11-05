# README_MPU.TXT
#
# azuepke, 2014-08-05: initial

MPU -- How Stuff Works
=======================

We assume that the MPU supports a fixed number of "windows", where each window
is defined by a start address, a size, and access permissions. Now, if the CPU
performs a memory access, the MPU unit compares the memory address, and if
the address matches one of the windows' defined memory regions and access types,
e.g. [start, start+size), the access is granted. If not, the MPU raises an
exception.

Depending on the implementation, an MPU could support distinct sets of windows
for data and instruction fetches, or impose alignment restrictions on start
address and region sizes. For overlapping memory regions with different access
permissions, an MPU could imply an ordering of the windows, so that first
matching window defines the actual access permission, or an implementation
could use the most permissive access right. An MPU implementation could further
distinguish between user and supervisor mode type of memory access.
Additionally, certain MPU implementations allow to partition a window into
a fixed number of equally sized subwindows. For each of these subwindows,
access permission could be set individually.

And, last but not least, MPUs can be placed at different positions in an SoC
(system-on-a-chip) architecture. Some MPUs can monitor CPU accesses, while
other MPUs can control memory transfers of DMA-capable devices.

Compared to MMUs, all MPUs have in common that they don't perform translation
of memory addresses from one address space to another. MPUs always operate
on physical addresses.

In summary, we can assume the following for any MPU implementation:
- we have a fixed number of windows
- windows are ordered or have a number
- each window describes a memory region with
  - a start address
  - a size or an according end address
  - access permissions, a triplet of RWX (read, write, and executable)
    for user mode and probably for supervisor mode as well
  - a valid bit to enable or disable the window

Typically, each window can be described in full detail in two to four
hardware specific registers. An example for ARM using three registers
is given below:

  struct arch_mpu_region {
    uint32_t base;              // Region Base Register
    uint32_t size_enable;       // Region Size and Enable Register
    uint32_t access_control;    // Region Access Control Register
  };


Now, we partition the set of MPU windows in 3 classes for different use cases:
1. We need windows for the kernel itself. This is a static set of windows
   that never changes after system boot.
2. We need windows to describe a partition's code and data. This set changes
   on every partition switch.
3. We need task specific windows for each task. Usually, these windows cover
   the task's stack and must be switched on every task switch.

To simplify the implementation, we should enforce that the number of per-task
windows is the same for all partitions. And we can safely assume that we can
cover a task's stack with one window.

So, an architecture implementation needs to consider the following constants
and data types, assuming 12 windows in the following example for ARM Cortex R4:

  #define ARCH_MPU_REGIONS_KERN   0
  #define ARCH_NUM_WINDOWS_PART  11
  #define ARCH_NUM_WINDOWS_TASK   1

  /** kernel specific MPU configuration */
  struct arch_mpu_kern_cfg {
    struct arch_mpu_region region[ARCH_MPU_REGIONS_KERN];
    // NOTE: data structure is not used by the kernel
    // NOTE: regions are usually hardcoded in the BSP!
  };

  /** partition specific MPU configuration */
  struct arch_mpu_part_cfg {
    struct arch_mpu_region region[ARCH_MPU_REGIONS_PART];
  };

  /** task specific MPU configuration */
  struct arch_mpu_task_cfg {
    struct arch_mpu_region region[ARCH_MPU_REGIONS_TASK];
  };


This means:
- We don't need any windows for the kernel, as the kernel virtually runs
  with MPU disabled. This is a feature of the Cortex R4 in this example.
- We have 11 windows that we switch on every partition switch.
- We have 1 window that we switch on every task switch.
- We need to keep the window configurations (arrays of struct arch_mpu_region)
  in both struct task_cfg and struct part_cfg.

Alternatively, we could use 4 windows for the kernel (code, data, I/O), further
6 windows for the partitions (code, data, I/O, shared memory), and the last 2
windows for the tasks (stack, per task accessible memory).

The exact format of these three data structures depend on the CPU architecture.


ARM: Task Stack Guarding by means of Subwindows
================================================

The ARM Cortex-R MPU implementation allows to partition a single window into
8 equally sized subwindows. Access to each of these subwindows can
individually be enabled/disabled. Disabling the very last subwindow of a task's
stack window may be a good approach to implement stack guarding.
