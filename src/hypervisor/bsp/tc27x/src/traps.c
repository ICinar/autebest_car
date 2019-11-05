/**
 * \file traps.c
 *
 * \details This file contains the trap handlers which the startup code
 * delegates to.
 *
 * For trap description, see User Manual (Volume 1), Table 6-1 Supported Traps,
 * page 93.
 *
 * \date        03.06.2015
 * \author      Liviu Beraru <Liviu.Beraru@easycore.com>
 * \copyright   easycore GmbH, 91058 Erlangen, Germany
 */
/*==================[inclusions]==============================================*/

#include <tc_regs.h> /* MFCR */

/*==================[macros]==================================================*/

#define READ_D15() ({\
        register unsigned long d15 __asm__ ("d15");\
        d15;\
        })

#define READ_A10() ({\
        unsigned long a11;\
        __asm__ volatile ("mov.d %0, %%a10" : "=d"(a11) : : "memory");\
        a11;\
        })

#define READ_A11() ({\
        unsigned long a11;\
        __asm__ volatile ("mov.d %0, %%a11" : "=d"(a11) : : "memory");\
        a11;\
        })

#define FREEZE(x) \
    if (NumberOfTraps < NUMBER_OF_CPUS) \
    {\
        trap_state[NumberOfTraps].Trap_Class = x;\
        trap_state[NumberOfTraps].CPU    = (MFCR(CSFR_CORE_ID) & 0x7);\
        trap_state[NumberOfTraps].TIN    = READ_D15();\
        trap_state[NumberOfTraps].A10    = READ_A10();\
        trap_state[NumberOfTraps].A11    = READ_A11();\
        trap_state[NumberOfTraps].PSW    = MFCR(CSFR_PSW);\
        trap_state[NumberOfTraps].SYSCON = MFCR(CSFR_SYSCON);\
        trap_state[NumberOfTraps].PCXI   = MFCR(CSFR_PCXI);\
        trap_state[NumberOfTraps].DATR   = MFCR(CSFR_DATR);\
        trap_state[NumberOfTraps].DEADD  = MFCR(CSFR_DEADD);\
        NumberOfTraps++;\
    }

#define WAIT_FOREVER() while (1) __asm__ volatile ("wait" : : : "memory")

/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/

/* Implemented in this file */
void trap_class_0(void);
void trap_class_1(void);
void trap_class_2(void);
void trap_class_3(void);
void trap_class_4(void);
void trap_class_5(void);
void trap_class_6(void);
void trap_class_7(void);

/*==================[internal function declarations]==========================*/


/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

/* We can track 3 traps because there are 3 CPUs. */
#define NUMBER_OF_CPUS 3

/* How many traps did we tracked so far. */
static unsigned int NumberOfTraps = 0;

/* This is what you should inspect in your debugger. */
static struct
{
    /* Which CPU got trapped. */
    unsigned int CPU;

    /* Trap class number. */
    unsigned int Trap_Class;
    /* Trap Identification Number */
    unsigned int TIN;

    /* Stack Pointer (SP) register. */
    unsigned int A10;

    /* Return Address (RA) register. */
    unsigned int A11;

    /* Program Status Word */
    unsigned int PSW;

    /* System Configuration Register */
    unsigned int SYSCON;

    /* Trap class specific register contents. */

    /* class 3, TIN 1 */
    unsigned int PCXI;

    /* class 4, TIN 3 */
    unsigned int DATR;
    unsigned int DEADD;
} trap_state[NUMBER_OF_CPUS];

/*==================[external function definitions]===========================*/

/*------------------[Class 0 - MMU]-------------------------------------------*/

void trap_class_0(void)
{
    FREEZE(0);
    WAIT_FOREVER();
}

/*------------------[Class 1 - Internal Protection Traps]---------------------*/

void trap_class_1(void)
{
    FREEZE(1);
    WAIT_FOREVER();
}

/*------------------[Class 2 - Instruction Errors]----------------------------*/

void trap_class_2(void)
{
    FREEZE(2);
    WAIT_FOREVER();
}

/*------------------[Class 3 - Context Management]----------------------------*/

void trap_class_3(void)
{
    FREEZE(3);
    WAIT_FOREVER();
}

/*------------------[Class 4 - System Bus and Peripheral Errors]--------------*/

void trap_class_4(void)
{
    FREEZE(4);
    WAIT_FOREVER();
}

/*------------------[Class 5 - Assertion Traps]-------------------------------*/

void trap_class_5(void)
{
    FREEZE(5);
    WAIT_FOREVER();
}

/*------------------[Class 6 - System Call]-----------------------------------*/

void trap_class_6(void)
{
    FREEZE(6);
    WAIT_FOREVER();
}

/*------------------[Class 7 - Non-Maskable Interrupt]------------------------*/


void trap_class_7(void)
{
    FREEZE(7);
    WAIT_FOREVER();
}

/*==================[internal function definitions]===========================*/
/*==================[end of file]=============================================*/

