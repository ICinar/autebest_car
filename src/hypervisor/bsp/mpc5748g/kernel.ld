/*
 * kernel.ld
 *
 * Kernel linker script
 *
 * tjordan, 2014-07-15: initial, stolen from qemu-ppc
 */

ENTRY(_start)

PHDRS
{
    ROM  PT_LOAD FLAGS(5);
    RAM  PT_LOAD FLAGS(6);
    RAM2 PT_LOAD FLAGS(6);
    RAM3 PT_LOAD FLAGS(6);
}

SECTIONS
{
    /* 00FC_0000 is the second address where the BAF searches for the boot header.
     * The first address is skipped if HSM is enabled.
     * See MPC5748G Reference Manual
     * Table 57-3. Locations of boot headers, page 2838 */
    .text 0x00fc0000 :
    {
        __text_start = .;

        /* must be first */
        *(.text.rchw)
        *(.text.start)
        *(.text.init)
        *(.text.cold)
        *(.text.unlikely)

        __text_hot_start = .;
        *(.text)
        __text_hot_end = .;
        *(.text.*)
    } : ROM

    .rodata :
    {
        *(.rodata.cfg)
        *(.rodata)
        *(.rodata.*)
        *(.rodata1)

        __text_end = .;

        /* initialized data is placed right behind the text segment */
        . = ALIGN(16);
        __rom_data_start = .;
    } : ROM

    .data 0x40000000 : AT (__rom_data_start)
    {
        __ram_data_start = .;
        __data_start = .;
        *(.data)
        *(.data.*)
        *(.data1)
        . = ALIGN(16);
        __data_end = .;
        __rom_data_end = . - ABSOLUTE(__ram_data_start) + __rom_data_start;
    } : RAM

    .bss ALIGN(16) :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } : NONE

    /* .data and .bss on 2nd core */
    .data.core1 0x40040000 :
    {
        __data_core1_start = .;
        . = ALIGN(16);
        __data_core1_end = .;
    } : RAM2

    .bss.core1 ALIGN(16) :
    {
        __bss_core1_start = .;
        *(.core1.bss)
        *(.core1.bss.*)
        __bss_core1_end = .;
    } : NONE
    
    /* .data and .bss on 3rd core */
    .data.core2 0x40080000 :
    {
        __data_core2_start = .;
        . = ALIGN(16);
        __data_core2_end = .;
    } : RAM3
    
    .bss.core2 ALIGN(16) :
    {
        __bss_core2_start = .;
        *(.core2.bss)
        *(.core2.bss.*)
        __bss_core2_end = .;
    } : NONE
    
    /DISCARD/ :
    {
        *(.eh_frame)
    }
}
