/*
 * board.c
 *
 * Board initialization for AURIX tc27x.
 *
 * azuepke, 2014-10-25: initial
 * azuepke, 2014-12-23: minimal board layer
 */
/*=================[inclusions]===============================================*/

#include <kernel.h>
#include <assert.h>
#include <tc_io.h> /* writel, readl */
#include <board.h>
#include <board_stuff.h>
#include <linker.h>
#include <sched.h>  /* num_cpus */
#include <tc_irq.h>
#include <hm.h>

#include "wdt.h"
#include "scu.h"
#include "leds.h"
#include "serial.h"
#include "stm_timer.h"

/*=================[external function declarations]===========================*/

/* This is the start function in the uncached code segment.
 * It is declared in the linker file and is the start point of CPU1 and CPU2.
 */
extern void _start_uncached (void);


/*=================[internal function declarations]===========================*/

static inline uint32_t board_read  (uint32_t address);
static inline void     board_write (uint32_t address, uint32_t value);

/*==================[internal data]===========================================*/

#ifdef SMP
static volatile unsigned int board_cpus_online;
#endif

/*=================[public function implementation]===========================*/

/*-----------------[set up CSA]-----------------------------------------------*/

/* called from asm boot code to setup boot CSA with PCX, FCX, and LCX */
void __init board_setup_boot_csa(void *base, unsigned int num_contexts)
{
    struct arch_ctxt_frame *csa = base;
    unsigned long fcx;
    unsigned int i;

    /* IRQ CSA: entry #0 starts the free chain */
    for (i = 0; i < num_contexts - 1; i++)
    {
        csa[i].pcxi = PTR_TO_CX(&csa[i + 1]);
    }

    assert(i == num_contexts - 1);
    csa[i].pcxi = 0;

    fcx = PTR_TO_CX(&csa[0]);
    MTCR(CSFR_FCX, fcx);
    MTCR(CSFR_LCX, 0);  /* keep LCX invalid, we can't handle exceptions yet */
    MTCR(CSFR_PCXI, 0);
    ISYNC();
}

/*-----------------[board initialization]-------------------------------------*/

void __init board_init(void)
{
    unsigned int cpu_id;

    cpu_id = arch_cpu_id();
    assert(cpu_id < num_cpus);

    if (cpu_id == 0)
    {
#ifdef SMP
        board_cpus_online = 1;
#endif

        scu_init_pll();
        leds_init();
        tc_irq_setup_all();
        serial_init();
        stm_timer_init(1000);
#if !defined(SMP)
        stm_timer_init_core(cpu_id);
#endif

        printf("Starting up ...\n");

        printf("assuming ROM from %08x to %08x\n", BOARD_FLASH_BASE, BOARD_FLASH_BASE + BOARD_FLASH_SIZE);
        printf("     kernel .text %08x to %08x\n", (int)__text_start, (int)__text_end);
        printf("assuming RAM from %08x to %08x\n", BOARD_SRAM_BASE, BOARD_SRAM_BASE + BOARD_SRAM_SIZE);
        printf("     kernel .data %08x to %08x\n", (int)__data_start, (int)__data_end);
        printf("     kernel .bss  %08x to %08x\n", (int)__bss_start, (int)__bss_end);
    }

#ifdef SMP
    stm_timer_init_core(cpu_id);
#endif

    /* enter the kernel */
    /* NOTE: all processors take the same entry point! */
    kernel_main(cpu_id);
}

/*-----------------[initialize secondary CPUs]--------------------------------*/

#ifdef SMP

/**
 * Base addresses of the CSFR registers for each CPU of the tc27x.
 * We use this array to iterate over all secondary CPUs when starting them.
 */
static const uint32_t const csfr_bases[] =
{
    CPU0_CSFR_BASE,
    CPU1_CSFR_BASE,
    CPU2_CSFR_BASE
};

void __init board_start_secondary_cpus(void)
{
    unsigned int cpu;
    uint32_t dbgsr_halt;

    assert(board_cpus_online == 1);
    assert(arch_cpu_id() == 0);
    assert((sizeof csfr_bases / sizeof csfr_bases[0]) == num_cpus);

    /* Start CPU1 and CPU2. For more information see the document
     * "TC2xx Start-up and Initialization, AP32201",
     * chapter 3.6 Multi-Core Start-up (page 35)
     */

    for (cpu = 1; cpu < num_cpus; cpu++)
    {
        /* Read the DBGST register, it tells us if the CPU is already running. */
        dbgsr_halt = board_read(csfr_bases[cpu] + CSFR_DBGSR);

        /* Get the value of the HALT bitfield */
        dbgsr_halt = (dbgsr_halt & CPU_DBGSR_HALT_MASK) >> CPU_DBGSR_HALT_OFF;

        /* If the CPU is not already running, start it */
        if (dbgsr_halt != CPU_DBGST_HALT_RUN)
        {
            /* Write the address of the uncached start routine in the program
             * counter register of the CPU. */
            board_write(csfr_bases[cpu] + CSFR_PC, (uint32_t)&_start_uncached);

            /* The value 4 is a "Not Applicable" value (10b) of the HALT
             * bitfield. This line will nudge the CPU which will immediately
             * start to execute the start routine. */
            board_write(csfr_bases[cpu] + CSFR_DBGSR, 4);

            /* Wail until the CPU has entered the kernel code. */
            while (!(board_cpus_online & (1u << cpu)))
                ;
        }
    }
}

/** send IPI request to other processors */
void board_ipi_broadcast(unsigned long cpu_mask __unused)
{
    assert(num_cpus > 1);
     /* FIXME: tricore specific magic here: */
    assert(1);
}
#endif

/*-----------------[end of CPU0 initialization]-------------------------------*/

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

/*-----------------[end of CPU1 and CPU2 initialization]----------------------*/
#ifdef SMP
void __init board_secondary_cpu_up(unsigned int cpu)
{
    assert(cpu == arch_cpu_id());
    board_cpus_online |= (1u << cpu);

    /* serve core watchdogs ... */
    if (cpu == 1)
    {
        wdt_set_endinit(WDT_BASE + WDT_CPU1_OFFSET, 0);
        wdt_disable(WDT_BASE + WDT_CPU1_OFFSET);
        wdt_set_endinit(WDT_BASE + WDT_CPU1_OFFSET, 1);
    }
    else
    {
        assert(cpu == 2);
        wdt_set_endinit(WDT_BASE + WDT_CPU2_OFFSET, 0);
        wdt_disable(WDT_BASE + WDT_CPU2_OFFSET);
        wdt_set_endinit(WDT_BASE + WDT_CPU2_OFFSET, 1);
    }
}
#endif

/*-----------------[end of kernel startup]------------------------------------*/

void __init board_startup_complete(void)
{
#if defined(SMP)
    stm_timer_release_all();
#endif
}

/*-----------------[board halt]-----------------------------------------------*/

/* enter idle mode with all interrupts disabled */
__cold void __board_halt(void)
{
    while (1)
    {
        /* wait and stay in idle mode, with interrupts disabled */
        __asm__ volatile ("wait" : : : "memory");
    }
}

__cold void board_halt(haltmode_t mode __unused)
{
    /* just halt the machine */
    __board_halt();
}

/*-----------------[board idle]-----------------------------------------------*/

void board_idle(void)
{
    while (1)
    {
        /* wait and stay in idle mode */
        __asm__ volatile ("wait" : : : "memory");
    }
}

/*-----------------[handle NMI]-----------------------------------------------*/

/* Dispatch for Non-Maskable Interrupt. */
void board_nmi_dispatch(unsigned int vector __unused)
{
    hm_system_error(HM_ERROR_NMI, vector);
}

/*-----------------[handle HM exception]--------------------------------------*/

int board_hm_exception
(
    struct arch_reg_frame *regs __unused,
    int fatal __unused,
    unsigned int hm_error_id __unused,
    unsigned long vector __unused,
    unsigned long fault_addr __unused,
    unsigned long aux __unused
)
{
    return 0;   /* exception not handled */
}

void board_tp_switch(
    unsigned int prev_timepart __unused,
    unsigned int next_timepart __unused,
    unsigned int tpwindow_flags __unused)
{
}

/*=================[internal function implementation]=========================*/

static inline uint32_t board_read(uint32_t address)
{
    return readl((volatile void *) address);
}

static inline void board_write(uint32_t address, uint32_t value)
{
    writel((volatile void *)address, value);
}

/*=================[end of file]==============================================*/

