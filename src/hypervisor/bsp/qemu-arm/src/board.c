/*
 * board.c
 *
 * Board initialization for QEMU Versatile Express Cortex A15.
 *
 * azuepke, 2013-11-19: initial cloned
 */

#include <kernel.h>
#include <assert.h>
#include <arm_insn.h>
#include <board.h>
#include <arm_private.h>
#include <arm_io.h>
#include <board_stuff.h>
#include <mpcore.h>
#include <sp804_timer.h>
#include <linker.h>
#include <sched.h>	/* num_cpus */
#include <hm.h>


__cold void board_halt(haltmode_t mode __unused)
{
#ifdef SMP
	mpcore_send_stop();
#endif
	/* FIXME: no reset defined */

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

	serial_init(38400);
	printf("Starting up ...\n");

	printf("assuming ROM from %08x to %08x\n", BOARD_ROM_PHYS, BOARD_ROM_PHYS + BOARD_ROM_SIZE);
	printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
	printf("assuming RAM from %08x to %08x\n", BOARD_RAM_PHYS, BOARD_RAM_PHYS + BOARD_RAM_SIZE);
	printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
	printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);
#ifdef SMP
	printf("assuming RAM from %08x to %08x\n", BOARD_RAM2_PHYS, BOARD_RAM2_PHYS + BOARD_RAM2_SIZE);
	printf("     kernel .data %08x to %08x\n", (int)__data_core1_start, (int)__data_core1_end);
	printf("     kernel .bss  %08x to %08x\n", (int)__bss_core1_start, (int)__bss_core1_end);
#endif

#ifdef SMP
	cpus = mpcore_init_smp();
	printf("SMP: found %d CPUs\n", cpus);
#endif

	mpcore_irq_init();
	mpcore_gic_enable();
	sp804_timer_init(20);	/* HZ -- FIXME: can't exceed 20 HZ on QEMU */
}


#ifdef SMP
/** number of CPUs in the system (values 0 and 1 denote an UP system) */
static volatile unsigned int board_cpus_online;

/** release the hounds */
void __init board_start_secondary_cpus(void)
{
	unsigned int cpu;

	assert(board_cpus_online == 1);
	assert(arch_cpu_id() == 0);

	/* we flush the caches before doing anything, as the first instructions
	 * of a newly started processor runs in uncached memory mode.
	 */
	for (cpu = 1; cpu < num_cpus; cpu++) {
		__boot_release = cpu;
		arm_dmb();
		arm_clean_dcache(&__boot_release);
		arm_sev();

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

#if 1
// dummy KLDDs for demos
unsigned int bsp_kldd_led_on(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);
unsigned int bsp_kldd_led_on(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
	(void)arg0;
	(void)arg1;
	(void)arg2;
	(void)arg3;
	return 0;
}

unsigned int bsp_kldd_led_off(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);
unsigned int bsp_kldd_led_off(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
	(void)arg0;
	(void)arg1;
	(void)arg2;
	(void)arg3;
	return 0;
}

unsigned int bsp_kldd_led_toggle(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3);
unsigned int bsp_kldd_led_toggle(void *arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3)
{
	(void)arg0;
	(void)arg1;
	(void)arg2;
	(void)arg3;
	return 0;
}
#endif
