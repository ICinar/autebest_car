/*
 * README.TXT
 *
 * Various thoughts
 *
 * azuepke, 2013-07-17: initial
 */

/*
 * Assumed limitations
 * - <16 CPUs
 * - <64 resource partitions
 * - <64 time partitions
 * - <256 threads in the system (otherwise adjust uint8_t data type)
 * - <256 priorities (uint8_t type)
 * - <256 time partition windows (uint8_t type)
 * - <512 ports (uint16_t type) (ARINC requires at least 512 channels)
 * - <512 channels (uint16_t type)
 * - <64K message size (ARINC requires only 8K, I think)
 */

/*
 * NOTE:
 *  As all objects in the kernel exist over the kernel's lifetime,
 *  no object can go away and pointers remain valid all the time!
 */

/*
 * NOTE:
 *  All kernel data is represented by fixed-sized arrays which are initialized
 *  at compile time. If possible, the representation uses indices into these
 *  arrays to save valuable RAM.
 *  To save more RAM, most data structures are split into two parts,
 *  where the _cfg part holds configuration data and is placed in flash or ROM,
 *  and the _dyn part for the dynamic runtime data placed in RAM.
 */

/*
 * NOTE:
 *  For partitioning reasons, partitions gets their own "namespaces",
 *  namely indices into a partition's arrays, starting from 0
 *  to the partition's resource limit.
 *  Unprivileged partitions cannot access other partition's resources.
 */


Design Decisions
=================

- we opt for "run-to-completion" semantics in the kernel
  -> single kernel stack
- the kernel itself is non preemptive
  -> kernel code runs with interrupts disabled
- on SMP, fine granular locking is applied
  -> spin locks, but on SMP only

-> these overall design decisions keep the kernel itself simple

- continuation blocks
  - the kernel defines continuation blocks for kernel activity that would
    block inside the kernel
  - each thread has an associated continuation block
  - if a "wait" condition is set in the continuation block,
    the system call blocks when leaving the kernel

- system calls into the kernel are always considered to be "returning"
  - save minimal registers on syscall entry
  - syscall in C (clobbers r0..r3, r12, lr)
  - on return, check continuation flag
    - save remaining registers (non-volatile regs)
    - call the scheduler (in C) on the standard kernel stack
      - the scheduler does not return, but returns via longjmp
    - restore remaining registers (non-volatile regs)
  - restore volatile registers on syscall exit

- interrupts -- same concept, make them fast
  - save volatile registers on syscall entry
  - handle IRQ in C (clobbers r0..r3, r12, lr)
  - on return, check continuation flag
    - save remaining registers (non-volatile regs)
    - call the scheduler (in C) on the standard kernel stack
      - the scheduler does not return, but returns via longjmp
    - restore remaining registers (non-volatile regs)
  - restore volatile registers on syscall exit

- exceptions
  - save all registers
  - handle in C (clobbers whatever)
  - restore all registers

- continuations == match the syscall save area
  - special kernel stack on syscall/exception to save the regs
  - normal kernel stack for C function like syscalls, the scheduler, IRQ handling

- continuation on ARM
  - tls2 keeps a pointer to the current / C-stack
  - we need to have that in a register on SMP anyway, cannot use hardcoded values

!!!!!!IGNORE THIS LINE!!!!!!
