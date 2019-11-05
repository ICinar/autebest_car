/*
 * board.c
 *
 * Board initialization, ARM specific.
 *
 * azuepke, 2013-09-15: initial
 * azuepke, 2016-04-14: ported from ZYNQ
 * azuepke, 2016-04-18: adapted to MPU
 */

#include <kernel.h>
#include <assert.h>
#include <arm_insn.h>
#include <board.h>
#include <arm_private.h>
#include <arm_io.h>
#include <board_stuff.h>
#include <mpcore.h>
#include <mpcore_timer.h>
#include <linker.h>
#include <sched.h>	/* num_cpus */
#include <hm.h>


/* reset the board */
static inline void __board_reset(void)
{
	/* Reset via watchdog control register @ 0x020bc000 */
	volatile uint16_t *wdog_wcr = (volatile uint16_t *)0x020bc000;

	/* enable the watchdog twice, see
	 * https://community.freescale.com/thread/323034?db=5
	 */
	*wdog_wcr = 0x4;
	arm_dmb();
	*wdog_wcr = 0x4;
	arm_dmb();

	/* now wait for WFI */
}

__cold void board_halt(haltmode_t mode __unused)
{
#ifdef SMP
	mpcore_send_stop();
#endif

	if ((mode == BOARD_RESET) || (mode == BOARD_HM_RESET)) {
		__board_reset();
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
	/* empty -- uses boot page table */
}

static __init void board_init_core0(void)
{
#ifdef SMP
	unsigned int cpus;
#endif

	serial_init(115200);
	printf("Starting up ...\n");

	printf("assuming ROM from %08x to %08x\n", BOARD_ROM_PHYS, BOARD_ROM_PHYS + BOARD_ROM_SIZE);
	printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
	printf("assuming RAM from %08x to %08x\n", BOARD_RAM_PHYS, BOARD_RAM_PHYS + BOARD_RAM_SIZE);
	printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
	printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);

#ifdef SMP
	cpus = mpcore_init_smp();
	printf("SMP: found %d CPUs\n", cpus);
#endif

	mpcore_irq_init();
	mpcore_gic_enable();
	mpcore_timer_init(20);	/* HZ */
}


#ifdef SMP
/** number of CPUs in the system (values 0 and 1 denote an UP system) */
static volatile unsigned int board_cpus_online;

/** release the hounds */
void __init board_start_secondary_cpus(void)
{
	/* Sytem reset controller @ 0x020d8000, see page 5069 in i.MX6 manual */
	volatile uint32_t *src_scr = (volatile uint32_t *)0x020d8000;
	volatile uint32_t *src_gpr = (volatile uint32_t *)0x020d8020;

	unsigned int cpu;

	assert(board_cpus_online == 1);
	assert(arch_cpu_id() == 0);

	for (cpu = 1; cpu < num_cpus; cpu++) {
		/* set SMP entry point */
		src_gpr[cpu * 2] = (unsigned long)_start;
		/* bits 22..24: enable core1..3 */
		/* bits 13..16: reset core0..3 */
		*src_scr |= (1 << (21 + cpu)) | (1 << (13 + cpu));
		arm_dmb();

		/* wait until CPU is up */
		while (!(board_cpus_online & (1u << cpu))) {
			arm_wfe();
		}
	}
}

/** callback to signal successful booting of a secondary CPU */
void __init board_secondary_cpu_up(unsigned int cpu)
{
	assert(cpu == arch_cpu_id());
	board_cpus_online |= (1u << cpu);
	arm_dmb();
	arm_sev();
}
#endif

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
	/* do low-level init only on first CPU */
#ifdef SMP
	if (board_cpus_online == 0) {
		board_cpus_online = 1;
#endif
		board_init_core0();
#ifdef SMP
	}
	else {
		printf("secondary CPU %d came up\n", arch_cpu_id());
		mpcore_gic_enable();
	}
#endif

	/* enter the kernel */
	/* NOTE: all processors take the same entry point! */
	kernel_main(0);
}
