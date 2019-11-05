/* HSM linker script for GCC */

ENTRY(__hsm_entry_point)

/*
 * Program Headers:
 * PT_LOAD: load that 
 * FLAGS:   flags in the pt_flags field: X=1, W=2, R=4
 */
PHDRS
{
   ROM PT_LOAD FLAGS(5);
   RAM PT_LOAD FLAGS(6);
}

/*
MEMORY
{
   rom_1     : ORIGIN = 0x00610000,   LENGTH = 0x00000020
   rom       : ORIGIN = 0x00610020,   LENGTH = 0x0000FFE0
	ram_vect  : ORIGIN = 0xA0000000,   LENGTH = 0x00000100
   ram       : ORIGIN = 0xA0000100,   LENGTH = 0x00007F00
}
*/

SECTIONS
{
   .text 0x00610000 :
   {
	   *(.text.hsmboot) /* must be first */
      __hsm_entry_point = . ;
      *(.text.hsmstart)
      *(.text_vle)
      *(.text)
      *(.fini)
      *(.eini)

      *(.init)
      *(.init_vle)

      *(.rdata)
      *(.rodata)
       . = ALIGN(0x10);

      __rom_data_start = . ;
      /* . = . + (__data_end - __data_start) ; */
      __sdata0_rom_start = . ;
      __rom_data_end = . ;
   } : ROM
  
   .data 0xA0000100 : AT(__rom_data_start)
   {
		__ram_start = . ;
	
		. = ALIGN(16);
		__data_start = . ;
	   *(.data)
	   *(.sdata)
		. = ALIGN(16);
		__data_end = . ;

		__ram_end = . ;
   } : RAM

   .bss ALIGN(16) :
   {
		__bss_start = . ;
	    *(.sbss)
	    *(.bss)
		__bss_end = . ;
   } : NONE
} 
