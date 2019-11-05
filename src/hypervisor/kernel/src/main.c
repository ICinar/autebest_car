/*
 * main.c
 *
 * Kernel main routine
 *
 * azuepke, 2013-03-22: initial
 * azuepke, 2013-11-24: rework for MPU kernel
 */

#include <kernel.h>
#include <assert.h>
#include <arch.h>
#include <board.h>
#include <sched.h>
#include <core.h>
#include <task.h>
#include <part.h>
#include <counter.h>
#include <alarm.h>
#include <schedtab.h>
#include <wq.h>
#include <mpu.h>
#include <arch_mpu.h>


/* forward declaration */
static void further_init(void *stack) __tc_fastcall;

static int boot_hm_restart;

/** kernel entry function */
__init void kernel_main(int hm_restart)
{
	if (arch_cpu_id() == 0) {
		/* initialize kernel subsystems, part #1 */
		sched_init();
		boot_hm_restart = hm_restart;
		part_init_idle();
		task_init_idle();
	}

	arch_set_sched_state(core_cfg[arch_cpu_id()].sched);
	arch_init_exceptions();
	board_mpu_init();

	arch_switch_to_kernel_stack(further_init);
	/* DOES NOT RETURN */
	unreachable();
}

/** further kernel initialization -- executed on the new kernel stack, not on the boot stack */
__init static void further_init(void *stack __unused)
{
	VVprintf("* cpu %d is now on the kernel stack %p ...\n", arch_cpu_id(), stack);

	/* initialize scheduling on this CPU */
	sched_start();

#ifdef SMP
	if (arch_cpu_id() == 0) {
#endif
		/* we're now running on a kernel stack, ready to rock the world */
		board_cpu0_up();

		printf("build: %s " ARCH_BANNER_NAME, kernel_buildid);
#if __WORDSIZE == 64
		printf(" 64-bit");
#else
		printf(" 32-bit");
#endif
#ifndef NDEBUG
		printf(" DEBUG");
#else
		printf(" RELEASE");
#endif
#ifdef SMP
		printf(" SMP");
#else
		printf(" UP");
#endif
		printf("\n");

		assert(board_timer_resolution > 0);
		Vprintf("* board timer resolution: %u\n", board_timer_resolution);

#ifndef NDEBUG
		/* print resource consumption */
		Vprintf("* system configuration / memory consumption    ROM   RAM (in bytes)\n");
		Vprintf("  scheduler state   struct sched_state              %4zd\n", sizeof(struct sched_state));
		Vprintf("  partition         struct part[_cfg]         %4zd  %4zd\n", sizeof(struct part_cfg), sizeof(struct part));
		Vprintf("  task              struct task[_cfg]         %4zd  %4zd\n", sizeof(struct task_cfg), sizeof(struct task));
		Vprintf("  alarm             struct alarm[_cfg]        %4zd  %4zd\n", sizeof(struct alarm_cfg), sizeof(struct alarm));
		Vprintf("  counter           struct counter[_cfg]      %4zd  %4zd\n", sizeof(struct counter_cfg), sizeof(struct counter));
		Vprintf("  schedule table    struct schedtab[_cfg]     %4zd  %4zd\n", sizeof(struct schedtab_cfg), sizeof(struct schedtab));
		Vprintf("  wait queue        struct wq[_cfg]           %4zd  %4zd\n", sizeof(struct wq_cfg), sizeof(struct wq));
		Vprintf("  CPU registers     struct arch_reg_frame           %4zd\n", sizeof(struct arch_reg_frame));
		Vprintf("  FPU registers     struct arch_fpu_frame           %4zd\n", sizeof(struct arch_fpu_frame));
		Vprintf("  Register context  struct arch_ctxt_frame          %4zd\n", sizeof(struct arch_ctxt_frame));
#endif

		/* initialize kernel subsystems, part #2 */
		part_init_rest(boot_hm_restart ?
		               PART_START_CONDITION_HM_MODULE_RESTART :
		               PART_START_CONDITION_NORMAL_START);
		task_init_rest();
		alarm_init_all();
		schedtab_init_all();
		wq_init_all();

		/* boot other CPUs */
#ifdef SMP
		board_start_secondary_cpus();
#endif

		board_startup_complete();

#ifdef SMP
	} else {
		board_secondary_cpu_up(arch_cpu_id());
	}
#endif

	/* initialize kernel subsystems, part #3: per core specific subsystems */
	counter_init_all_per_cpu();

	/* FINALLY: */
	/* start all partitions of that core */
	part_start_all(arch_cpu_id());

	/* on return, we leave to user space (or the idle task) */
}
