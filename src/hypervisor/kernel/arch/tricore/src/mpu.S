/*
 * mpu.S
 *
 * Tricore MPU handling
 *
 * azuepke, 2015-02-23: implementation
 */

#include <tc_regs.h>
#include <assembler.h>

	.text

/*
 * prepare MPU for next partition on partition switch
 * void arch_mpu_part_switch(const struct arch_mpu_part_cfg *cfg);
 */
FUNC_PROLOG(arch_mpu_part_switch)
	/* load nine task windows for data */
	ld.d	%e0, [%a4] 0
	mtcr	CSFR_DPRL(0), %d0
	mtcr	CSFR_DPRU(0), %d1

	ld.d	%e0, [%a4] 8
	mtcr	CSFR_DPRL(1), %d0
	mtcr	CSFR_DPRU(1), %d1

	ld.d	%e0, [%a4] 16
	mtcr	CSFR_DPRL(2), %d0
	mtcr	CSFR_DPRU(2), %d1

	ld.d	%e0, [%a4] 24
	mtcr	CSFR_DPRL(3), %d0
	mtcr	CSFR_DPRU(3), %d1

	ld.d	%e0, [%a4] 32
	mtcr	CSFR_DPRL(4), %d0
	mtcr	CSFR_DPRU(4), %d1

	ld.d	%e0, [%a4] 40
	mtcr	CSFR_DPRL(5), %d0
	mtcr	CSFR_DPRU(5), %d1

	ld.d	%e0, [%a4] 48
	mtcr	CSFR_DPRL(6), %d0
	mtcr	CSFR_DPRU(6), %d1

	ld.d	%e0, [%a4] 56
	mtcr	CSFR_DPRL(7), %d0
	mtcr	CSFR_DPRU(7), %d1

	ld.d	%e0, [%a4] 64
	mtcr	CSFR_DPRL(8), %d0
	mtcr	CSFR_DPRU(8), %d1

	/* load five task windows for code */
	ld.d	%e0, [%a4] 72
	mtcr	CSFR_CPRL(0), %d0
	mtcr	CSFR_CPRU(0), %d1

	ld.d	%e0, [%a4] 80
	mtcr	CSFR_CPRL(1), %d0
	mtcr	CSFR_CPRU(1), %d1

	ld.d	%e0, [%a4] 88
	mtcr	CSFR_CPRL(2), %d0
	mtcr	CSFR_CPRU(2), %d1

	ld.d	%e0, [%a4] 96
	mtcr	CSFR_CPRL(3), %d0
	mtcr	CSFR_CPRU(3), %d1

	ld.d	%e0, [%a4] 104
	mtcr	CSFR_CPRL(4), %d0
	mtcr	CSFR_CPRU(4), %d1

	/* no isync here: the final isync is in arch_mpu_task_switch instead */
	ret
FUNC_EPILOG(arch_mpu_part_switch)


/*
 * prepare MPU for next task on task switch
 * void arch_mpu_task_switch(const struct arch_mpu_task_cfg *cfg);
 */
FUNC_PROLOG(arch_mpu_task_switch)
	/* load the two task windows for data (no code here) */
	ld.d	%e0, [%a4] 0
	mtcr	CSFR_DPRL(9), %d0
	mtcr	CSFR_DPRU(9), %d1

	ld.d	%e0, [%a4] 8
	mtcr	CSFR_DPRL(10), %d0
	mtcr	CSFR_DPRU(10), %d1

	/* update write permissions */
	ld.w	%d1, [%a4] 16
	mov.u	%d15, 0xf000	/* hardcoded write permissions for the kernel */
	or		%d0, %d1, %d15
	mtcr	CSFR_DPWE(0), %d0
	mtcr	CSFR_DPWE(1), %d1

	isync
	ret
FUNC_EPILOG(arch_mpu_task_switch)
