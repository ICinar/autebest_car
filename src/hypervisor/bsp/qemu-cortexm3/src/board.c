/*
 * board.c
 *
 * Board initialization for Cortex-M3/M4
 *
 * azuepke, 2015-06-26: cloned from QEMU ARM
 */

#include <kernel.h>
#include <assert.h>
#include <arm_insn.h>
#include <board.h>
#include <arm_private.h>
#include <arm_io.h>
#include <board_stuff.h>
#include <nvic.h>
#include <linker.h>
#include <sched.h>	/* num_cpus */
#include <hm.h>


__cold void board_halt(haltmode_t mode __unused)
{
	if ((mode == BOARD_RESET) || (mode == BOARD_HM_RESET)) {
		/* trigger reset */
		AIRCR = AIRCR_VECTKEY | AIRCR_SYSRESETREQ;
		arm_dsb();
	}

	/* just halt the machine */
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
	/* on Cortex-M3/M4, the kernel runs with MPU disabled */
	MPU_CTRL = MPU_CTRL_PRIVDEFENA | MPU_CTRL_HFNMIENA | MPU_CTRL_ENABLE;
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

/** kernel entry function */
void __init board_init(void)
{
	/* set vector table to flash start */
	VTOR = BOARD_ROM_PHYS;

	/* setup caches, clocks, etc */

	serial_init(38400);
	printf("Starting up ...\n");

	printf("assuming ROM from %08x to %08x\n", BOARD_ROM_PHYS, BOARD_ROM_PHYS + BOARD_ROM_SIZE);
	printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
	printf("assuming RAM from %08x to %08x\n", BOARD_RAM_PHYS, BOARD_RAM_PHYS + BOARD_RAM_SIZE);
	printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
	printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);

	nvic_irq_init();
	nvic_timer_init(20);	/* HZ -- FIXME: can't exceed 20 HZ on QEMU */

	/* enter the kernel */
	/* NOTE: all processors take the same entry point! */
	kernel_main(0);
}
