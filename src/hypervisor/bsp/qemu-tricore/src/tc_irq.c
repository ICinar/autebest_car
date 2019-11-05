/*
 * tc_irq.c
 *
 * Generic IRQ management for TriCore.
 *
 * azuepke, 2014-12-31: initial
 */

#include <tc_irq.h>
#include <kernel.h>
#include <assert.h>
#include <board.h>
#include <board_stuff.h>
#include <sched.h>	/* num_cpus */
#include <isr.h>
#include <hm.h>


void board_irq_enable(unsigned int irq_id)
{
	const struct tc_irq *entry;

	entry = tc_irq_get_entry(arch_cpu_id(), irq_id);
	assert(entry != NULL);
	assert(entry->src != NULL);
	assert(entry->tos == arch_cpu_id());

	tc_irq_enable(entry);
}

void board_irq_disable(unsigned int irq_id)
{
	const struct tc_irq *entry;

	entry = tc_irq_get_entry(arch_cpu_id(), irq_id);
	assert(entry != NULL);
	assert(entry->src != NULL);
	assert(entry->tos == arch_cpu_id());

	tc_irq_disable(entry);
}

__cold void board_unhandled_irq_handler(unsigned int irq_id)
{
	hm_system_error(HM_ERROR_UNHANDLED_IRQ, irq_id);
}

/** Setup all interrupts in the given interrupt table. */
__init void tc_irq_setup(const struct tc_irq_table *table)
{
	const struct tc_irq *entry;
	unsigned int tos;
	unsigned int i;
	uint32_t val;

	assert(table != NULL);
	assert(table->entries != NULL);

	assert(table->num > 0);

	tos = table->tos;
	for (i = 0; i < table->num; i++) {
		entry = &table->entries[i];
		if (entry->src != NULL) {
			assert(entry->srpn == i);
			assert(entry->tos == tos);
			/* interrupt must not be enabled already! */
			assert((tc_src_read(entry->src) & SRC_SRE) == 0);

			/* set TOS and SRPN, but keep interrupt disabled */
			val = SRC_TOS(tos) | SRC_SRPN(i);
			tc_src_write(entry->src, val);
		}
	}
}

__init void tc_irq_setup_all(void)
{
	const struct tc_irq_table *table;
	unsigned int i;

	assert(num_tc_irq_tables > 0);

	for (i = 0; i < num_tc_irq_tables; i++) {
		table = tc_irq_get_table(i);
		assert(table != NULL);
		assert(table->tos == i);

		tc_irq_setup(table);
	}
}
