/*
 * intc.c
 *
 * PPC INTC interrupt controller
 *
 * +-------------------------------------------------+
 * |           IPI routing                           |
 * +-------------------------------------------------+
 * |source |destination |software interrupt register |
 * +-------------------------------------------------+
 * |CPU0   |CPU1        |INTC_SSCIR1                 |
 * |CPU0   |CPU2        |INTC_SSCIR2                 |
 * |CPU1   |CPU0        |INTC_SSCIR3                 |
 * |CPU1   |CPU2        |INTC_SSCIR4                 |
 * |CPU2   |CPU0        |INTC_SSCIR5                 |
 * |CPU2   |CPU1        |INTC_SSCIR6                 |
 * +-------------------------------------------------+
 *
 * tjordan, 2014-07-14: initial MPC5646C port
 */

#include <kernel.h>
#include <assert.h>
#include <ppc_insn.h>
#include <ppc_io.h>
#include <board.h>
#include <board_stuff.h>
#include <isr.h>
#include <hm.h>
#include "intc.h"
#include "stm.h"


/* we support 1024 interrupt vectors (number of INTC_PSR*)*/
#define NUM_IRQS 1024

/* register definitions for INTC */
#define IRQ_INTC_BASE           (0xFC040000u)
/* INTC Priority Select Register INTC_PSR0 - INTC_PSR1023.
 * All INTC_PSR registers are 16 bit wide, which is why we multiplicate
 * the interrupt number by 2. */
#define IRQ_INTC_PSR(x)         (*((volatile unsigned short *) (0xFC040060u + ((x) << 1))))


/* PRC_SELN value 1000b: Interrupt request sent to processor 0.
 * Offset of SELN is 12 bit. */
#define IRQ_PSR_PRC_0           (0x08u << 12u)


/* INTC has 16 priorities, but internally, we only use two oft them:
 * priority 0 means that the source is masked, priority 1 is unmasked.
 * consequently, we always set CPR to 0 to enable all unmasked interrupts
 */
#define IRQ_PSR_PRI_MASK        0u
#define IRQ_PSR_PRI_UNMASK      1u

static volatile unsigned int stop_request;

static inline void         intc_write_cpr   (unsigned int pri);
static inline unsigned int intc_read_intvec (void);
static inline void         intc_write_eoir  (void);

static void irq_INTC_dispatch(void)
{
    unsigned int irq = intc_read_intvec();
#if (defined SMP)
    unsigned int cpu_id = arch_cpu_id();
#endif

    switch (irq)
    {
#if (defined SMP)
        /* Software interrupts (IPI) */
        case INTC_SOURCE_SSCIR1:
            /* CPU0 -> CPU1 */
            IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR1) = IRQ_SSCIR_CLR;
            assert(cpu_id == CPU1);
            kernel_ipi_handle(cpu_id, CPU0);
            break;
        case INTC_SOURCE_SSCIR2:
            /* CPU0 -> CPU2 */
            IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR2) = IRQ_SSCIR_CLR;
            assert(cpu_id == CPU2);
            kernel_ipi_handle(cpu_id, CPU0);
            break;
        case INTC_SOURCE_SSCIR3:
            /* CPU1 -> CPU0 */
            IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR3) = IRQ_SSCIR_CLR;
            assert(cpu_id == CPU0);
            kernel_ipi_handle(cpu_id, CPU1);
            break;
        case INTC_SOURCE_SSCIR4:
            /* CPU1 -> CPU2 */
            IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR4) = IRQ_SSCIR_CLR;
            assert(cpu_id == CPU2);
            kernel_ipi_handle(cpu_id, CPU1);
            break;
        case INTC_SOURCE_SSCIR5:
            /* CPU2 -> CPU0 */
            IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR5) = IRQ_SSCIR_CLR;
            assert(cpu_id == CPU0);
            kernel_ipi_handle(cpu_id, CPU2);
            break;
        case INTC_SOURCE_SSCIR6:
            /* CPU2 -> CPU1 */
            IRQ_INTC_SSCIR(INTC_SOURCE_SSCIR6) = IRQ_SSCIR_CLR;
            assert(cpu_id == CPU1);
            kernel_ipi_handle(cpu_id, CPU2);
            break;
#endif
        /* STM */
        case INTC_SOURCE_STM_0_CIR0:
        case INTC_SOURCE_STM_1_CIR0:
        case INTC_SOURCE_STM_2_CIR0:
            stm_handler();
            break;
        default:
            isr_cfg[irq].func(isr_cfg[irq].arg0);
            break;
    }

    /* handler finished - either the interrupt was handled or it has been masked.
     * now we can lower CPR again */
    intc_write_eoir();
    intc_write_cpr(0);
}

/* default interrupt handlers */

__cold void board_unhandled_irq_handler(unsigned int irq)
{
    hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq);
}

/** initialize the INTC interrupt controller */
__init void board_intc_init(void)
{
#if (defined SMP)
    unsigned int cpu_id = arch_cpu_id();

    /* On each CPU enable the corresponding software interrupts we use for
     * simulating the IPI mechanism and disable all others so we do not get the
     * same interrupts on multiple CPUs. */
    switch (cpu_id)
    {
        case CPU0:
            board_irq_enable(INTC_SOURCE_SSCIR3);
            board_irq_enable(INTC_SOURCE_SSCIR5);

            board_irq_disable(INTC_SOURCE_SSCIR1);
            board_irq_disable(INTC_SOURCE_SSCIR2);
            board_irq_disable(INTC_SOURCE_SSCIR4);
            board_irq_disable(INTC_SOURCE_SSCIR6);

            break;
        case CPU1:
            board_irq_enable(INTC_SOURCE_SSCIR1);
            board_irq_enable(INTC_SOURCE_SSCIR6);

            board_irq_disable(INTC_SOURCE_SSCIR2);
            board_irq_disable(INTC_SOURCE_SSCIR3);
            board_irq_disable(INTC_SOURCE_SSCIR4);
            board_irq_disable(INTC_SOURCE_SSCIR5);

            break;
        case CPU2:
            board_irq_enable(INTC_SOURCE_SSCIR2);
            board_irq_enable(INTC_SOURCE_SSCIR4);

            board_irq_disable (INTC_SOURCE_SSCIR1);
            board_irq_disable (INTC_SOURCE_SSCIR3);
            board_irq_disable (INTC_SOURCE_SSCIR5);
            board_irq_disable (INTC_SOURCE_SSCIR6);

            break;
        default:
            /* unknown CPU id? */
            break;
    }
#endif

    printf("INTC supports %u IRQs\n", NUM_IRQS);
    intc_write_cpr(0);
}

/** mask IRQ in distributor */
void board_irq_disable(unsigned int irq)
{
    unsigned int   cpu_id   = 0;
    unsigned short intc_psr = 0;

    assert(irq < NUM_IRQS);
    cpu_id = arch_cpu_id();

    /* Read the corresponding INTC_PSR register */
    intc_psr = IRQ_INTC_PSR(irq);

    /* clear the corresponding cpu bit 
     * the rest of the INTC_PSR remains unchanged */
    intc_psr &= ~(IRQ_PSR_PRC_0 >> cpu_id);

    /* write back INTC_PSR */
    IRQ_INTC_PSR(irq) = intc_psr;
}

/** unmask IRQ in distributor */
void board_irq_enable(unsigned int irq)
{
    unsigned int   cpu_id  = 0;
    unsigned short prc_sel = 0;
    assert(irq < NUM_IRQS);

    cpu_id  = arch_cpu_id();

    prc_sel = IRQ_INTC_PSR(irq);
    prc_sel &= 0xE000u;
    prc_sel |= (IRQ_PSR_PRC_0 >> cpu_id);

    IRQ_INTC_PSR(irq) = prc_sel | IRQ_PSR_PRI_UNMASK;
}

/** dispatch IRQ: mask and ack, call handler */
void board_irq_dispatch(unsigned int vector)
{
    /* vector contains the IVOR number */
    if (vector == 4)
    {
        /* INTC interrupt */
        irq_INTC_dispatch();
    }
    else
    {
        hm_system_error(HM_ERROR_UNHANDLED_IRQ, 0x1000 + vector);
    }
}

/* "magic marker" for KLDD functions */
unsigned int board_irq_kldd_magic;

/** trigger a software interrupt
 * used as KLDD - may be used by tests or as a way of inter-partition communication
 * parameters: arg0 - "magic marker" for KLDD
 *             arg1 - logical interrupt number, 0 to 3
 * returns non-null if interrupt was triggered
 *
 * mpc5646c implementation: uses software interrupts 4 to 7
 */
unsigned int board_irq_trigger_kldd(void *arg0, unsigned long arg1,
                                    unsigned long arg2, unsigned long arg3)
{
    unsigned int num = arg1;
    assert(arg0 == &board_irq_kldd_magic);
    (void) arg0;
    (void) (arg2 | arg3);

    if (num < 4)
    {
        IRQ_INTC_SSCIR(num | 4) = IRQ_SSCIR_SET;
    }
    else
    {
        return 0;
    }

    return 1;
}

/** clear a software interrupt after it has been reported
 * used as KLDD - may be used by tests or as a way of inter-partition communication
 * parameters: arg0 - "magic marker" for KLDD
 *             arg1 - logical interrupt number, 0 to 3
 * returns non-null if interrupt has been cleared
 */
unsigned int board_irq_clear_kldd(void *arg0, unsigned long arg1,
                                  unsigned long arg2, unsigned long arg3)
{
    unsigned int num = arg1;
    assert(arg0 == &board_irq_kldd_magic);
    (void) arg0;
    (void) (arg2 | arg3);

    if (num < 4)
    {
        IRQ_INTC_SSCIR(num | 4) = IRQ_SSCIR_CLR;
    }
    else
    {
        return 0;
    }

    return 1;
}
/*==================[internal function definitions]===========================*/

/**
 * \brief   Write current priority register for current processor (INTC_CPRx).
 * \details This function writes the specified value into the INTC_CPRx register
 * of the current CPU. Depending on the CPU ID, the values is written to
 * INTC_CPR0, INTC_CPR1 or INTC_CPR2.
 *
 * \param[in] pri Value of the field INTC_CPRx.PRI.
 */
static inline void intc_write_cpr(unsigned int pri)
{
    unsigned int cpu_id = arch_cpu_id();

    MEMORY_WORD(IRQ_INTC_BASE + 0x10u + (cpu_id << 2)) = pri;
}

/*------------------[Read INTVEC field]---------------------------------------*/

/**
 * \brief   Read the field INTC_IACKRx.INTVEC of current processor.
 * \details Reading the source number also acknowledges the interrupt.
 */
static inline unsigned int intc_read_intvec(void)
{
    unsigned int cpu_id = arch_cpu_id();
    unsigned int iackr  = MEMORY_WORD(IRQ_INTC_BASE + 0x20u + (cpu_id << 2));
    return (iackr & 0x0FFCu) >> 2;
}

/*------------------[Write  End Of Interrupt]---------------------------------*/

/**
 * \brief   Write End Of Interrupt Register for current processor.
 * \details Writing to the INTC_EOIRn signals the end of the servicing of the
 * interrupt request.
 */
static inline void intc_write_eoir(void)
{
    unsigned int cpu_id = arch_cpu_id();

    /* End of Interrupt.
     * Write four all-zero bytes to this field to signal the end of the
     * servicing of an interrupt request. */
    MEMORY_WORD(IRQ_INTC_BASE + 0x30u + (cpu_id << 2)) = 0u;
}

/*==================[end of file]=============================================*/
