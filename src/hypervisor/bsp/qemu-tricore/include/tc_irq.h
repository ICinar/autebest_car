/*
 * tc_irq.h
 *
 * Generic IRQ management for TriCore.
 *
 * azuepke, 2014-12-31: initial
 */

#ifndef __TC_IRQ_H__
#define __TC_IRQ_H__

#include <hv_compiler.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

/** An entry in the interrupt table. */
struct tc_irq {
	/** Pointer to interrupt SRC register, skipped if NULL. */
	uint32_t *src;
	/** Interrupt priority, also matches array index in the interrupt table. */
	uint8_t srpn;
	/** Type of Service, encodes the receiving CPU or DMA engine. */
	uint8_t tos;
	/* Padding. */
	uint16_t padding;
};

/** Generate an entry in the interrupt table. */
#define TC_IRQ_ENTRY(_addr, _srpn, _tos)	{	\
		.src = (uint32_t *)(_addr),	\
		.srpn = (_srpn),	\
		.tos = (_tos),	\
		.padding = 0,	\
	}

/** Type of Service selection. */
#define SRC_TOS_CPU			0
#define SRC_TOS_PCP			1

#define SRC_TOS_CPU0		0
#define SRC_TOS_CPU1		1
#define SRC_TOS_CPU2		2
#define SRC_TOS_DMA			3

/** An interrupt table descriptor. */
struct tc_irq_table {
	/** Pointer to interrupt table array. */
	const struct tc_irq *entries;
	/** Number of entries in the table */
	uint8_t num;
	/** Type of Service, encodes the receiving CPU or DMA engine. */
	uint8_t tos;
	/* Padding. */
	uint16_t padding;
};

/** Generate an entry in the interrupt table's table. */
#define TC_IRQ_TABLE(_table, _num, _tos)	{	\
		.entries = (_table),	\
		.num = (_num),	\
		.tos = (_tos),	\
		.padding = 0,	\
	}

#ifdef AURIX_IRQ
/* AURIX uses a different encoding in SRC registers */

/** Bits in Service Request Control Registers. */
#define SRC_SRPN(x)			((x)<<0)	/* interrupt priority */
#define SRC_SRE_BIT			10			/* service request enable */
#define SRC_SRE				(1<<SRC_SRE_BIT)
#define SRC_TOS(x)			((x)<<11)	/* type of service */
										/* 0: CPU #0 */
										/* 1: CPU #1 */
										/* 2: CPU #2 */
										/* 3: DMA */
#define SRC_SRR				(1<<24)		/* service request flag (sticky) */
#define SRC_CLRR_BIT		25			/* clear request flag */
#define SRC_CLRR			(1<<SRC_CLRR_BIT)
#define SRC_SETR_BIT		26			/* set request flag */
#define SRC_SETR			(1<<SRC_SETR_BIT)
#define SRC_IOV				(1<<27)		/* interrupt overflow flag (sticky) */
#define SRC_IOVCLR			(1<<28)		/* clear interrupt overflow flag */
#define SRC_SWS				(1<<29)		/* software sticky bit (sticky) */
#define SRC_SWSCLR			(1<<30)		/* clear software sticky bit */

#else
/* classic encoding in SRC registers */

/** Bits in Service Request Control Registers. */
#define SRC_SRPN(x)			((x)<<0)	/* interrupt priority */
#define SRC_TOS(x)			((x)<<10)	/* type of service */
										/* 0: CPU #0 */
										/* 1: PCP */
#define SRC_SRE_BIT			12			/* service request enable */
#define SRC_SRE				(1<<SRC_SRE_BIT)
#define SRC_SRR				(1<<13)		/* service request flag (sticky) */
#define SRC_CLRR_BIT		14			/* clear request flag */
#define SRC_CLRR			(1<<SRC_CLRR_BIT)
#define SRC_SETR_BIT		15			/* set request flag */
#define SRC_SETR			(1<<SRC_SETR_BIT)

#endif


/** Read SRC register. */
static inline uint32_t tc_src_read(uint32_t *src)
{
	uint32_t val;
	__asm__ volatile ("ld.w	%0, %1" : "=&d" (val) : "m" (*src) : "memory");
	return val;
}

/** Write SRC register. */
static inline void tc_src_write(uint32_t *src, uint32_t val)
{
	__asm__ volatile ("st.w	%0, %1" : "=m" (*src) : "d" (val) : "memory");
}

/** Enable a specific interrupt (set SRE bit). */
static inline void tc_irq_enable(const struct tc_irq *entry)
{
	uint64_t mask;

	__asm__ ("imask %A0, 1, %1, 1" : "=&d" (mask) : "i" (SRC_SRE_BIT));
	__asm__ volatile ("ldmst %0, %A1" : "+m" (*entry->src) : "d" (mask) : "memory");
}

/** Disable a specific interrupt (clear SRE bit). */
static inline void tc_irq_disable(const struct tc_irq *entry)
{
	uint64_t mask;

	__asm__ ("imask %A0, 0, %1, 1" : "=&d" (mask) : "i" (SRC_SRE_BIT));
	__asm__ volatile ("ldmst %0, %A1" : "+m" (*entry->src) : "d" (mask) : "memory");
}

/** Set Service Request flag (SRR) via SETR bit. */
static inline void tc_irq_setr(const struct tc_irq *entry)
{
	uint64_t mask;

	__asm__ ("imask %A0, 1, %1, 1" : "=&d" (mask) : "i" (SRC_SETR_BIT));
	__asm__ volatile ("ldmst %0, %A1" : "+m" (*entry->src) : "d" (mask) : "memory");
}

/** Clear Service Request flag (SRR) via CLEARR bit. */
static inline void tc_irq_clearr(const struct tc_irq *entry)
{
	uint64_t mask;

	__asm__ ("imask %A0, 1, %1, 1" : "=&d" (mask) : "i" (SRC_CLRR_BIT));
	__asm__ volatile ("ldmst %0, %A1" : "+m" (*entry->src) : "d" (mask) : "memory");
}

/** Check if a Service Request is pending (SRR bit set). */
static inline int tc_irq_pending(const struct tc_irq *entry)
{
	return tc_src_read(entry->src) & SRC_SRR;
}


/* provided by tc_irq_conf_*.c */
extern const unsigned int num_tc_irq_tables;
extern const struct tc_irq_table tc_irq_tables[];

/** Get Interrupt table descriptor. */
static inline const struct tc_irq_table *tc_irq_get_table(
	unsigned int tos)
{
	assert(tos < num_tc_irq_tables);
	return &tc_irq_tables[tos];
}

/** Get Interrupt table entry. */
static inline const struct tc_irq *tc_irq_get_entry(
	unsigned int tos,
	unsigned int srpn)
{
	const struct tc_irq_table *table;

	table = tc_irq_get_table(tos);
	assert(table != NULL);
	assert(table->tos == tos);

	assert(srpn < table->num);
	return &table->entries[srpn];
}

/** Setup all interrupts in the given interrupt table. */
void tc_irq_setup(const struct tc_irq_table *table);

/** Setup all interrupt tables. */
void tc_irq_setup_all(void);

#endif
