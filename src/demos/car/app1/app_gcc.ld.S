/*
 * app_gcc.ld.S
 *
 * Simple linker script for user space applications
 *
 * azuepke, 2013-12-06: initial
 * azuepke, 2015-06-26: adapted for two-step dummy/final ELF concept
 * azuepke, 2015-06-29: small data sections
 */

#ifdef FINAL_ELF
#include "app.ld.h"
#else
/* defaults for dummy ELF */
#define SECTION_START_text 0x10000000
#define SECTION_START_data 0x20000000
#endif

ENTRY(__sys_startup)

SECTIONS
{
	/* read-only text sections */
	.text SECTION_START_text : {
#if defined __arm__
	__sda1_base = .;	/* magic symbol for config generator, dummy on ARM */
	__sda2_base = .;	/* magic symbol for config generator, dummy on ARM */
#endif
		__text_start = .;
		*(.text)
		*(.text.*)
		__text_end = .;
	}

	/* read-only data sections */
	.rodata : {
		*(.rodata)
		*(.rodata.*)
		*(.rodata1)
	}

	/* const small data area */
	.sdata2 : {
#ifdef __tricore__
		_SMALL_DATA2_ = . + 0x8000;	/* TriCore register a1 points here */
		__sda2_base = . + 0x8000;	/* magic symbol for config generator */
#endif
#if (defined __powerpc__) || (defined __PPC__)
		_SDA2_BASE_ = . + 0x8000;	/* PowerPC register r2 points here */
		__sda2_base = . + 0x8000;	/* magic symbol for config generator */
#endif
		*(.sdata2)
		*(.sdata2.*)
		*(.gnu.linkonce.s2.*)
#ifdef __tricore__
		*(.sdata.rodata)
		*(.sdata.rodata.*)
		*(.srodata)
		*(.srodata.*)
		*(.gnu.linkonce.sr.*)
#endif
	}
	.sbss2 : {
		*(.sbss2)
		*(.gnu.linkonce.sb2.*)
	}

	/* initialized data is placed right behind the text segment */
	. = ALIGN(8);
	__rom_data_start = .;

	/* initialized read/write sections */
	.data SECTION_START_data : AT (__rom_data_start) {
		__ram_data_start = .;
		__data_start = .;
		*(.data)
		*(.data.*)
		*(.data1)
	}

	/* initialized small data area */
	.sdata : AT (__rom_data_start + SIZEOF(.data)) {
#ifdef __tricore__
		_SMALL_DATA_ = . + 0x8000;	/* TriCore register a0 points here */
		__sda1_base = . + 0x8000;	/* magic symbol for config generator */
#endif
#if (defined __powerpc__) || (defined __PPC__)
		_SDA_BASE_ = . + 0x8000;	/* PowerPC register r13 points here */
		__sda1_base = . + 0x8000;	/* magic symbol for config generator */
#endif
		*(.sdata)
		*(.sdata.*)
		*(.gnu.linkonce.s.*)

		. = ALIGN(8);
		__data_end = .;
		__rom_data_end = ABSOLUTE(__rom_data_start) + (__data_end - __data_start);

		/* kernel shared data is kept outside the overwritten area */
		*(.sbss.kernel_shared)
		*(.kernel_shared)
	}

	/* uninitialized small data area */
	. = ALIGN(8);
	__bss_start = .;
	.sbss : {
		*(.sbss)
		*(.sbss.*)
		*(.gnu.linkonce.sb.*)
		*(.scommon)
	}
	/* uninitialized read/write sections */
	.bss : {
		*(.bss)
		*(.bss.*)
		*(COMMON)
	}
	. = ALIGN(8);
	__bss_end = .;

	.stacks : {
		__stack_start = .;
		*(.bss.stack)
		*(.bss.stack.*)
		__stack_end = .;
	}

	/DISCARD/ : {
		*(.eh_frame)
	}
}
