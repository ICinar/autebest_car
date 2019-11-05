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
	.text 0x10008000 : {
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
		/* page tables come first, with 16K and 1K alignment */
		*(.rodata.pt1)
		*(.rodata.pt2)
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

	.data 0x11000000 : AT (__rom_data_start) {
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

	.data.core1 0x12000000 : {
		__data_core1_start = .;
		. = ALIGN(16);
		__data_core1_end = .;
	} : RAM2

	.bss.core1 ALIGN(16) : {
		__bss_core1_start = .;
		*(.core1.bss)
		*(.core1.bss.*)
		__bss_core1_end = .;
	} : NONE

	.data.core2 0x13000000 : {
		__data_core2_start = .;
		. = ALIGN(16);
		__data_core2_end = .;
	} : RAM2

	.bss.core2 ALIGN(16) : {
		__bss_core2_start = .;
		*(.core2.bss)
		*(.core2.bss.*)
		__bss_core2_end = .;
	} : NONE

	.data.core3 0x14000000 : {
		__data_core3_start = .;
		. = ALIGN(16);
		__data_core3_end = .;
	} : RAM2

	.bss.core3 ALIGN(16) : {
		__bss_core3_start = .;
		*(.core3.bss)
		*(.core3.bss.*)
		__bss_core3_end = .;
	} : NONE

	/DISCARD/ : {
		*(.eh_frame)
	}
}
