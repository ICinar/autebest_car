# README_POWERPC.TXT
#
# azuepke, 2014-06-03: initial

PowerPC Port
=============

The PowerPC port focues on Freescale's e200 cores.

The first implementation covers the e200z6 core, which uses
classic PowerPC instructions and an MMU. This core is used in the (now older)
MPC55xx and MPC56xx designs of Freescale or STM.

The upcoming MPC57xx cores use an e200z4 core, which only supported VLE
(variable length encoding), but brings a "Core MPU" instead of a TLB based MMU.


ABI
====

	r0		volatile
	r1		stack pointer
	r2		TLS pointer
	r3		arg0 | ret0
	r4		arg1 | ret1
	r5		arg2
	r6		arg3
	r7		arg4
	r8		arg5
	r9		arg6
	r10		arg7
	r11		volatile
	r12		volatile
	r13		small data area pointer				non-volatile
	r14		non-volatile
	...
	r30		non-volatile
	r31		frame pointer


	cr0..7	condition registers					cr2, cr3, cr4: non-volatile
	lr		link register						volatile
	ctr		count register						volatile
	xer		integer error register				volatile
	fpscr	FPU status and control registers	(partly)


- cr bit 6: variable argument list has FPU registers

- syscall number in r0

- stack frame
  - r1: 16 byte alignment
  - struct stack_frame {
        struct stack_frame *next;  // set by stwu r1, -16(r1)
        saved_lr;                  // set by stw  lr, 20(r1)
        param word 1
        param word 2
        ...
    };

