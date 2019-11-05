/*
 * kernel.ld
 *
 * Kernel linker script
 *
 * azuepke, 2013-03-22: initial
 * azuepke, 2013-11-25: initial MPU version
 * azuepke, 2013-12-08: split ROM / RAM sections
 * azuepke, 2014-04-25: config generator relocates the kernel
 */

ENTRY(_start_uncached)

PHDRS
{
    ROM  PT_LOAD FLAGS(5);
    RAM  PT_LOAD FLAGS(6);
    RAM2 PT_LOAD FLAGS(6);
    RAM3 PT_LOAD FLAGS(6);
}

SECTIONS
{
    .text 0x80000000 :
    {
        __text_start = .;
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
        //*(.sdata2)
        //*(.sbss2)

        __text_end = .;

        /* initialized data is placed right behind the text segment */
        . = ALIGN(16);
        __rom_data_start = .;
    } : ROM

    .data 0x70000000 : AT (__rom_data_start)
    {
        __ram_data_start = .;
        __data_start = .;
        *(.data)
        *(.data.*)
        *(.data1)
        //*(.sdata)
        . = ALIGN(16);
        __data_end = .;
        __rom_data_end = . - ABSOLUTE(__ram_data_start) + __rom_data_start;
    } : RAM

    /* NOTE: register contexts need 64-byte alignment! */
    .bss ALIGN(64) :
    {
        __bss_start = .;
        //*(.sbss)
        //*(.scommon)
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } : NONE

    /* .data and .bss on 2nd core */
    .data.core1 0x60000000 :
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
    .data.core2 0x50000000 :
    {
        __data_core2_start = .;
        . = ALIGN(16);
        __data_core2_end = .;
    } : RAM2

    .bss.core2 ALIGN(16) :
    {
        __bss_core2_start = .;
        *(.core2.bss)
        *(.core2.bss.*)
        __bss_core2_end = .;
    } : NONE

    .uncached_flash 0xa0000020 :
    {
        _start_uncached = .;
    } : NONE

    /DISCARD/ :
    {
        *(.eh_frame)
    }
}
