/*
 * tc_irq_conf_tsim.c
 *
 * IRQ configuration for TSIM 1.6P and E models (tsim16p_e.exe of 2012-03-14)
 *
 * azuepke, 2014-12-31: initial
 */

#include <tc_irq.h>

/* IRQ config for TSIM */
const struct tc_irq tc_irq_table_cpu0[24] = {
	/* The first two entries are unused for CPUs:
	 * priority #0 can never receive an interrupt, and
	 * priority #1 is usually used for rescheduling.
	 */
	TC_IRQ_ENTRY(0x00000000,   0, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,   1, SRC_TOS_CPU0),
	/* STM Timer #0 at 0xf0000000 */
	TC_IRQ_ENTRY(0xf00000fc,   2, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,   3, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,   4, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,   5, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,   6, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,   7, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,   8, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,   9, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  10, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  11, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  12, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  13, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  14, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  15, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  16, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  17, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  18, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  19, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  20, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  21, SRC_TOS_CPU0),
	TC_IRQ_ENTRY(0x00000000,  22, SRC_TOS_CPU0),
	/* STM Timer #1 at 0xf0000000 */
	/* NOTE: the default config requires a valid interrupt #23 */
	TC_IRQ_ENTRY(0xf00000f8,  23, SRC_TOS_CPU0),
};

const unsigned int num_tc_irq_tables = 1;
const struct tc_irq_table tc_irq_tables[1] = {
	TC_IRQ_TABLE(tc_irq_table_cpu0, 24, 0),
};
