/*
 * linker.h
 *
 * Linker specific symbols.
 *
 * azuepke, 2013-06-22: initial
 * azuepke, 2013-12-06: added __rom_data and __heap_end
 * azuepke, 2014-05-01: removed heap
 */

#ifndef __LINKER_H__
#define __LINKER_H__

/* .text in ROM */
void __text_start(void);
void __text_end(void);
/* .data placed after .text in ROM */
void __rom_data_start(void);
void __rom_data_end(void);
/* .data in RAM */
void __data_start(void);
void __data_end(void);
/* .bss in RAM */
void __bss_start(void);
void __bss_end(void);

#if defined(SMP) && defined(__KERNEL)
/* extra sections for additional cores of the kernel */

/* .data in RAM */
void __data_core1_start(void);
void __data_core1_end(void);
/* .bss in RAM */
void __bss_core1_start(void);
void __bss_core1_end(void);

/* .data in RAM */
void __data_core2_start(void);
void __data_core2_end(void);
/* .bss in RAM */
void __bss_core2_start(void);
void __bss_core2_end(void);

/* .data in RAM */
void __data_core3_start(void);
void __data_core3_end(void);
/* .bss in RAM */
void __bss_core3_start(void);
void __bss_core3_end(void);
#endif

#endif
