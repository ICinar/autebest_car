/*
 * tc_layout.h
 *
 * Tricore memory layout
 *
 * azuepke, 2014-12-21: initial
 */

#ifndef __TC_LAYOUT_H__
#define __TC_LAYOUT_H__

/* memory layout on AURIX */
#define CRAM_BASE 0xc0000000
#define CRAM_VECTOR_BASE (CRAM_BASE + 0)
#define CRAM_VECTOR_IRQ_OFFSET 0x7fc
#define CRAM_VECTOR_IRQ  (CRAM_BASE + CRAM_VECTOR_IRQ_OFFSET)
#define CRAM_VECTOR_SIZE 0x800

#endif
