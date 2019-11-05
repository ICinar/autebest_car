/*
 * board.c
 *
 * Board initialization for QEMU Versatile Express Cortex A15.
 *
 * azuepke, 2013-11-19: initial cloned
 * azuepke, 2013-12-23: Beaglebone Black port
 * azuepke, 2014-05-06: adapted to MPU
 */

#include <kernel.h>
#include <assert.h>
#include <arm_insn.h>
#include <board.h>
#include <arm_private.h>
#include <arm_io.h>
#include <board_stuff.h>
#include <linker.h>
#include <sched.h>	/* num_cpus */
#include <hm.h>


/** AM335x specific reset */
static inline __cold void am335x_reset(void)
{
	/* set RST_GLOBAL_COLD_SW bit in PRM_RSTCTRL */
	/* PRM_DEVICE is at 44e0'0f00 */
	writel((volatile void *)(BOARD_IO1_PHYS + 0x0f00), 0x2);
}

__cold void board_halt(haltmode_t mode)
{
	if ((mode == BOARD_RESET) || (mode == BOARD_HM_RESET)) {
		am335x_reset();
	}

	/* else just halt the machine */
	__board_halt();
}

void board_idle(void)
{
	while (1) {
		arm_dsb();
		arm_wfi();
	}
}

void __init board_mpu_init(void)
{
	/* empty -- uses boot page table */
}

void __init board_cpu0_up(void)
{
}

void __init board_startup_complete(void)
{
}

void board_nmi_dispatch(unsigned int vector __unused)
{
	hm_system_error(HM_ERROR_NMI, vector);
}

int board_hm_exception(
	struct arch_reg_frame *regs __unused,
	int fatal __unused,
	unsigned int hm_error_id __unused,
	unsigned long vector __unused,
	unsigned long fault_addr __unused,
	unsigned long aux __unused)
{
	return 0;	/* exception not handled */
}

void board_tp_switch(
	unsigned int prev_timepart __unused,
	unsigned int next_timepart __unused,
	unsigned int tpwindow_flags __unused)
{
}

void __init board_init(void)
{
	serial_init(115200);
	printf("Starting up ...\n");

	printf("assuming ROM from %08x to %08x\n", BOARD_ROM_PHYS, BOARD_ROM_PHYS + BOARD_ROM_SIZE);
	printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
	printf("assuming RAM from %08x to %08x\n", BOARD_RAM_PHYS, BOARD_RAM_PHYS + BOARD_RAM_SIZE);
	printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
	printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);

	intc_init();
	dmtimer_init(20);	/* HZ -- FIXME: can't exceed 20 HZ on QEMU */

	/* enter the kernel */
	/* NOTE: all processors take the same entry point! */
	kernel_main(0);
}
