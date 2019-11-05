/*
 * board.c
 *
 * Board initialization for Tricore AURIX simulator.
 *
 * azuepke, 2014-10-25: initial
 * azuepke, 2014-12-23: minimal board layer
 */

#include <kernel.h>
#include <assert.h>
#include <board.h>
#include <board_stuff.h>
#include <linker.h>
#include <sched.h>	/* num_cpus */
#include <tc_irq.h>
#include <hv_error.h>
#include <hm.h>


/* enter idle mode with all interrupts disabled */
__cold void __board_halt(void)
{
#if 1
	/* FIXME: bail out of TSIM */
	__asm__ volatile ("ja 0xa00001f0" : : : "memory");
	/* DOES NOT RETURN */
#endif

	while (1) {
		/* wait and stay in idle mode, with interrupts disabled */
		__asm__ volatile ("wait" : : : "memory");
	}
}

__cold void board_halt(haltmode_t mode __unused)
{
	/* just halt the machine */
	__board_halt();
}

void board_idle(void)
{
	while (1) {
		/* wait and stay in idle mode */
		__asm__ volatile ("wait" : : : "memory");
	}
}

/* dummy serial implementation */
unsigned int board_putc(int c)
{
	/* unlike specified in the docs, __IOWRITE expects the char in d2 */
	__asm__ volatile (
		"mov %%d2, %0\n"
		"ja 0xa00001f8\n" : : "d"(c) : "memory");
	/* NOT REACHED -- CONTAINS RET */
	unreachable();
}

/* called from asm boot code to setup boot CSA with PCX, FCX, and LCX */
void __init board_setup_boot_csa(void *base, unsigned int num_contexts)
{
	struct arch_ctxt_frame *csa = base;
	unsigned long fcx;
	unsigned int i;

	/* IRQ CSA: entry #0 starts the free chain */
	for (i = 0; i < num_contexts - 1; i++) {
		csa[i].pcxi = PTR_TO_CX(&csa[i+1]);
	}
	assert(i == num_contexts - 1);
	csa[i].pcxi = 0;

	fcx = PTR_TO_CX(&csa[0]);
	MTCR(CSFR_FCX, fcx);
	MTCR(CSFR_LCX, 0);	/* keep LCX invalid, we can't handle exceptions yet */
	MTCR(CSFR_PCXI, 0);
	ISYNC();
}

void __init board_cpu0_up(void)
{
	/* serve and disable all watchdogs ... */
	wdt_set_endinit(WDT_BASE + WDT_S_OFFSET, 0);
	wdt_disable(WDT_BASE + WDT_S_OFFSET);
	wdt_set_endinit(WDT_BASE + WDT_S_OFFSET, 1);

	wdt_set_endinit(WDT_BASE + WDT_CPU0_OFFSET, 0);
	wdt_disable(WDT_BASE + WDT_CPU0_OFFSET);
	wdt_set_endinit(WDT_BASE + WDT_CPU0_OFFSET, 1);
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
	unsigned int cpu_id;

	cpu_id = arch_cpu_id();
	assert(cpu_id < num_cpus);

	if (cpu_id == 0) {
		printf("Starting up ...\n");

		printf("assuming ROM from %08x to %08x\n", BOARD_FLASH_BASE, BOARD_FLASH_BASE + BOARD_FLASH_SIZE);
		printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
		printf("assuming RAM from %08x to %08x\n", BOARD_SRAM_BASE, BOARD_SRAM_BASE + BOARD_SRAM_SIZE);
		printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
		printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);

		tc_irq_setup_all();
		stm_timer_init(100);
	}

	/* enter the kernel */
	/* NOTE: all processors take the same entry point! */
	kernel_main(cpu_id);
}
