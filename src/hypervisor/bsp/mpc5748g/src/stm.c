/**
 * \file      stm.c
 * \brief     Implementation of the STM driver for MPC5748G Calypso.
 * \details   For details see stm.h.
 *
 * \see       stm.h
 *
 * \date      27.08.2015
 * \author    Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author    easycore GmbH, 91058 Erlangen, Germany
 * \version
 * \par       License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 *
 * \copyright Copyright 2015 easycore GmbH, 91058 Erlangen, Germany.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include <kernel.h>      /* kernel_timer */
#include <board_stuff.h> /* PPC_TIMER_CLOCK */

#if defined(SMP)
#include <sched.h>  /* num_cpus */
#endif

#include "stm.h"
#include "intc.h"

#include "leds.h"

/*==================[macros]==================================================*/

/** Nanoseconds pro second */
#define NANOSECONDS (1000000000)

/*==================[type definitions]========================================*/

typedef struct
{
    /** Time in nanoseconds on last interrupt.
     *  Incremented periodically on each interrupt. */
    time_t time_last_tick_ns;
    
    /** Value of STM_CNT on last interrupt. */
    uint32_t last_stm_counter;

    /** Ticker time in nanoseconds. */
    uint32_t clock_ns;
    
    /** Next expiry point. */
    uint32_t next_expiry;

    /**
     * \brief   Reload value of the compare register.
     *
     * \details When the counter register reaches the value in the compare
     * register, it triggers an interrupt. The interrupt handler then increases
     * the compare register with this reload value.
     */
    uint32_t reload;
    
    /* Interrupt frequency of this timer as requested by user. */
    uint32_t int_frequency;
}
stm_timer;

/*==================[external function declarations]==========================*/
void stm_led_task(void);
/*==================[internal function declarations]==========================*/

static inline
void stm_write(uint32_t cpu_id, uint32_t reg, uint32_t value);

static inline
uint32_t stm_read(uint32_t cpu_id, uint32_t reg);

/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/

unsigned int board_timer_resolution;

/*==================[internal data]===========================================*/

static stm_timer* timer[3];

static stm_timer  timer_cpu0;
#if (defined SMP)
static stm_timer  timer_cpu1 __section_bss_core(1);
static stm_timer  timer_cpu2 __section_bss_core(2);
#endif

/*==================[external function definitions]===========================*/


/*------------------[Get board time]------------------------------------------*/

/* Get current time in nanoseconds. */
time_t board_get_time(void)
{
    uint32_t cpu_id        = arch_cpu_id();
    uint32_t current_timer = stm_read(cpu_id, STM_CNT);
    
    /* Being incremented at 160 MHz and being only 32 bits wide, the STM counter
     * needs about 26 seconds to overflow. So in order to avoid returning an
     * overflown number of nanoseconds, we do not directly convert the STM
     * counter, but first compute the absolute difference to the counter stored
     * at last interrupt, convert the difference to nanoseconds and add it to
     * the nanoseconds we recorded at last interrupt.
     */
    time_t   time_diff     = 0;

    /* Handle counter overflow, compute absolute difference */
    if (current_timer >= timer[cpu_id]->last_stm_counter)
    {
        time_diff = current_timer - timer[cpu_id]->last_stm_counter;
    }
    else
    {
        time_diff = timer[cpu_id]->last_stm_counter - current_timer;
    }    

    /* Convert to nanoseconds.
     * As the STM runs at 160 MHz, we have
     * 1 tick = 6.25 Nanoseconds.
     * t * 6.25 = t * (25/4) = t/4 * 25.
     */
    return timer[cpu_id]->time_last_tick_ns + ((time_diff / 4) * 25);
}

/*------------------[Initialize the STM timer]--------------------------------*/

void __init stm_init(unsigned int freq)
{
    unsigned int cpu_id = arch_cpu_id();

    if (cpu_id == CPU0)
    {
        timer[CPU0] = &timer_cpu0;
#if (defined SMP)
        timer[CPU1] = &timer_cpu1;
        timer[CPU2] = &timer_cpu2;
#endif
    }

    /* STM hangs on FS80 which routes only 80 MHz. So multiply the freq by two
     * in order to get the desired interrupt frequency. (Do so as we would run
     * at 160 MHz). */
    freq *= 2;

    timer[cpu_id]->reload           = PPC_TIMER_CLOCK / freq;
    timer[cpu_id]->clock_ns         = NANOSECONDS / freq;
    timer[cpu_id]->next_expiry      = timer[cpu_id]->reload;
    timer[cpu_id]->int_frequency    = freq / 2;
    timer[cpu_id]->last_stm_counter = 0;

    board_timer_resolution = timer[cpu_id]->clock_ns;

    /* Enable the STM interrupt in the interrupt controller.
     * Otherwise the timer will count without triggering an interrupt. */
    switch (cpu_id)
    {
        case CPU0:
            board_irq_enable  (INTC_SOURCE_STM_0_CIR0);
            board_irq_disable (INTC_SOURCE_STM_1_CIR0);
            board_irq_disable (INTC_SOURCE_STM_2_CIR0);
            break;
#if (defined SMP)
        case CPU1:
            board_irq_disable (INTC_SOURCE_STM_0_CIR0);
            board_irq_enable  (INTC_SOURCE_STM_1_CIR0);
            board_irq_disable (INTC_SOURCE_STM_2_CIR0);
            break;
        case CPU2:
            board_irq_disable (INTC_SOURCE_STM_0_CIR0);
            board_irq_disable (INTC_SOURCE_STM_1_CIR0);
            board_irq_enable  (INTC_SOURCE_STM_2_CIR0);
            break;
#endif
        default:
            /* Unknown CPU?? */
            while (1)
                ;
            break;
    }
    
    /* Load the reload value. At this point the timer should be turn off and the
     * count register have the value zero, otherwise the reload value will
     * probably point in the past (be less then the counter register) and the
     * timer would have to first overflow before getting a match */

    stm_write(cpu_id, STM_CMP, timer[cpu_id]->next_expiry);
}

/*------------------[Start the STM timer]-------------------------------------*/

void stm_release_all(void)
{
    uint32_t cpu = 0;
    uint32_t number_of_cpus = 1;
#if defined(SMP)
    number_of_cpus = num_cpus;
#endif

    assert(arch_cpu_id() == 0);

    /* start all STM timers */
    for (cpu = 0; cpu < number_of_cpus; cpu++)
    {
        /* Enable channel 0 */
        stm_write(cpu, STM_CCR, STM_CCR_ENABLE);

        /* enable timer, enable freeze on debug */

        /* "MPC5748G Reference Manual", "9.4.3.3 STM clocking", page 197:
         * "STM counter should be enabled only after selecting STM_2CLK_CR[CSL]." 
         * In our case CSL = 0 (system clock)
         * STM_CR.CPS = 0 meaning the system clock is devided by 1,
         * meaning the counter increments at 160 Mhz.
         */
        stm_write(cpu, STM_CR, (STM_CR_FRZ | STM_CR_TEN));
    }
}

/*------------------[Interrupt handler]---------------------------------------*/

/**
 * \note Only channel 0 is handled.
 */
void stm_handler(void)
{
    uint32_t cpu_id = arch_cpu_id();

    /* Clear channel interrupt request flag */
    stm_write(cpu_id, STM_CIR, STM_CIR_CLEAR);

    /* Set the compare register in the future
     * and increase it until it really points into the future. */
    do
    {
        timer[cpu_id]->next_expiry += timer[cpu_id]->reload;
    }
    while (timer[cpu_id]->next_expiry <= stm_read(cpu_id, STM_CNT));

    /* Write the new compare value */
    stm_write(cpu_id, STM_CMP, timer[cpu_id]->next_expiry);
    
    /* Update STM counter */
    timer[cpu_id]->last_stm_counter = stm_read(cpu_id, STM_CNT);

    /* update nanoseconds counter */
    timer[cpu_id]->time_last_tick_ns += timer[cpu_id]->clock_ns;

    /* notify kernel on timer interrupt */
    kernel_timer(timer[cpu_id]->time_last_tick_ns);

    //stm_led_task();
}

/*==================[internal function definitions]===========================*/
static unsigned int stm_counter0;
static unsigned int stm_counter1 __section_bss_core(1);
static unsigned int stm_counter2 __section_bss_core(2);

void stm_led_task(void)
{
    uint32_t cpu_id = arch_cpu_id();
    uint32_t half_second = timer[cpu_id]->int_frequency / 2;

    switch (cpu_id)
    {
        case CPU0:
            stm_counter0++;
            if (stm_counter0 >= half_second)
            {
                stm_counter0 = 0;
                led_toggle(CPU0);
            }
            break;
        case CPU1:
            stm_counter1++;
            if (stm_counter1 >= half_second)
            {
                stm_counter1 = 0;
                led_toggle(CPU1);
            }
            break;
        case CPU2:
            stm_counter2++;
            if (stm_counter2 >= half_second)
            {
                stm_counter2 = 0;
                led_toggle(CPU2);
            }
            break;
        default:
            break;
    }
}

static inline
void stm_write(uint32_t cpu_id, uint32_t reg, uint32_t value)
{
    MEMORY_WORD(STM_BASE + (cpu_id * STM_CORE_OFFSET) + reg) = value;
}

static inline
uint32_t stm_read(uint32_t cpu_id, uint32_t reg)
{
    return MEMORY_WORD(STM_BASE + (cpu_id * STM_CORE_OFFSET) + reg);
}

/*==================[end of file]=============================================*/
