/*
 * start.S
 *
 * Assembler startup code for Tricore
 *
 * NOTE: this startup code assumes no bootloader!
 * Instead, we are loaded by Lauerbach / QEMU directly from an ELF file
 * into "flash" memory and can use core local CRAM and DRAM exclusively.
 *
 * azuepke, 2014-12-23: initial
 * azuepke, 2014-12-26: startup adapted for TSIM / AURIX
 */

#include <board_stuff.h>
#include <tc_regs.h>

        .global _start

        .text
        .section .text.start, "ax"
        /* start of flash at 0xa0000000 */
.Lorg:

        /* the boot ROM on AURIX expected an 8-word signature at the beginning */
_boot_header:
        .word 0x00000000        /* user code start addr */
        .word 0xb3590070        /* boot from internal flash with lock-step disabled */
        .word 0x00000000        /* memory range start */
        .word 0x00000000        /* memory range end */
        .word 0x00000000        /* CRC memory range inverted */
        .word 0x00000000        /* CRC memory range inverted */
        .word 0x791eb864        /* CRC header */
        .word 0x86e1479b        /* CRC header inverted */

_start:

        /* enable instruction and data caches */
        mov     %d15, 0
        mtcr    CSFR_PCON0, %d15
        mtcr    CSFR_DCON0, %d15
        /* allow caching of global memories (segments 5..9) */
        mov     %d15, 0x3e0
        mtcr    CSFR_PMA0, %d15
        mtcr    CSFR_PMA1, %d15
        dsync

        /* jump to absolute address in cached segment */
        ja      1f
1:

        /* NOTE: workaround for Lauterbach simulator bug: PSW must be written
         * at least once before it can be read.
         * We also enable write access for a0..1 and a8..9 during kernel init.
         */
        mov     %d0, PSW_IDLE_BITS | PSW_GW
        mtcr    CSFR_PSW, %d0
        isync

        j       .Lets_go1

        /*
         * Trap Vector Table
         *
         * Core Architecture, User Manual (Volume 1), chapter 6.1, Trap Types:
         * "The TriCore architecture specifies eight general classes for traps.
         * Each class has its own trap handler, accessed through a trap vector
         * of 32 bytes per entry, indexed by the hardware-defined trap class
         * number. Within each class, specific traps are distinguished by a Trap
         * Identification Number (TIN) that is loaded by hardware into register
         * D[15] before the first instruction of the trap handler is executed."
         *
         * The reset value of the BTV register is 0xA0000100.
         */

        /* Class 0 - MMU */
        .org .Lorg + 0x100
        j               trap_class_0
        /* Class 1 - Internal Protection Traps */
        .org .Lorg + 0x120
        j               trap_class_1
        /* Class 2 - Instruction Errors */
        .org .Lorg + 0x140
        j               trap_class_2
        /* Class 3 - Context Management */
        .org .Lorg + 0x160
        j               trap_class_3
        /* Class 4 - System Bus and Peripheral Errors */
        .org .Lorg + 0x180
        j               trap_class_4
        /* Class 5 - Assertion Traps */
        .org .Lorg + 0x1a0
        j               trap_class_5
        /* Class 6 - System Call */
        .org .Lorg + 0x1c0
        j               trap_class_6
        /* Class 7 - Non-Maskable Interrupt */
        .org .Lorg + 0x1e0
        j               trap_class_7

.Lbad:
        wait
        j               .Lbad

        /* normal operation continues here */
.Lets_go1:
        jl     clear_all_registers

#ifdef SMP
        mfcr    %d15, CSFR_CORE_ID
        and     %d15, %d15, 7
        jeq     %d15, 0, .boot_csa_core0
        jeq     %d15, 1, .boot_csa_core1
        /* FALL-THROUGH: core 2 */

.boot_csa_core2:
        movh.a  %a10, hi:BOOT_STACK_CPU2
        lea     %a10, [%a10] lo:BOOT_STACK_CPU2

        movh.a  %a4, hi:BOOT_CSA_CPU2
        lea     %a4, [%a4] lo:BOOT_CSA_CPU2
        mov     %d4, BOOT_CSA_ENTRIES
        jl      board_setup_boot_csa
        jl      board_init
        /* DOES NOT RETURN */

.boot_csa_core1:
        movh.a  %a10, hi:BOOT_STACK_CPU1
        lea     %a10, [%a10] lo:BOOT_STACK_CPU1

        movh.a  %a4, hi:BOOT_CSA_CPU1
        lea     %a4, [%a4] lo:BOOT_CSA_CPU1
        mov     %d4, BOOT_CSA_ENTRIES
        jl      board_setup_boot_csa
        jl      board_init
        /* DOES NOT RETURN */

.boot_csa_core0:
#endif

        /* set stack (a10) and initialize CSA (in C), this also sets
         * a11 (return register), PCXI (to zero), FCX, and LCX */
        movh.a  %a10, hi:BOOT_STACK_CPU0
        lea     %a10, [%a10] lo:BOOT_STACK_CPU0

        movh.a  %a4, hi:BOOT_CSA_CPU0
        lea     %a4, [%a4] lo:BOOT_CSA_CPU0
        mov     %d4, BOOT_CSA_ENTRIES
        jl      board_setup_boot_csa


        /* copy .data
         *
         * src = &__rom_data_start;
         * dst = &__data_start;
         * while (dst < &__data_end)
         *     *dst++ = *src++;
         */
        movh.a  %a2, hi:__rom_data_start
        lea     %a2, [%a2] lo:__rom_data_start
        movh.a  %a3, hi:__ram_data_start
        lea     %a3, [%a3] lo:__ram_data_start
        movh.a  %a15, hi:__data_end
        lea     %a15, [%a15] lo:__data_end

        /* align end to next 64-bit boundary */
        mov.d   %d15, %a15
        add     %d15, 7
        andn    %d15, %d15, 7

        /* loopcounter = ((end - start) >> 3) */
        mov.d   %d14, %a3
        sub     %d15, %d14
        sh      %d15, -3
        mov.a   %a15, %d15

        j       2f

        /* 64-bit copy loop */
1:      ld.d    %e0, [%a2+] 8
        st.d    [%a3+] 8, %e0
2:      loop    %a15, 1b

        /* zero .bss
         *
         * dst = &__bss_start;
         * while (dst < &__bss_end)
         *     *dst++ = 0;
         */
        movh.a  %a3, hi:__bss_start
        lea     %a3, [%a3] lo:__bss_start
        movh.a  %a15, hi:__bss_end
        lea     %a15, [%a15] lo:__bss_end

        /* align end to next 64-bit boundary */
        mov.d   %d15, %a15
        add     %d15, 7
        andn    %d15, %d15, 7

        /* loopcounter = ((end - start) >> 3) */
        mov.d   %d14, %a3
        sub     %d15, %d14
        sh      %d15, -3
        mov.a   %a15, %d15

        mov     %e0, 0
        j       4f

        /* 64-bit clear loop */
3:      st.d    [%a3+] 8, %e0
4:      loop    %a15, 3b


        /* continue in C */
        jl              board_init
        /* DOES NOT RETURN */




/*============[ Subroutines ]=================================================*/

/*------------[ Clear all registers ]-----------------------------------------*/
clear_all_registers:
        /* clear all data registers */
        mov     %d0,    0
        mov     %d1,    0
        mov     %d2,    0
        mov     %d3,    0
        mov     %d4,    0
        mov     %d5,    0
        mov     %d6,    0
        mov     %d7,    0
        mov     %d8,    0
        mov     %d9,    0
        mov     %d10,   0
        mov     %d11,   0
        mov     %d12,   0
        mov     %d13,   0
        mov     %d14,   0
        mov     %d15,   0

        /* clear all address registers,
         * a10 and a11 are stack and return address */
        mov.a   %a0,  %d15
        mov.a   %a1,  %d15
        mov.a   %a2,  %d15
        mov.a   %a3,  %d15
        mov.a   %a4,  %d15
        mov.a   %a5,  %d15
        mov.a   %a6,  %d15
        mov.a   %a7,  %d15
        mov.a   %a8,  %d15
        mov.a   %a9,  %d15
        mov.a   %a12, %d15
        mov.a   %a13, %d15
        mov.a   %a14, %d15
        mov.a   %a15, %d15
        /* Go back where we were called from
           a11 is the return address when instruction
           JL is used */
        ji      %a11


