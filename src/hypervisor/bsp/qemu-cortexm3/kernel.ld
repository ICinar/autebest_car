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

ENTRY(_start)

PHDRS
{
	ROM PT_LOAD FLAGS(5);
	RAM PT_LOAD FLAGS(6);
	RAM2 PT_LOAD FLAGS(6);
}

SECTIONS
{
	.text 0x00000000 : {
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

	.data 0x20000000 : AT (__rom_data_start) {
		__ram_data_start = .;
		__data_start = .;
		*(.data)
		*(.data.*)
		*(.data1)
		*(.core0.data)
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

		/* direct access to kernel and NMI stack */
		*(.core0.bss.kern_stack)
		__kern_stack = .;
		*(.core0.bss.nmi_stack)
		__nmi_stack = .;
		/* ditto for sched_state */
		__sched_state = .;
		*(.core0.bss.sched_state)

		*(.core0.bss)
		*(.core0.bss.*)
		__bss_end = .;
	} : NONE

	/DISCARD/ : {
		*(.eh_frame)
	}
}
