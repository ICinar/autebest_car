/*
 * ipi.h
 *
 * Multicore IPI-queue management.
 *
 * azuepke, 2015-03-30: initial
 */

#ifndef __IPI_H__
#define __IPI_H__

#ifdef SMP

#include <ipi_state.h>

/** IPI configuration */
extern const uint8_t num_ipi_actions;
extern const struct ipi_cfg ipi_cfg[];

void ipi_enqueue(unsigned int target_cpu, const void *object, uint8_t action, uint8_t aux);
void ipi_send(uint32_t cpu_mask);

#endif

#endif
