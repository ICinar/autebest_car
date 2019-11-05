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
	ROM PT_LOAD FLAGS(5);
	RAM PT_LOAD FLAGS(6);
}

SECTIONS
{
	.text 0x00000000 : {
		__text_start = .;
		/* FIXME: RCHW *must* be put in one of the boot block locations, ideally at 0x0000000.
		 * FIXME: find out how to ensure this - should/can we hardcode this here? */
		*(.text.rchw) /* must be first */
		*(.text.start)
		*(.text.init)
		*(.text.cold)
		*(.text.unlikely)

		__text_hot_start = .;
		*(.text)
		__text_hot_end = .;
		*(.text.*)
	} : ROM

	.rodata : {
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

	.data 0x40000000 : AT (__rom_data_start) {
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

	.bss ALIGN(16) : {
		__bss_start = .;
		//*(.sbss)
		//*(.scommon)
		*(.bss)
		*(.bss.*)
		*(COMMON)
		__bss_end = .;
	} : NONE

	/DISCARD/ : {
		*(.eh_frame)
	}
}
