/**
 * \file   stm_timer.h
 * \brief  Interface of the stm timer module.
 *
 * \author easycore GmbH, 91058 Erlangen, Germany
 * \date   02.03.2015
 */

#ifndef STM_TIMER_H
#define STM_TIMER_H
/*==================[definitions and macros]==================================*/

/** Base address of the STM0 timer. */
#define STM_TIMER_BASE          0xf0000000

#define STM_TIMER_COREOFFSET    0x100
#define STM_TIMER_IRQ           2   /* see IRQ table in tc_irq_conf_tsim.c! */

/** Address of the CLC register. */
#define STM0_CLC                (*(volatile unsigned int*)0xF0000000u)

/** Offset of the DISR bit field in the CLC register. */
#define STM0_CLC_DISR_OFF       (0)

/* Timer Register 0 */
#define STM0_TIM0               (*(volatile unsigned int*)0xF0000010u)
/* Timer Capture Register */
#define STM0_CAP                (*(volatile unsigned int*)0xF000002Cu)

#define NANOSECONDS             (1000000000u)

/*==================[external function declarations]==========================*/

void stm_timer_handler(unsigned int irq);
void stm_timer_init_core(unsigned int core_id);
void stm_timer_init(unsigned int freq);

#ifdef SMP
void stm_timer_release_all(void);
#endif

#endif /* STM_TIMER_H */
/*==================[end of file]=============================================*/

