/**
* \file hsm_start.S
*
* \brief HSM startup code for the MPC5748G HSM.
*
* \author easycore GmbH, 91058 Erlangen, Germany
*
* \par  License
* Customer: @@LicenseCustomer@@,
* License type: @@LicenseType@@,
* Licensed for project: @@LicenseProject@@.

* Copyright 2015 easycore GmbH. All rights exclusively reserved for easycore GmbH,
* unless expressly agreed to otherwise.
*/
/*==================[inclusions]==================================================================*/
#include <ppc_asm.h>
#include <ppc_spr.h>
#include <ppc_tlb.h>

/*==================[macros]======================================================================*/
#define HSMRAMBASE  0xA0000000
#define HSMRAMSIZE  0x8000
#define HSMVECTSIZE 256

/*==================[type definitions]============================================================*/

/*==================[internal function declarations]==============================================*/

/*==================[external function declarations]==============================================*/

/*==================[external constants]==========================================================*/

/*==================[internal constants]==========================================================*/

/*==================[external data]===============================================================*/

/*==================[internal data]===============================================================*/

/*==================[internal function definitions]===============================================*/

/*==================[external function definitions]===============================================*/
	.global __start
	.global hsm_bootheader
	.global hsm_bootvector
	.extern hsm_board_init
	.extern __rom_data_start
	.extern __data_start
	.extern __data_end
	.extern __bss_start
	.extern __bss_end
	.extern _SDA2_BASE_
	.extern _SDA_BASE_
	.global __hsmentry

	.data
	.section .text.hsmboot, "a",  @progbits

	/* HSM Boot header for SSCM */
	/* 0x610000:	FFFF0000	Id part 1 */
	/* 0x610004:	FFFF0000	Id part 2 */
	/* 0x610008:	FFFFFFFF	reserved */
	/* 0x61000C:	00610020	start address: 0x610020 */
	/* 0x610010:	FFFFFFFF	reserved for "configuration bits" */
	/* 0x610014:	FFFFFFFF	reserved for "configuration bits" */
	/* ... */
	/* 0x610020:	erste Instruktion des Startup-Codes */
hsm_bootheader:
	/* 00h  id */
	.long 0xFFFF0000
	/* 04h  id */
	.long 0xFFFF0000
	/* 08h  resvd */
	.long 0
	/* 0Ch  start address */
hsm_bootvector:
	.long __hsmentry
	/* 10h  resvd */
	.long 0
	/* 14h  resvd */
	.long 0
	/* 18h  padding */
	.long 0
	/* 1Ch  padding */
	.long 0

	.section .text_vle
	.section .text.hsmstart, "a", @progbits
	/* entry point(s) from boot loader */
__hsmentry:
__start:
	/* disable all interrupts */
   e_li     r0, 0
   mtmsr    r0

#define HSM_RAM_PHYS 0xA0000000
#define HSM_RAM_SIZE 0x8000
_init_ram:
        /* initialize all registers with this dummy value */
        lwi        r3, 0xDEADBEEF
        e_bl       _init_regs

        lwi        r3, HSM_RAM_PHYS
        /* will use stmw to write 32 registers - 128 bytes - per iteration. no need
         * write the last 128 byte block, as this was initialized above for stack */
        lwi        r4, (HSM_RAM_SIZE / 128)
        se_mtctr   r4

1:
        e_stmw     r0, 0(r3)
        e_addi     r3, r3, 128
        e_bdnz     1b

_copy_vectors:
	/* initialize exception vectors in RAM (in case flash access yields exceptions) */
   lwi     r3, 0x78000000 /* "branch to self" opcode */
	lwi     r4, HSMRAMBASE
	lwi     r5, (HSMRAMBASE + HSMVECTSIZE)

1:
   e_stw    r3, 0(r4)
   e_addi   r4, r4, 4
   cmplw    r4, r5
	e_blt	   1b

_init_ivpr:
	/* now relocate vectors to RAM */
   lwi    r2, HSMRAMBASE
   mtspr    SPR_IVPR, r2

/*
 * copy .data
 */
_copy_data:
   lwi      r3, __rom_data_start
   lwi      r4, __data_start
   lwi      r5, __data_end

1:
	e_lwz    r0, 0(r3)
	e_addi   r3, r3, 4
	e_stw	   r0, 0(r4)
	e_addi   r4, r4, 4
	cmplw    r4, r5
	e_blt		1b

/*
 * clear .bss
 */
_clear_bss:
   se_li    r0, 0
   lwi      r4, __bss_start
   lwi      r5, __bss_end

1:
   e_stw    r0, 0(r4)
   e_addi   r4, r4, 4
   cmplw    r4, r5
	e_blt		1b


_common_start:
	se_li		r0, 0
	mtspr	SPR_PID0, r0
	isync

_init_stackpointer:
	/* set up the stack pointer */
   lwi      r1, (HSMRAMBASE+HSMRAMSIZE)-16

	/* invalidate link register: set an end marker that will result in an exception when used */
   lwi      r2, 0x12345678
	mtlr     r2
	/* terminate stack chain */
	e_stw    r2, 0(r1)

/*
_init_sdastuff:
   lwi      r2, _SDA2_BASE_
   lwi      r13, _SDA_BASE_
*/

   e_b      hsm_board_init
	/* does not return */
__endlessloop:
   e_b		__endlessloop

/**
 * Initialize all registers with the value in R3.
 */
_init_regs:
        mr         r0,  r3
        mr         r1,  r3
        mr         r2,  r3
        mr         r4,  r3
        mr         r5,  r3
        mr         r6,  r3
        mr         r7,  r3
        mr         r8,  r3
        mr         r9,  r3
        mr         r10, r3
        mr         r11, r3
        mr         r12, r3
        mr         r13, r3
        mr         r14, r3
        mr         r15, r3
        mr         r16, r3
        mr         r17, r3
        mr         r18, r3
        mr         r19, r3
        mr         r20, r3
        mr         r21, r3
        mr         r22, r3
        mr         r23, r3
        mr         r24, r3
        mr         r25, r3
        mr         r26, r3
        mr         r27, r3
        mr         r28, r3
        mr         r29, r3
        mr         r30, r3
        mr         r31, r3

        se_blr

