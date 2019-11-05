/*
 * board.c
 *
 * Board initialization for QEMU Versatile Express Cortex A15.
 *
 * azuepke, 2013-11-22: initial PPC port
 * azuepke, 2014-06-03: adapted to MPU
 */

#include <kernel.h>
#include <assert.h>
#include <ppc_tlb.h>
#include <board.h>
#include <ppc_private.h>
#include <ppc_io.h>
#include <board_stuff.h>
#include <linker.h>
#include <sched.h>	/* num_cpus */
#include <hm.h>


/* enter idle mode with all interrupts disabled */
__cold void __board_halt(void)
{
	unsigned long msr;

	/* disable all interrupts */
	msr = ppc_get_msr();
	msr &= ~(MSR_EE | MSR_CE | MSR_ME | MSR_DE);
	msr |= MSR_WE;

	ppc_sync();
	ppc_set_msr(msr);
	ppc_isync();
	while (1) {
		/* wait and stay in idle mode, with interrupts disabled */
	}
}

/* reset the board */
static inline void __board_reset(void)
{
	/* Reset Control Register RSTCR, see chapter 23.4.1.17 in MPC8572ERM.pdf */
	writel((uint32_t*)(BOARD_CCSRBAR_VIRT + 0xe00b0), 0x2);

	__board_halt();
}

__cold void board_halt(haltmode_t mode)
{
#ifdef SMP
	mpic_send_stop();
#endif
	if ((mode == BOARD_RESET) || (mode == BOARD_HM_RESET)) {
		__board_reset();
	}

	/* just halt the machine */
	__board_halt();
}

void board_idle(void)
{
	unsigned long msr;

	/* only effective if HID0[DOZE] is enabled! */
	msr = ppc_get_msr();
	msr |= MSR_WE;

	/* see e500corerm 6.4.1 "Software Considerations for Power Management" */
	ppc_sync();
	ppc_set_msr(msr);
	ppc_isync();
	while (1) {
		/* wait and stay in idle mode */
	}
}

void __init board_mpu_init(void)
{
	/* PowerPC uses TLB entries set at boot */
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
	/* setup mapping to CCSRBAR */
	ppc_set_tlb(MAS0_ESEL(BOARD_PRIVATE_TLBS + 2) | MAS0_TLBSEL(1),
	            MAS1_V | MAS1_TID(0) | MAS1_TSIZE(TLB_SIZE_1M),
	            BOARD_CCSRBAR_VIRT | MAS2_I | MAS2_M | MAS2_G,
	            BOARD_CCSRBAR_PHYS | MAS3_SR | MAS3_SW);

	serial_init(115200);
	printf("Starting up ...\n");

	printf("assuming ROM from %08x to %08x\n", BOARD_ROM_PHYS, BOARD_ROM_PHYS + BOARD_ROM_SIZE);
	printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
	printf("assuming RAM from %08x to %08x\n", BOARD_RAM_PHYS, BOARD_RAM_PHYS + BOARD_RAM_SIZE);
	printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
	printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);

	mpic_irq_init();
	mpic_enable();
	ppc_timer_init(20);	/* HZ -- FIXME: can't exceed 20 HZ on QEMU */

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
