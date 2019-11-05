/**
 * \file     start.S
 * \brief    Assembler startup code for MPC5748G.
 *
 * \author   easycore GmbH, 91058 Erlangen, Germany
 * \version
 * \par          License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * \par      File history
 * - lberaru, 2015-07-27: adapted to VLE syntax
 * - tjordan, 2014-07-15: initial version for MPC5646C
 * 
 * \copyright Copyright 2015 easycore GmbH.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include <board_stuff.h>
#include <ppc_asm.h>
#include <ppc_spr.h>

/*==================[macros]==================================================*/ 

/* interrupt inputs clear reservation */
#define HID0_ICR       0x00020000

#define SWT_0_CR       0xFC050000
#define SWT_0_CR_VAL   0x8000010A
#define SWT_0_SR       0xFC050010

#define SWT_1_CR       0xFC054000
#define SWT_1_CR_VAL   0x4000010A
#define SWT_1_SR       0xFC054010

#define SWT_2_CR       0xFC058000
#define SWT_2_CR_VAL   0x2000010A
#define SWT_2_SR       0xFC058010

#define SWT_SR_KEY     0xC520
#define SWT_SR_IKEY    0xD928


/*==================[global routine declarations]=============================*/

        .global bam_rchw
        .global _start
        .global __cpu0entry
        .global __cpu1entry
        .global __cpu2entry
        .global bam_resetvector
        .extern board_init
        .extern board_init_clocks
        .extern _ivor_0
        .extern _ivor_1
        .extern _ivor_2
        .extern _ivor_3
        .extern _ivor_4
        .extern _ivor_5
        .extern _ivor_6
        .extern _ivor_8
        .extern _ivor_15
        .extern _ivor_33
        .extern _ivor_34

        .data
        .section .text.rchw, "a", @progbits

        /* special boot word RCHW for BAM */
bam_rchw:
        /* 00h  boot header config */
        .long 0x005A0002
        /* 04h  CPU2 reset vector */
        .long 0
        /* 08h  config bits */
        .long 0
        /* 0Ch  config bits */
        .long 0
        /* 10h  CPU0 reset vector */

bam_resetvector:
        .long __cpu0entry
        /* 14h  CPU1 reset vector */
        .long 0
        /* 18h  reserved */
        .long 0
        /* 1Ch  padding */
        .long 0

#ifdef SECURE_BOOT
.extern secure_boot_table

.long secure_boot_table   /* points to the signed Secure Boot table */
#endif

        /* entry point(s) from boot loader */
__cpu1entry:
        e_b _cpu1_start
__cpu2entry:
        e_b _cpu2_start

__cpu0entry:
_start:
        /* disable all interrupts */
        e_li       r0, 0
        mtmsr      r0

_shut_down_watchdog:        
        /* arguments for _configure_watchdog */
        lwi        r0, SWT_0_SR
        lwi        r1, SWT_0_CR
        lwi        r2, SWT_0_CR_VAL
        /* disable watchdog */
        e_bl       _configure_watchdog
        

/* ECC RAM needs initialisation before it can be used */
_init_ram:
        /* initialize all registers with this dummy value */
        lwi        r3, 0xDEADBEEF
        e_bl       _init_regs

        lwi        r3, BOARD_RAM_PHYS
        /* will use stmw to write 32 registers - 128 bytes - per iteration. no need
         * write the last 128 byte block, as this was initialized above for stack */
        lwi        r4, (BOARD_RAM_SIZE / 128)
        se_mtctr   r4

1:
        e_stmw     r0, 0(r3)
        e_addi     r3, r3, 128
        e_bdnz     1b

        /*
         * copy .data
         */
_copy_data:
        lwi        r3, __rom_data_start
        lwi        r4, __data_start
        lwi        r5, __data_end

        /* jump over if nothing to copy */
        cmplw      r4, r5
        e_beq      _copy_data_end 

1:      se_lwz     r0, 0(r3)  // load from source
        e_addi     r3, r3, 4  // increment source index
        se_stw     r0, 0(r4)  // store to destination
        e_addi     r4, r4, 4  // increment destination index
        cmplw      r4, r5     // reached end of destination?
        e_blt      1b         // if no, repeat
_copy_data_end:

        /*
         * clear .bss
         */
_clear_bss:
        se_li      r0, 0
        lwi        r5, __bss_core2_end // clear until end of bss of cpu2

1:      se_stw     r0, 0(r4)
        e_addi     r4, r4, 4
        cmplw      r4, r5
        e_blt      1b


_common_start:
        se_li      r0, 0
        mtspr      SPR_PID0, r0
        se_isync

_set_prekernel_exception_table:
        /* Set preliminary exception table.
         * The kernel will switch to final table
         * during initialization (ppc_set_ivors).
         */
        lwi        r2, _prekernel_interrupt_vector_base
        mtspr      SPR_IVPR, r2

        /* init FPU */
        // mtspr        SPR_SPEFSCR, r0

_enable_branch_prediction:
        /* flush internal branch cache
         * and enable branch prediction */
        lwi        r3, 0x200     /* flush */
        mtspr      SPR_BUCSR, r3
        sync
        se_li      r3, 1         /* enable */
        mtspr      SPR_BUCSR, r3
        sync

_init_hid0:
        lwi        r3, HID0_ICR
        mtspr      SPR_HID0, r3
        se_isync

_init_registers_for_C:
        se_li      r3, 0x00
        e_bl       _init_regs

_init_stack_for_C:
        /* set up the stack pointer */
        lwi        r1, BOOT_STACK-16

        /* invalidate link register:
         * set an end marker that will result in an exception when used */
        lwi        r2, 0
        /* Move To Link Register */
        se_mtlr    r2
        /* terminate stack chain */
        se_stw     r2, 0(r1)
        /* also clear r2 */
        se_li      r2, 0

_enter_C_code:
        e_bl       board_init
        /* does not return */

_unreachable_after_C_code:
        e_b        _unreachable_after_C_code

/*==================[Utility Routines]========================================*/

/*------------------[Startup code CPU1]---------------------------------------*/

_cpu1_start:

_cpu1_disable_interrupts:
        e_li       r0, 0
        mtmsr      r0

_cpu1_shut_down_watchdog:
        /* arguments for disable_swt */
        lwi        r0, SWT_1_SR
        lwi        r1, SWT_1_CR
        lwi        r2, SWT_1_CR_VAL
        /* disable watchdog */
        e_bl       _configure_watchdog

_cpu1_continue_with_common_code:
        /* should not return */
        e_bl       _common_start

/* catch cpu if something goes very wrong */
_cpu1_start_end:
        e_b       _cpu1_start_end

/*------------------[Startup code CPU2]---------------------------------------*/
_cpu2_start:

_cpu2_disable_interrupts:
        e_li       r0, 0
        mtmsr      r0

_cpu2_shut_down_watchdog:
        /* arguments for disable_swt */
        lwi        r0, SWT_2_SR
        lwi        r1, SWT_2_CR
        lwi        r2, SWT_2_CR_VAL
        /* disable watchdog */
        e_bl       _configure_watchdog

_cpu2_continue_with_common_code:
        /* should not return */
        e_bl       _common_start

/* catch cpu if something goes very wrong */
_cpu2_start_end:
        e_b       _cpu2_start_end

/*------------------[Initialize Registers]------------------------------------*/

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

/*------------------[Disable Watchdog]----------------------------------------*/

/**
 * \brief Disable watchdog.
 *
 * \param[in] r0 Address of the SWT Service Register (SWTx_SR)
 * \param[in] r1 Address of the SWT Control Register (SWTx_CR)
 * \param[in] r2 Value of the SWTx_CR register
e*/
_configure_watchdog:
        lwi        r3, SWT_SR_KEY    // key for SWT_SR
        lwi        r4, SWT_SR_IKEY   // inverted key for SWT_SR

        se_mtar    r11, r0           // address of SWT_SR

        e_stw      r3,0(r11)         // write key to SWT_SR
        sync                         // wait for memory write to complete

        e_stw      r4,0(r11)         // write inverted key SWT_SR
        sync

        se_mtar    r11, r1           // r11 = address of SWT_CR
        e_stw      r2, 0(r11)        // disable SWT
        sync

        se_blr

/*==================[Jump Tables]=============================================*/

/*------------------[Prekernel hardware exception table]----------------------*/
/* This exception table is used to catch any exception before the kernel
 * switches to the final exception table (routine ppc_set_ivors).
 * To use the table, put its address into the register IVPR.
 * The reason we use a pre kernel table is to avoid jumping to kernel routines
 * before the kernel has finished initialization.
 */

.balign 256
_prekernel_interrupt_vector_base:

/* Offset 0x00 */
_preKernel_Critical_Input_Interrupt:
    e_b _preKernel_Critical_Input_Interrupt

/* Offset 0x10 */
.balign 16
_preKernel_Machine_Check_interrupt:
    e_b _preKernel_Machine_Check_interrupt

/* Offset 0x20 */
.balign 16
_preKernel_Data_Storage_interrupt:
    e_b _preKernel_Data_Storage_interrupt

/* Offset 0x30 */
.balign 16
_preKernel_Instruction_Storage_interrupt:
    e_b _preKernel_Instruction_Storage_interrupt

/* Offset 0x40 */
.balign 16
_preKernel_External_Input_interrupt:
    e_b _preKernel_External_Input_interrupt

/* Offset 0x50 */
.balign 16
_preKernel_Alignment_interrupt:
    e_b _preKernel_Alignment_interrupt

/* Offset 0x60 */
.balign 16
_preKernel_Program_interrupt:
    e_b _preKernel_Program_interrupt

/* Offset 0x70 */
.balign 16
_preKernel_Performance_Monitor_interrupt:
    e_b _preKernel_Performance_Monitor_interrupt

/* Offset 0x80 */
.balign 16
_preKernel_System_Call_interrupt:
    e_b _preKernel_System_Call_interrupt

/* Offset 0x90 */
.balign 16
_preKernel_Debug_interrupt:
    e_b _preKernel_Debug_interrupt

/* Offset 0xA0 */
.balign 16
_preKernel_EFPU_Data_interrupt:
    e_b _preKernel_EFPU_Data_interrupt

/* Offset 0xB0 */
.balign 16
_preKernel_EFPU_Round_interrupt:
    e_b _preKernel_EFPU_Round_interrupt

/*==================[end of file]=============================================*/

